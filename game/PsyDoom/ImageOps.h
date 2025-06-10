#pragma once

#include "Macros.h"

#include <cstddef>
#include <cstdint>

//------------------------------------------------------------------------------------------------------------------------------------------
// Utility functions for manipulating images
//------------------------------------------------------------------------------------------------------------------------------------------
BEGIN_NAMESPACE(ImageOps)

// Holds an integer x/y coordinate or size
struct Vec2i {
    int32_t x;
    int32_t y;
};

// Contains the integer position and size of a rectangle
struct Rect {
    Vec2i pos;
    Vec2i size;
};

// Contains a reference to a block of image data
template <class PixelT>
struct Image {
    PixelT*     pPixels;        // Pointer to the top left pixel of the image
    uint32_t    width;          // Width of the image in pixels
    uint32_t    height;         // Height of the image in pixels
    uint32_t    rowStridePx;    // How many pixels to skip when moving onto the next row of pixels
};

template <class PixelT>
void Blit(
    const Image<PixelT>& srcImg,
    const Image<PixelT>& dstImg,
    const Rect& srcRect,
    const Vec2i& dstPos
) noexcept;

void Convert8bppTo32bit(
    const uint8_t* const pSrcPixels,
    uint32_t* const pDstPixels,
    const uint32_t imgW,
    const uint32_t imgH,
    const uint32_t* const pPalette
) noexcept;

template <class PixelT>
void DebugPrint(
    const char* const pText,
    const size_t textLen,
    const int32_t dstTopLeftX,
    const int32_t dstTopLeftY,
    const PixelT fontColor,
    const Image<PixelT>& dstImg
) noexcept;

END_NAMESPACE(ImageOps)
