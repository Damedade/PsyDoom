//------------------------------------------------------------------------------------------------------------------------------------------
// A module containing map patches to apply to the Alpha 0.05 version of PSX Doom
//------------------------------------------------------------------------------------------------------------------------------------------
#include "MapPatches.h"

#include "Doom/Renderer/r_data.h"
#include "MapPatcherUtils.h"

using namespace MapPatcherUtils;

BEGIN_NAMESPACE(MapPatches)

//------------------------------------------------------------------------------------------------------------------------------------------
// Fix issues for MAP05: Phobos Lab
//------------------------------------------------------------------------------------------------------------------------------------------
static void patchMap_PhobosLab() noexcept {
    applyOriginalMapCommonPatches();

    if (shouldApplyMapPatches_GamePlay()) {
        // Fix the exit switch not being visible
        gpLines[701].flags &= ~ML_DONTPEGBOTTOM;
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Fix issues for MAP07: Computer Station
//------------------------------------------------------------------------------------------------------------------------------------------
static void patchMap_ComputerStation() noexcept {
    applyOriginalMapCommonPatches();

    if (shouldApplyMapPatches_Visual()) {
        // Fix the side wall for platform that raises (beside the slime and computer map powerup) not having any texture
        gpSides[gpLines[460].sidenum[0]].bottomtexture = gpSides[gpLines[432].sidenum[0]].bottomtexture;
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Fix issues for MAP08: Phobos Anomaly
//------------------------------------------------------------------------------------------------------------------------------------------
static void patchMap_PhobosAnomaly() noexcept {
    applyOriginalMapCommonPatches();

    if (shouldApplyMapPatches_GamePlay()) {
        // Fix the tag for the kill Baron special (was 666)
        gpSectors[62].tag = 671;
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Fix issues for MAP25: Unknown Alpha Map
//------------------------------------------------------------------------------------------------------------------------------------------
static void patchMap_UnknownAlphaMap() noexcept {
    applyOriginalMapCommonPatches();

    if (shouldApplyMapPatches_GamePlay()) {
        // Fix the switch to open the door to the exit not being visible
        gpSides[gpLines[283].sidenum[0]].rowoffset = -55 * FRACUNIT;
    }

    if (shouldApplyMapPatches_Visual()) {
        // Fix the exit sign having missing textures
        const int32_t exitTex = R_TextureNumForName("EXIT");;

        for (int32_t lineIdx = 335; lineIdx <= 338; ++lineIdx) {
            gpSides[gpLines[lineIdx].sidenum[0]].toptexture = exitTex;
        }
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Fix issues for MAP33: The Gantlet
//------------------------------------------------------------------------------------------------------------------------------------------
static void patchMap_TheGantlet() noexcept {
    applyOriginalMapCommonPatches();

    if (shouldApplyMapPatches_GamePlay()) {
        // Fix an important progression switch (past the blue door) not being visible
        gpSides[gpLines[429].sidenum[0]].textureoffset = 4 * FRACUNIT;
        gpSides[gpLines[429].sidenum[0]].rowoffset = 56 * FRACUNIT;
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------
// All of the map patches for this game type
//------------------------------------------------------------------------------------------------------------------------------------------
static const PatchDef gPatchArray_Doom_Alpha_0_05[] = {
    { 118979, 0x9B790D104F9AB5D7, 0xEEC863121C609710, patchMap_PhobosLab                    },      // MAP05
    { 125419, 0xE3D1DBB48A385680, 0x8CE1E362E467334A, patchMap_ComputerStation              },      // MAP07
    {  62155, 0xDF87F4E0FA0626F5, 0x422FBCBCADF70594, patchMap_PhobosAnomaly                },      // MAP08
    {  74591, 0x2E85C0E75B32BE7C, 0xED123A5B177C3B5B, patchMap_UnknownAlphaMap              },      // MAP25
    {  90849, 0x570d691dc2986aa1, 0xeffdea7d5c29fe61, patchMap_TheGantlet                   },      // MAP33
};

const PatchList gPatches_Doom_Alpha_0_05 = { gPatchArray_Doom_Alpha_0_05, C_ARRAY_SIZE(gPatchArray_Doom_Alpha_0_05) };

END_NAMESPACE(MapPatches)
