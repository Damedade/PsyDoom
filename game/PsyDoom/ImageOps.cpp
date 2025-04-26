#include "ImageOps.h"

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

END_NAMESPACE(ImageOps)
