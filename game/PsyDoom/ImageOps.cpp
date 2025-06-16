#include "ImageOps.h"

#include "Asserts.h"
#include "PsyQ/LIBGPU.h"

#include <algorithm>
#include <cstring>

BEGIN_NAMESPACE(ImageOps)

//------------------------------------------------------------------------------------------------------------------------------------------
// Clips a rectangle to be within a given image bounds
//------------------------------------------------------------------------------------------------------------------------------------------
Rect ClipRect(const Rect& origRect, const uint32_t boundsW, const uint32_t boundsH) noexcept {
    Rect r = origRect;

    if (r.pos.x < 0) {
        r.size.x += r.pos.x;
        r.pos.x = 0;
    }
    else if ((uint32_t) r.pos.x >= boundsW) {
        r.pos.x = boundsW;
        r.size.x = 0;
    }

    if (r.pos.y < 0) {
        r.size.y += r.pos.y;
        r.pos.y = 0;
    }
    else if ((uint32_t) r.pos.y >= boundsH) {
        r.pos.y = boundsH;
        r.size.y = 0;
    }

    return r;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Blit from a region within a given source image to a destination image
//------------------------------------------------------------------------------------------------------------------------------------------
template <class PixelT>
void Blit(
    const Image<PixelT>& srcImg,
    const Image<PixelT>& dstImg,
    const Rect& srcRect,
    const Vec2i& dstPos
) noexcept
{
    // Convert the source image rectangle to the destination image coordinate system, then clip against the destination image.
    // Note: to do coord system conversion, adjust the source rect position by subtracting 'srcRect.pos' and then adding 'dstPos'.
    Rect r = { dstPos, srcRect.size };
    r = ClipRect(r, dstImg.width, dstImg.height);

    if ((r.size.x <= 0) || (r.size.y <= 0))
        return;

    // Convert the rect back to source image coordinates by performing the inverse of the transform mentioned above.
    // Then clip against the source image.
    r.pos.x = r.pos.x - dstPos.x + srcRect.pos.x;
    r.pos.y = r.pos.y - dstPos.y + srcRect.pos.y;
    r = ClipRect(r, srcImg.width, srcImg.height);

    if ((r.size.x <= 0) || (r.size.y <= 0))
        return;

    // Adjust the destination position based our adjustments to the source image position
    const int32_t srcPosAdjustmentX = r.pos.x - srcRect.pos.x;
    const int32_t srcPosAdjustmentY = r.pos.y - srcRect.pos.y;

    const Vec2i clippedDstPos = {
        dstPos.x + srcPosAdjustmentX,
        dstPos.y + srcPosAdjustmentY,
    };

    // Okay, now we are ready to blit.
    // Setup our source and destination row pointers and strides.
    const uint32_t srcStride = srcImg.rowStridePx;
    const uint32_t dstStride = dstImg.rowStridePx;
    const uint32_t rowSize = sizeof(PixelT) * r.size.x;

    uint32_t numRowsLeft = r.size.y;
    PixelT* pSrcRow = srcImg.pPixels + r.pos.x + (uint32_t) r.pos.y * srcStride;
    PixelT* pDstRow = dstImg.pPixels + clippedDstPos.x + (uint32_t) clippedDstPos.y * dstStride;

    while (numRowsLeft > 0) {
        std::memcpy(pDstRow, pSrcRow, rowSize);
        pSrcRow += srcStride;
        pDstRow += dstStride;
        numRowsLeft--;
    }
}

// Supported variants of this function:
#define DEFINE_BLIT_VARIANT(PixelT)\
    template void Blit<PixelT>(const Image<PixelT>& srcImg, const Image<PixelT>& dstImg, const Rect& srcRect, const Vec2i& dstPos) noexcept

DEFINE_BLIT_VARIANT(uint8_t);
DEFINE_BLIT_VARIANT(uint16_t);
DEFINE_BLIT_VARIANT(uint32_t);

#undef DEFINE_BLIT_VARIANT

//------------------------------------------------------------------------------------------------------------------------------------------
// Converts an 8-bit palettized image to 32-bit
//------------------------------------------------------------------------------------------------------------------------------------------
void Convert8bppTo32bit(
    const uint8_t* const pSrcPixels,
    uint32_t* const pDstPixels,
    const uint32_t imgW,
    const uint32_t imgH,
    const uint32_t* const pPalette
) noexcept
{
    const uint32_t numPixels = imgW * imgH;

    for (uint32_t i = 0; i < numPixels; ++i) {
        const uint8_t colorIndex = pSrcPixels[i];
        pDstPixels[i] = pPalette[colorIndex];
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Draw text onto an image using the LIBGPU debug font.
// Used to produce the 'Legals' text for the Alpha 0.05 version of PSX Doom.
//------------------------------------------------------------------------------------------------------------------------------------------
template <class PixelT>
void DebugPrint(
    const char* const pText,
    const size_t textLen,
    const int32_t dstTopLeftX,
    const int32_t dstTopLeftY,
    const PixelT fontColor,
    const Image<PixelT>& dstImg
) noexcept
{
    // Sanity checks and abort if the destination image is zero sized
    ASSERT(pText || (textLen == 0));

    if ((dstImg.width == 0) || (dstImg.height == 0))
        return;

    ASSERT(dstImg.pPixels);

    // Cache these values locally
    PixelT* const pDstPixels = dstImg.pPixels;
    const uint32_t dstW = dstImg.width;
    const uint32_t dstH = dstImg.height;
    const uint32_t dstStride = dstImg.rowStridePx;

    // Text printing loop
    int32_t outputCursorX = dstTopLeftX;
    int32_t outputCursorY = dstTopLeftY;

    for (uint32_t charIdx = 0; charIdx < textLen; ++charIdx) {
        // Figure out the glyph index and skip the character if it is out of range
        const char rawChar = pText[charIdx];
        const char charUpper = (rawChar >= 'a' && rawChar <= 'z') ? rawChar - 32 : rawChar;
        const int32_t glyphIdx = charUpper - 32;

        if ((glyphIdx < 0) || (glyphIdx >= 64))
            continue;

        // Handle whitespace characters
        if (charUpper == ' ') {
            outputCursorX += 8;
            continue;
        }

        if (charUpper == '\t') {
            outputCursorX += 32;
            continue;
        }

        if (charUpper == '\n') {
            outputCursorX = dstTopLeftX;
            outputCursorY += 8;
            continue;
        }

        // Which row and column of the font (128x32 @ 4bpp) to get this glyph from?
        // Also compute the top left pixel in the source image.
        const uint32_t glyphRow = (uint32_t) glyphIdx / 16u;
        const uint32_t glyphCol = (uint32_t) glyphIdx % 16u;

        constexpr uint32_t SRC_ROW_STRIDE = 64;
        const uint8_t* const pSrcTopLeft = gLIBGPU_DebugFont_Texture + (glyphRow * 8u * SRC_ROW_STRIDE) + (glyphCol * 4u);

        // Output the 8x8 pixels of the glyph to the destination image
        for (int32_t glyphY = 0; glyphY < 8; ++glyphY) {
            // Abort if the destination row is out of range
            const int32_t dstY = outputCursorY + glyphY;

            if (dstY < 0)
                continue;

            if (dstY >= (int32_t) dstH)
                break;

            // Get the source and destination row
            const uint8_t* const pSrcRow = pSrcTopLeft + glyphY * SRC_ROW_STRIDE;
            PixelT* const pDstRow = &pDstPixels[(uint32_t) dstY * dstStride];

            // Output this row of pixels
            for (int32_t glyphX = 0; glyphX < 8; ++glyphX) {
                // Get whether the source pixel is regarded as opaque/drawn or not and skip over it if not
                const uint8_t srcByte = pSrcRow[(uint32_t) glyphX / 2u];
                const uint8_t srcNibble = (glyphX & 1) ? (uint8_t)(srcByte >> 4u) : (uint8_t)(srcByte & 0xF);
                const bool bOpaquePixel = (srcNibble != 0);

                if (!bOpaquePixel)
                    continue;

                // Get the destination image x value and skip over this pixel if its out of range
                const int32_t dstX = outputCursorX + glyphX;

                if (dstX < 0)
                    continue;

                if (dstX >= (int32_t) dstW)
                    break;

                // Save the destination pixel
                pDstRow[dstX] = fontColor;
            }
        }

        // Move onto the next output location
        outputCursorX += 8;
    }
}

// Supported variants of this function:
#define DEFINE_DEBUG_PRINT_VARIANT(PixelT)\
    template void DebugPrint<PixelT>(\
        const char* const pText,\
        const size_t textLen,\
        const int32_t dstX,\
        const int32_t dstY,\
        const PixelT fontColor,\
        const Image<PixelT>& dstImg\
    ) noexcept

DEFINE_DEBUG_PRINT_VARIANT(uint8_t);
DEFINE_DEBUG_PRINT_VARIANT(uint16_t);
DEFINE_DEBUG_PRINT_VARIANT(uint32_t);

#undef DEFINE_DEBUG_PRINT_VARIANT

END_NAMESPACE(ImageOps)
