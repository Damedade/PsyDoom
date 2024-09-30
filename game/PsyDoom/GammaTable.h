#pragma once

#include <cstdint>

//------------------------------------------------------------------------------------------------------------------------------------------
// Precomputed gamma adjustment table.
// Maps color component values from before gamma adjustment to afterwards.
//------------------------------------------------------------------------------------------------------------------------------------------
struct GammaTable {
    uint8_t remapTbl5[32];      // 5-bit color remap table. Note: output is 8-bit!
    uint8_t remapTbl6[64];      // 6-bit color remap table. Note: output is 8-bit!
    uint8_t remapTbl8[256];     // 8-bit color remap table.

    void build(const float gamma) noexcept;
};
