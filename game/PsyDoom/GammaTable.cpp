#include "GammaTable.h"

#include <algorithm>
#include <cmath>

//------------------------------------------------------------------------------------------------------------------------------------------
// Builds the gamma adjustment mapping table.
// A gamma of '1.0' means no adjustment to the color values.
//------------------------------------------------------------------------------------------------------------------------------------------
void GammaTable::build(const float gamma) noexcept {
    const float exponent = 1.0f / gamma;

    for (uint32_t i = 0; i < 32; ++i) {
        const float color8f = (float)(i << 3);
        const float adjustedCol8f = std::pow(color8f / 255.0f, exponent) * 255.0f;
        remapTbl5[i] = (uint8_t) std::min(adjustedCol8f + 0.5f, 255.0f);
    }

    for (uint32_t i = 0; i < 64; ++i) {
        const float color8f = (float)(i << 2);
        const float adjustedCol8f = std::pow(color8f / 255.0f, exponent) * 255.0f;
        remapTbl6[i] = (uint8_t) std::min(adjustedCol8f + 0.5f, 255.0f);
    }

    for (uint32_t i = 0; i < 256; ++i) {
        const float color8f = (float) i;
        const float adjustedCol8f = std::pow(color8f / 255.0f, exponent) * 255.0f;
        remapTbl8[i] = (uint8_t) std::min(adjustedCol8f + 0.5f, 255.0f);
    }
}
