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
static const PatchDef gPatchArray_Doom_Alpha_0_32[] = {
    {  56435, 0x3431117192B15EC3, 0x0017BCC6054C2F45, applyOriginalMapCommonPatches },      // MAP01
    { 119369, 0x1AE729DF4AFE0372, 0xD1BCC6C6B2A8A9F7, applyOriginalMapCommonPatches },      // MAP02
    { 108495, 0xBF39E1C5BFB1ADF5, 0x75B2FF2FACDC5B5E, applyOriginalMapCommonPatches },      // MAP03
    {  92341, 0x863049D79469A975, 0x8FEA34DDD53FC0B2, applyOriginalMapCommonPatches },      // MAP04
    {  89450, 0xFCEC1C266C0F9880, 0x2DC01F1692E14548, applyOriginalMapCommonPatches },      // MAP05
    { 124172, 0xD611F44FA323DFB5, 0xE2E793413BCDB10C, applyOriginalMapCommonPatches },      // MAP06
    { 106103, 0x443D5352B7755816, 0x0C85E0293CD08F5C, applyOriginalMapCommonPatches },      // MAP07
    {  51882, 0xF2BBFFA1EE73B88A, 0x2B685344D101E3A0, applyOriginalMapCommonPatches },      // MAP08
    {  47025, 0x309E1FCB8C9C25AA, 0x49B6860A56F0F76B, applyOriginalMapCommonPatches },      // MAP09
    {  96251, 0x31CF22864E8E03F0, 0x8B80403B947D9EFE, applyOriginalMapCommonPatches },      // MAP10
    {  74912, 0x2C1239F6225A7A1B, 0x82BDED22C0BD25BF, applyOriginalMapCommonPatches },      // MAP11
    { 119289, 0x727030F0D6F18735, 0xEF357F31A80A556C, applyOriginalMapCommonPatches },      // MAP12
    {  83505, 0x3DCACD3D02831569, 0x231A361616B8338C, applyOriginalMapCommonPatches },      // MAP13
    {  85285, 0xD2DCFAA1C901982B, 0x4D822BA4D0DCCC1B, applyOriginalMapCommonPatches },      // MAP14
    {  84946, 0x012AEB139669F80D, 0x1FE71BAA4F45152D, applyOriginalMapCommonPatches },      // MAP15
    {  27956, 0x39B94C1CF5E19EB0, 0xE0A691816A8C166A, applyOriginalMapCommonPatches },      // MAP16
    {  56133, 0x2297D9E960503628, 0x5847BC4E4BB725D9, applyOriginalMapCommonPatches },      // MAP17
    {  71000, 0xA661112794C67B72, 0x7E70692F67C2E7DE, applyOriginalMapCommonPatches },      // MAP18
    {  75129, 0x5D0FFABA9255BCA0, 0x9F267080DB064C20, applyOriginalMapCommonPatches },      // MAP19
    { 143513, 0x22B7AE197D29FDBE, 0xDA141947395A5168, applyOriginalMapCommonPatches },      // MAP20
    {  86236, 0x0A5E80C73F5A8AD6, 0x1291429AB013BE98, applyOriginalMapCommonPatches },      // MAP21
    { 109754, 0x07F52522C5F75030, 0xFB47686DCDC04723, applyOriginalMapCommonPatches },      // MAP22
    {  32935, 0x55A24A4ED4053AC3, 0x636CDB24CE519EF8, applyOriginalMapCommonPatches },      // MAP23
    {  56531, 0xAB19C8965ABE1365, 0x96FDF892BB700B7A, applyOriginalMapCommonPatches },      // MAP24
    {  77252, 0x5C03B028654A64C9, 0xA6C9F1B128CA8545, applyOriginalMapCommonPatches },      // MAP25
    { 112416, 0x1FBBC40E9F15CDE5, 0x7BA2B253182A70E6, applyOriginalMapCommonPatches },      // MAP26
    {  82870, 0x52E5431693E11518, 0x75B11B490F461B4E, applyOriginalMapCommonPatches },      // MAP27
    { 146387, 0x3FAE0354629B39B1, 0xA044B72FC50746B9, applyOriginalMapCommonPatches },      // MAP28
    { 160221, 0x913750F5FB672F21, 0xDAAA7DC85A2DFF76, applyOriginalMapCommonPatches },      // MAP29
    { 146590, 0xE498C8158DBD644C, 0x89F5E532FA6428C8, applyOriginalMapCommonPatches },      // MAP30
    {  46210, 0xCF97FD14019E6EB2, 0x5DF0ACAC5013B2B2, applyOriginalMapCommonPatches },      // MAP31
    {  63265, 0x5FDD200DB5971CA6, 0x95C06254DDE14261, applyOriginalMapCommonPatches },      // MAP32
    {  75743, 0xA557BEF1A8C858DC, 0xA4B4C8162AA8ABC3, applyOriginalMapCommonPatches },      // MAP33
    {  67614, 0x39368840F59CF065, 0x839090A0C1E2284A, applyOriginalMapCommonPatches },      // MAP34
    { 114043, 0xA42555A81AABB8FE, 0x3FE5B80699AED9A8, applyOriginalMapCommonPatches },      // MAP35
    { 129405, 0x71D4B8183DA8AB1A, 0xCB1EBF6B909D69EB, applyOriginalMapCommonPatches },      // MAP36
    {  26682, 0x8F9F791CF59AF15D, 0x8B47E43DA524B7FB, applyOriginalMapCommonPatches },      // MAP37
    {  81245, 0x71AC6BA3FA054B65, 0xE3BC5BE3525910C0, applyOriginalMapCommonPatches },      // MAP38
    {  89929, 0x94760AF2EEAE8497, 0xD67E325756DA6234, applyOriginalMapCommonPatches },      // MAP39
    { 130522, 0xF9EDD494E5C6E74D, 0x22A2B510445049A6, applyOriginalMapCommonPatches },      // MAP40
    { 109025, 0x5880B2CAEC0069C7, 0x2F7A0DFE674DD103, applyOriginalMapCommonPatches },      // MAP41
    { 109281, 0x7CDAFFBA64C59867, 0xA5AE9F185F9DF486, applyOriginalMapCommonPatches },      // MAP42
    { 193211, 0x1F459AE931275469, 0xF1C42D7FB5E4FB2A, applyOriginalMapCommonPatches },      // MAP43
    { 110195, 0xD69BA8658580B65C, 0x36FDC7BE6637A9FA, applyOriginalMapCommonPatches },      // MAP44
    { 157097, 0x3FF8F355C724F673, 0x62410792FBA68961, applyOriginalMapCommonPatches },      // MAP45
    { 105495, 0x952A2717A312BA72, 0x22ECCC0949D86154, applyOriginalMapCommonPatches },      // MAP46
    { 185734, 0x7574E6CE87F882A1, 0x170C83E44D5F6150, applyOriginalMapCommonPatches },      // MAP47
    {  54866, 0xF41440631C2B6FB2, 0x728E55510D5AE858, applyOriginalMapCommonPatches },      // MAP48
    {  74343, 0x745C0AD7203B08C4, 0xF3BF08BF5F0E8194, applyOriginalMapCommonPatches },      // MAP49
    {  59256, 0xF7D2790AF7BFD17C, 0x95CFC9DD14A7EC61, applyOriginalMapCommonPatches },      // MAP50
    { 106725, 0xB0351D4EAED515C4, 0xF9EF7EC832777B43, applyOriginalMapCommonPatches },      // MAP51
    { 119347, 0xB429F7331D539807, 0x43D216F68C2350E3, applyOriginalMapCommonPatches },      // MAP52
    { 131799, 0xD57F4CA3E5DBA3AD, 0x81FDDFB491FC9970, applyOriginalMapCommonPatches },      // MAP53
    {  44301, 0xCBE5CBB3144F01F8, 0x456B68942836A2A3, applyOriginalMapCommonPatches },      // MAP54
    {  19237, 0xBB678D231FC2DCD7, 0xB4A69360734967D0, applyOriginalMapCommonPatches },      // MAP55
    {  85094, 0x4E13DB835B2D97E4, 0x0FC41339C916E4EE, applyOriginalMapCommonPatches },      // MAP56
    {  57715, 0x91D91172189D2496, 0x3FA573215B77D0DC, applyOriginalMapCommonPatches },      // MAP57
    { 192815, 0xDD9EE660934A27CA, 0xF726CEC051CBA877, applyOriginalMapCommonPatches },      // MAP58
    {  73304, 0x208A7963070BB94A, 0xF11D5A6A4D9AD475, applyOriginalMapCommonPatches },      // MAP59
};

const PatchList gPatches_Doom_Alpha_0_32 = { gPatchArray_Doom_Alpha_0_32, C_ARRAY_SIZE(gPatchArray_Doom_Alpha_0_32) };

END_NAMESPACE(MapPatches)
