//------------------------------------------------------------------------------------------------------------------------------------------
// A module containing map patches to apply to PlayStation Final Doom
//------------------------------------------------------------------------------------------------------------------------------------------
#include "MapPatches.h"

#include "MapPatcherUtils.h"

using namespace MapPatcherUtils;

BEGIN_NAMESPACE(MapPatches)

//------------------------------------------------------------------------------------------------------------------------------------------
// All of the map patches for this game type
//------------------------------------------------------------------------------------------------------------------------------------------
static const PatchDef gPatchArray_FinalDoom_Alpha[] = {
    { 149933, 0xA4CD904EC28DBD49, 0x37C0688438827FCA, applyOriginalMapCommonPatches },      // MAP01
    { 117111, 0x96311F75CA20BA00, 0xA1017B304496D646, applyOriginalMapCommonPatches },      // MAP02
    { 135524, 0xF3EF9E1F8ED2CB73, 0xCE00BF73F5334F98, applyOriginalMapCommonPatches },      // MAP03
    {  88676, 0x13DEA3B10B670978, 0xE5B85301F853EF60, applyOriginalMapCommonPatches },      // MAP04
    { 133227, 0x23C36CDE735B2AF1, 0x850B4D1FA173BCB1, applyOriginalMapCommonPatches },      // MAP05
    {  88004, 0x47F845701E1F0A8D, 0x464F4F4EDB70BD6B, applyOriginalMapCommonPatches },      // MAP06
    { 166060, 0xD397C7B985EC0395, 0x4D4F886D667E1790, applyOriginalMapCommonPatches },      // MAP07
    { 151747, 0x34083166AE306C65, 0x2FEA2139C7D3BE9A, applyOriginalMapCommonPatches },      // MAP08
    { 103246, 0xD21C91FE0A92E980, 0x1792F0367BD2E7BB, applyOriginalMapCommonPatches },      // MAP09
    { 142852, 0x87866203F8A5FEFB, 0x954F83AF7F93BC0F, applyOriginalMapCommonPatches },      // MAP10
    {  96201, 0x760CAFAD71B9BE4D, 0xDF6409BE60ED8844, applyOriginalMapCommonPatches },      // MAP11
    { 106776, 0xAD3AADE890018818, 0x7D70AC984E7211CC, applyOriginalMapCommonPatches },      // MAP12
    { 152859, 0xA52E052274571E42, 0x317DD6C397240E71, applyOriginalMapCommonPatches },      // MAP13
    {  54125, 0x97FF3F0EA07FD871, 0xA9FD76E41EAB4D6B, applyOriginalMapCommonPatches },      // MAP14
    {  77891, 0x20F93855131B2C1A, 0xD98E0D6C4EAEC765, applyOriginalMapCommonPatches },      // MAP15
    { 154947, 0xB24BB361D3AB0F76, 0xFA1C906C7122140E, applyOriginalMapCommonPatches },      // MAP16
    { 178779, 0x2B925EB9A81D3D58, 0x350A539ECFAFEE1C, applyOriginalMapCommonPatches },      // MAP17
    { 130734, 0xC6B7F7C4230FC694, 0xD895CC94D27C8853, applyOriginalMapCommonPatches },      // MAP18
    { 177888, 0xCAC51738CA64CFBB, 0x5D8C23ADCFAB95DE, applyOriginalMapCommonPatches },      // MAP19
    { 105326, 0x7FC89F35992FDC93, 0x571A8E53D0EC109E, applyOriginalMapCommonPatches },      // MAP20
    { 162561, 0x9E0542C24517B60A, 0x3FF60E2CE2E5D881, applyOriginalMapCommonPatches },      // MAP21
    {  96826, 0x4A1423CF18E8B5E2, 0xBD84D9567AECD313, applyOriginalMapCommonPatches },      // MAP22
    { 168151, 0x54782F08ACAA7FCD, 0xE6DE0F653EB2013F, applyOriginalMapCommonPatches },      // MAP23
    { 121920, 0xDE80C834D37E4A5D, 0x31138173C9CC6696, applyOriginalMapCommonPatches },      // MAP24
    { 113719, 0x7382FD5F98C6C225, 0x5322EDAE41DB0244, applyOriginalMapCommonPatches },      // MAP25
    { 127663, 0x786FB8887D12A5AA, 0x04157AD0C2022EE6, applyOriginalMapCommonPatches },      // MAP26
    { 116635, 0xFD155638DC955370, 0xC15831A700FB011E, applyOriginalMapCommonPatches },      // MAP27
    { 141827, 0xBDF67DC3F739EA54, 0x3D037A06CAAA2B15, applyOriginalMapCommonPatches },      // MAP28
    { 107756, 0x62C2198DA4447CDD, 0xEDFC67F1254798E2, applyOriginalMapCommonPatches },      // MAP29
    { 109594, 0x42AA7CA1627C7552, 0x046C4908050A43A9, applyOriginalMapCommonPatches },      // MAP30
};

const PatchList gPatches_FinalDoom_Alpha = { gPatchArray_FinalDoom_Alpha, C_ARRAY_SIZE(gPatchArray_FinalDoom_Alpha) };

END_NAMESPACE(MapPatches)
