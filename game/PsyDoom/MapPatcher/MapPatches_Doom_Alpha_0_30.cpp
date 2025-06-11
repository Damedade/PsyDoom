//------------------------------------------------------------------------------------------------------------------------------------------
// A module containing map patches to apply to the original PlayStation Doom
//------------------------------------------------------------------------------------------------------------------------------------------
#include "MapPatches.h"

#include "MapPatcherUtils.h"

using namespace MapPatcherUtils;

BEGIN_NAMESPACE(MapPatches)

//------------------------------------------------------------------------------------------------------------------------------------------
// All of the map patches for this game type
//------------------------------------------------------------------------------------------------------------------------------------------
static const PatchDef gPatchArray_Doom_Alpha_0_30[] = {
    {  56435, 0x4DEE58BE8BCF1E73, 0x695A35F8010E4F4D, applyOriginalMapCommonPatches },      // MAP01
    { 119369, 0x75F8C3091EE1C493, 0xFC023D0FF0A4CF9B, applyOriginalMapCommonPatches },      // MAP02
    { 108495, 0xE2FB7ED1B3213E45, 0x263B66DA2A66F60F, applyOriginalMapCommonPatches },      // MAP03
    {  92341, 0x202A6F275CA3D512, 0x2A35AB2587F3CD4E, applyOriginalMapCommonPatches },      // MAP04
    {  89450, 0xD5FAB734061D4F6F, 0xC9B34AA0ABA6C12E, applyOriginalMapCommonPatches },      // MAP05
    { 124172, 0x3230A3CC7D8AA5C2, 0x689E7E811DB81608, applyOriginalMapCommonPatches },      // MAP06
    { 106103, 0x20864A91778FD9BE, 0x2905135A8E34A15D, applyOriginalMapCommonPatches },      // MAP07
    {  50885, 0x211804279DF92496, 0xE2A559796F17CFF9, applyOriginalMapCommonPatches },      // MAP08
    {  47025, 0xAE35EDC7EE264C64, 0xEDB0D9EC2A6326D9, applyOriginalMapCommonPatches },      // MAP09
    {  96251, 0x31CF22864E8E03F0, 0x8B80403B947D9EFE, applyOriginalMapCommonPatches },      // MAP10
    
    {  74912, 0x2C1239F6225A7A1B, 0x82BDED22C0BD25BF, applyOriginalMapCommonPatches },      // MAP11
    { 119289, 0x1A2ABCCA4C071C97, 0xDDD9D1133B709DFE, applyOriginalMapCommonPatches },      // MAP12
    {  83803, 0x8D2496154C00EFDC, 0xD93CCFA11F6E50E0, applyOriginalMapCommonPatches },      // MAP13
    {  85285, 0xD2DCFAA1C901982B, 0x4D822BA4D0DCCC1B, applyOriginalMapCommonPatches },      // MAP14
    {  84946, 0x012AEB139669F80D, 0x1FE71BAA4F45152D, applyOriginalMapCommonPatches },      // MAP15
    {  27956, 0x39B94C1CF5E19EB0, 0xE0A691816A8C166A, applyOriginalMapCommonPatches },      // MAP16
    {  56133, 0xBC3DA1A0C7BF1612, 0x4D6B743306F7DA2B, applyOriginalMapCommonPatches },      // MAP17
    {  70860, 0x5A0B38D0F777EB62, 0xC22A7684D5AA4823, applyOriginalMapCommonPatches },      // MAP18
    {  75129, 0xB61B7D128274B536, 0xD1CA6CB4A4AC4710, applyOriginalMapCommonPatches },      // MAP19
    { 143513, 0xA00219661FCF2759, 0x87D75E2098046459, applyOriginalMapCommonPatches },      // MAP20
    {  85838, 0x61A8034D8BA96F93, 0xA4903A18B290FF40, applyOriginalMapCommonPatches },      // MAP21
    { 110854, 0x4C3D132B09239DA7, 0xD202F8FB050A412E, applyOriginalMapCommonPatches },      // MAP22
    {  32935, 0x55A24A4ED4053AC3, 0x636CDB24CE519EF8, applyOriginalMapCommonPatches },      // MAP23
    {  56531, 0xAB19C8965ABE1365, 0x96FDF892BB700B7A, applyOriginalMapCommonPatches },      // MAP24
    {  77282, 0x1C90877C47872497, 0xB6E7A06DCC561431, applyOriginalMapCommonPatches },      // MAP25
    { 112416, 0x91CFF318C8CFBCB0, 0x12B450144DD453FF, applyOriginalMapCommonPatches },      // MAP26
    {  82870, 0x52E5431693E11518, 0x75B11B490F461B4E, applyOriginalMapCommonPatches },      // MAP27
    { 146387, 0x3FAE0354629B39B1, 0xA044B72FC50746B9, applyOriginalMapCommonPatches },      // MAP28
    { 160201, 0x6F5ADF4094983C31, 0x9CA398F8C6A0021C, applyOriginalMapCommonPatches },      // MAP29
    { 146590, 0xE498C8158DBD644C, 0x89F5E532FA6428C8, applyOriginalMapCommonPatches },      // MAP30
    {  46140, 0x8157BFCE8AD40930, 0x37B42C6C68018BA8, applyOriginalMapCommonPatches },      // MAP31
    {  63265, 0x5FDD200DB5971CA6, 0x95C06254DDE14261, applyOriginalMapCommonPatches },      // MAP32
    {  75743, 0xA557BEF1A8C858DC, 0xA4B4C8162AA8ABC3, applyOriginalMapCommonPatches },      // MAP33
    {  67614, 0x39368840F59CF065, 0x839090A0C1E2284A, applyOriginalMapCommonPatches },      // MAP34
    { 114043, 0xA42555A81AABB8FE, 0x3FE5B80699AED9A8, applyOriginalMapCommonPatches },      // MAP35
    { 129405, 0x71D4B8183DA8AB1A, 0xCB1EBF6B909D69EB, applyOriginalMapCommonPatches },      // MAP36
    {  26682, 0x8F9F791CF59AF15D, 0x8B47E43DA524B7FB, applyOriginalMapCommonPatches },      // MAP37
    {  81245, 0x71AC6BA3FA054B65, 0xE3BC5BE3525910C0, applyOriginalMapCommonPatches },      // MAP38
    {  89929, 0x94760AF2EEAE8497, 0xD67E325756DA6234, applyOriginalMapCommonPatches },      // MAP39
    { 130522, 0xEAB35C3FE2D4E75F, 0x3D9B9B496006310B, applyOriginalMapCommonPatches },      // MAP40
    { 109025, 0x6D913F0F8A31BA58, 0x9F3B4E1BBBDEBA0B, applyOriginalMapCommonPatches },      // MAP41
    { 109281, 0xBEB47C8D316B8263, 0x70B3C54C1D533D3B, applyOriginalMapCommonPatches },      // MAP42
    { 192729, 0xB23F46DE82843C7A, 0x0C51B210BBC58031, applyOriginalMapCommonPatches },      // MAP43
    { 110195, 0xD69BA8658580B65C, 0x36FDC7BE6637A9FA, applyOriginalMapCommonPatches },      // MAP44
    { 157137, 0x95714135B4BE52BC, 0x59C978E3EFCDBB9A, applyOriginalMapCommonPatches },      // MAP45
    { 105497, 0xD339DC9EC1ED9FA2, 0xE3D964DB4F6E1095, applyOriginalMapCommonPatches },      // MAP46
    { 185905, 0xB60E3974F91A40D9, 0x40EDA6C40CE44F2F, applyOriginalMapCommonPatches },      // MAP47
    {  54866, 0xF41440631C2B6FB2, 0x728E55510D5AE858, applyOriginalMapCommonPatches },      // MAP48
    {  74343, 0x745C0AD7203B08C4, 0xF3BF08BF5F0E8194, applyOriginalMapCommonPatches },      // MAP49
    {  59256, 0xF7D2790AF7BFD17C, 0x95CFC9DD14A7EC61, applyOriginalMapCommonPatches },      // MAP50
    { 106725, 0xB0351D4EAED515C4, 0xF9EF7EC832777B43, applyOriginalMapCommonPatches },      // MAP51
    { 119347, 0x4F9FD19028852998, 0xFFAED942C699884C, applyOriginalMapCommonPatches },      // MAP52
    { 131809, 0x69BDB23636703A78, 0xB0B789DACEE6CFBB, applyOriginalMapCommonPatches },      // MAP53
    {  44301, 0xCBE5CBB3144F01F8, 0x456B68942836A2A3, applyOriginalMapCommonPatches },      // MAP54
    {  19237, 0xBB678D231FC2DCD7, 0xB4A69360734967D0, applyOriginalMapCommonPatches },      // MAP55
    {  85134, 0x45980C9B44538F23, 0xCD741B17B6B7DE98, applyOriginalMapCommonPatches },      // MAP56
    {  57066, 0xDE0E48A532B76976, 0x997243D461BFB032, applyOriginalMapCommonPatches },      // MAP57
    { 190604, 0x2BE7C9801EB716A3, 0x411A95EA36C40A68, applyOriginalMapCommonPatches },      // MAP58
    {  57298, 0x993299B3C0ED4163, 0x2FC11E8B55560B53, applyOriginalMapCommonPatches },      // MAP59
};

const PatchList gPatches_Doom_Alpha_0_30 = { gPatchArray_Doom_Alpha_0_30, C_ARRAY_SIZE(gPatchArray_Doom_Alpha_0_30) };

END_NAMESPACE(MapPatches)
