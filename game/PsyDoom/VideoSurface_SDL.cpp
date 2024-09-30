#include "VideoSurface_SDL.h"

#include "Asserts.h"
#include "GammaTable.h"
#include "PlayerPrefs.h"
#include "Video.h"

#include <cstring>
#include <SDL.h>

BEGIN_NAMESPACE(Video)

//------------------------------------------------------------------------------------------------------------------------------------------
// Attempts to create the surface with the specified dimensions.
// If creation fails then the failure is silent and usage of the surface results in NO-OPs.
//------------------------------------------------------------------------------------------------------------------------------------------
VideoSurface_SDL::VideoSurface_SDL(SDL_Renderer& renderer, const uint32_t width, const uint32_t height) noexcept
    : mpTexture(nullptr)
    , mWidth(width)
    , mHeight(height)
{
    if ((width > 0) && (height > 0)) {
        mpTexture = SDL_CreateTexture(&renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, width, height);
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Cleans up surface resources
//------------------------------------------------------------------------------------------------------------------------------------------
VideoSurface_SDL::~VideoSurface_SDL() noexcept {
    if (mpTexture) {
        SDL_DestroyTexture(mpTexture);
        mpTexture = nullptr;
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Get the dimensions of the surface
//------------------------------------------------------------------------------------------------------------------------------------------
uint32_t VideoSurface_SDL::getWidth() const noexcept { return mWidth; }
uint32_t VideoSurface_SDL::getHeight() const noexcept { return mHeight; }

//------------------------------------------------------------------------------------------------------------------------------------------
// Sets the pixels for the surface.
// Ignores the call if the surface was not successfully initialized.
// Also applies gamma correction to the surface (if required), since it is meant to be displayed directly.
//------------------------------------------------------------------------------------------------------------------------------------------
void VideoSurface_SDL::setPixels(const uint32_t* const pSrcPixels) noexcept {
    ASSERT(pSrcPixels);

    // Abort if the texture wasn't successfully initialized
    if (!mpTexture)
        return;

    // Abort if we can't lock the texture for some reason
    void* pCurDstRow = nullptr;
    int pitch = 0;

    if (SDL_LockTexture(mpTexture, nullptr, &pCurDstRow, &pitch) != 0) {
        ASSERT_FAIL("VideoSurface_SDL::setPixels: locking the texture failed!");
        return;
    }

    // Gamma correct related vars
    const bool bDoGammaCorrection = (PlayerPrefs::gGamma1000 != PlayerPrefs::GAMMA_DEFAULT);
    const uint8_t* const pGammaRemapTbl8 = Video::gGammaTbl.remapTbl8;

    // Copy the texture data row by row
    const uint32_t* pCurSrcPixels = pSrcPixels;

    if (!bDoGammaCorrection) {
        // Regular (fast) copy path
        for (uint32_t y = 0; y < mHeight; ++y) {
            std::memcpy(pCurDstRow, pCurSrcPixels, sizeof(uint32_t) * mWidth);
            pCurSrcPixels += mWidth;
            pCurDstRow = (char*) pCurDstRow + pitch;
        }
    }
    else {
        // Gamma corrected copy
        for (uint32_t y = 0; y < mHeight; ++y) {
            uint32_t* const pCurDstRow32 = reinterpret_cast<uint32_t*>(pCurDstRow);

            for (uint32_t x = 0; x < mWidth; ++x) {
                const uint32_t srcPixel = pCurSrcPixels[x];
                const uint8_t r = pGammaRemapTbl8[(uint8_t)(srcPixel)];
                const uint8_t g = pGammaRemapTbl8[(uint8_t)(srcPixel >> 8)];
                const uint8_t b = pGammaRemapTbl8[(uint8_t)(srcPixel >> 16)];

                pCurDstRow32[x] = (srcPixel & 0xFF000000) | (b << 16) | (g << 8) | r;
            }

            pCurSrcPixels += mWidth;
            pCurDstRow = (char*) pCurDstRow + pitch;
        }
    }

    // Done copying, unlock the texture
    SDL_UnlockTexture(mpTexture);
}

END_NAMESPACE(Video)
