#include "IntroLogos.h"

#include "Doom/Base/w_wad.h"
#include "Doom/Game/doomdata.h"
#include "Game.h"
#include "ImageOps.h"
#include "LogoPlayer.h"
#include "PsxVm.h"
#include "Utils.h"
#include "Wess/psxcd.h"

#include <cstring>
#include <memory>
#include <string_view>
#include <vector>

BEGIN_NAMESPACE(IntroLogos)

//------------------------------------------------------------------------------------------------------------------------------------------
// The dimensions of a PSX Doom intro logo which is embedded in the boot executable
//------------------------------------------------------------------------------------------------------------------------------------------
static constexpr uint32_t PSX_DOOM_BOOT_LOGO_W = 256;
static constexpr uint32_t PSX_DOOM_BOOT_LOGO_H = 240;

//------------------------------------------------------------------------------------------------------------------------------------------
// Defines a set of embedded intro logos for a specific variant of the PlayStation Doom boot/startup executable.
// Each logo begins with 256 RGB888 palette entries (768 bytes) followed by an 8-bit color indexed image of 256x240 pixels (61,440 bytes).
// 
// Note: surprisingly all versions of the boot executable contain logos for other regions, even if they are not used.
// For example the Europe and Japan executables contain the US SCEA intro logo.
//------------------------------------------------------------------------------------------------------------------------------------------
struct PsxDoomBootExeLogos {
    uint64_t    exeHashWord1;       // MD5 Hash of the PSX Doom boot executable containing the logo (word 1)
    uint64_t    exeHashWord2;       // MD5 Hash of the PSX Doom boot executable containing the logo (word 2)
    uint32_t    sceaLogoOffset;     // Offset of the SCEA (Sony America) logo within the executable
    uint32_t    sceeLogoOffset;     // Offset of the SCEE (Sony Europe) logo within the executable
    uint32_t    legalsLogoOffset;   // Offset of the logo containing PSX Doom legal/copyright info within the executable
};

// Embedded boot executable logos for all variants of PlayStation Doom
static constexpr PsxDoomBootExeLogos PSX_DOOM_BOOT_EXE_LOGOS[] = {
    { 0xB1B1457E43C6948E, 0xD4EC1D3C10F0358F, 0x1970C, 0x29A10, 0x39D14 },      // Doom 1.0: US (SLUS_000.77) (Original US PSX Doom release)
    { 0x5EC83BB625405725, 0xCA3067301CF27FEE, 0x1970C, 0x29A10, 0x39D14 },      // Doom 1.1: US (SLUS_000.77) (US 'Greatest Hits' re-release)
    { 0xFF0E934DE5BFA36E, 0x40841F9052CE40A3, 0x19758, 0x29A5C, 0x39D60 },      // Doom 1.1: Europe (SLES_001.32) (Original Europe PSX Doom release + 'Platinum' re-release)
    { 0x0444AA3FA1BB09E3, 0x97EE0D65D59DA840, 0x19758, 0x29A5C, 0x39D60 },      // Doom 1.1: Europe (SLES_001.57) (One level demo disc)
    { 0x1641C74D99D15272, 0x705EDAEAB28BFF2A, 0x19704, 0x2980C, 0x39D0C },      // Doom 1.1: Japan (SLPS_003.08) (PSX Doom Japan release)
    { 0x0668FF031942802C, 0xC6384BA037DA257D, 0x1B008, 0x2B30C, 0x3B610 },      // Final Doom: US (SLUS_003.31)
    { 0x28EF0816BB969BC4, 0x87302D5B286BCEC2, 0x1B054, 0x2B358, 0x3B65C },      // Final Doom: Europe (SLES_004.87)
    { 0xE42785D83C5778AD, 0x0B0F01693A83C71C, 0x1B000, 0x2B304, 0x3B608 },      // Final Doom: Japan (SLPS_007.27)
};

//------------------------------------------------------------------------------------------------------------------------------------------
// The MD5 hash of the PSX Doom boot executable (2 64-bit words)
//------------------------------------------------------------------------------------------------------------------------------------------
static uint64_t gPsxDoomBootExeHashWord1;
static uint64_t gPsxDoomBootExeHashWord2;

//------------------------------------------------------------------------------------------------------------------------------------------
// Returns the path (on the game disc) to the executable which is used to boot PSX Doom.
// This executable contains the embedded intro logos and was responsible for playing the intro movie.
// Returns 'nullptr' if this call is not applicable for the current game disc.
//------------------------------------------------------------------------------------------------------------------------------------------
static const char* getPsxDoomBootExePath() noexcept {
    if (Game::gGameType == GameType::Doom) {
        switch (Game::gGameVariant) {
            case GameVariant::NTSC_U:   return "SLUS_000.77";
            case GameVariant::NTSC_J:   return "SLPS_003.08";
            case GameVariant::PAL:      return (Game::gbIsDemoVersion) ? "SLES_001.57" : "SLES_001.32";
        }
    }
    else if (Game::gGameType == GameType::FinalDoom) {
        switch (Game::gGameVariant) {
            case GameVariant::NTSC_U:   return "SLUS_003.31";
            case GameVariant::NTSC_J:   return "SLPS_007.27";
            case GameVariant::PAL:      return "SLES_004.87";
        }
    }
    else if ((Game::gGameType == GameType::Doom_Alpha_0_05) ||
             (Game::gGameType == GameType::Doom_Alpha_0_30) ||
             (Game::gGameType == GameType::Doom_Alpha_0_32))
    {
        return "PSX.EXE";
    }

    return nullptr;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Determines the MD5 hash of the original PSX Doom boot executable.
// If no such executable exists on the disc then the hash is set to '0'.
//------------------------------------------------------------------------------------------------------------------------------------------
static void determinePsxDoomBootExeHash() noexcept {
    const char* const exePath = getPsxDoomBootExePath();

    if (!Utils::getDiscFileMD5Hash(PsxVm::gDiscInfo, PsxVm::gIsoFileSys, exePath, gPsxDoomBootExeHashWord1, gPsxDoomBootExeHashWord2)) {
        gPsxDoomBootExeHashWord1 = 0;
        gPsxDoomBootExeHashWord2 = 0;
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Helper: reads from the specified portion of a file in the CD-ROM image and returns the bytes if suceeded
//------------------------------------------------------------------------------------------------------------------------------------------
static std::unique_ptr<std::byte[]> readFromDiscFile(const char* const path, uint32_t offset, uint32_t size) noexcept {
    return Utils::getDiscFileData(
        PsxVm::gDiscInfo,
        PsxVm::gIsoFileSys,
        path,
        offset,
        (int32_t) size
    ).pBytes;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Helper: decodes a PSX Doom boot executable embedded logo into the specified LogoPlayer::Logo struct.
// Returns 'false' if the operation fails.
//------------------------------------------------------------------------------------------------------------------------------------------
static bool decodePsxDoomBootExeLogo(const uint32_t logoExeOffset, LogoPlayer::Logo& output) noexcept {
    // Read the raw bytes for the logo.
    // There is a 256 entry RGB888 palette followed by an 8-bit image of 256x240 pixels.
    constexpr uint32_t NUM_LOGO_PIXELS = PSX_DOOM_BOOT_LOGO_W * PSX_DOOM_BOOT_LOGO_H;
    constexpr uint32_t LOGO_PALETTE_SIZE = 256 * 3;
    constexpr uint32_t LOGO_SIZE_IN_BYTES = LOGO_PALETTE_SIZE + NUM_LOGO_PIXELS;

    std::unique_ptr<std::byte[]> logoBytes = readFromDiscFile(getPsxDoomBootExePath(), logoExeOffset, LOGO_SIZE_IN_BYTES);

    // If that fails then ensure the output is cleared
    if (!logoBytes) {
        output.pPixels.reset();
        output.width = 0;
        output.height = 0;
        return false;
    }

    // Otherwise allocate the output pixel array and decode the image
    output.pPixels = std::make_unique<uint32_t[]>(PSX_DOOM_BOOT_LOGO_W * PSX_DOOM_BOOT_LOGO_H);

    const uint8_t* const pPalette = reinterpret_cast<const uint8_t*>(logoBytes.get());
    const uint8_t* pPixelIn = reinterpret_cast<const uint8_t*>(logoBytes.get() + LOGO_PALETTE_SIZE);
    uint32_t* pPixelOut = output.pPixels.get();

    for (uint32_t i = 0; i < NUM_LOGO_PIXELS; ++i, ++pPixelIn, ++pPixelOut) {
        // Get the color components of the pixel.
        // Note: if the index is '0' then that is the transparent color, make it black:
        const uint8_t colorIdx = *pPixelIn;

        if (colorIdx == 0) {
            *pPixelOut = 0xFF000000u;   // Special case, transparent pixel but make it black to match the screen clear color...
            continue;
        }

        const uint8_t* const pPaletteEntryRgb = pPalette + colorIdx * 3;
        const uint32_t r = pPaletteEntryRgb[0];
        const uint32_t g = pPaletteEntryRgb[1];
        const uint32_t b = pPaletteEntryRgb[2];

        // Convert to XBGR8888 and save
        *pPixelOut = 0xFF000000u | (b << 16) | (g << 8) | (r);
    }

    return true;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Helper: returns the set of PSX Doom boot executable logos defined for this variant of the game (if any)
//------------------------------------------------------------------------------------------------------------------------------------------
static const PsxDoomBootExeLogos* getPsxDoomBootExeLogos() noexcept {
    for (const PsxDoomBootExeLogos& logos : PSX_DOOM_BOOT_EXE_LOGOS) {
        const bool bBootExeHashMatch = (
            (logos.exeHashWord1 == gPsxDoomBootExeHashWord1) &&
            (logos.exeHashWord2 == gPsxDoomBootExeHashWord2)
        );

        if (bBootExeHashMatch)
            return &logos;
    }

    return nullptr;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Helper: reads and returns the specified WAD file lump.
// Returns an empty byte array on failure.
//------------------------------------------------------------------------------------------------------------------------------------------
static std::vector<std::byte> readWadLump(const WadLumpName lumpName) noexcept {
    // Lookup the lump index and abort if not found
    const int32_t lumpIdx = W_CheckNumForName(lumpName);

    if (lumpIdx < 0)
        return {};

    // Read the lump and return the data
    const WadLump& lump = W_GetLump(lumpIdx);

    if (lump.uncompressedSize <= 0)
        return {};

    std::vector<std::byte> lumpData;
    lumpData.resize((uint32_t) lump.uncompressedSize);
    W_ReadLump(lumpIdx, lumpData.data(), true);
    return lumpData;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Helper: retrieves a PSX format palette (256 16-bit colors) from a specified WAD file lump.
// Returns a null pointer on failure to load the palette.
//------------------------------------------------------------------------------------------------------------------------------------------
static std::unique_ptr<uint16_t[]> readWadPalette(const WadLumpName paletteLumpName, const uint8_t paletteIdx) noexcept {
    // Get the palette lump contents
    std::vector<std::byte> paletteLump = readWadLump(paletteLumpName);

    // Verify this particular palette is in range for the palette lump
    constexpr uint32_t PALETTE_SIZE = 256 * sizeof(uint16_t);
    const uint32_t paletteOffset = paletteIdx * PALETTE_SIZE;

    if (paletteOffset + PALETTE_SIZE > paletteLump.size())
        return nullptr;

    // Read and return the palette
    std::unique_ptr<uint16_t[]> palette = std::make_unique<uint16_t[]>(256);
    std::memcpy(palette.get(), paletteLump.data() + paletteOffset, PALETTE_SIZE);
    return palette;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Helper: decodes an intro logo from currently loaded WAD files using the specified palette
//------------------------------------------------------------------------------------------------------------------------------------------
static LogoPlayer::Logo decodeWadLogo(const WadLumpName logoLumpName, const uint16_t* const pPalette) noexcept {
    // If no palette is given then no logo can be decoded!
    if (!pPalette)
        return {};

    // Get the lump data for the image and verify that the lump header can be extacted from that
    std::vector<std::byte> logoLumpBytes = readWadLump(logoLumpName);
    
    if (logoLumpBytes.size() < sizeof(texlump_header_t))
        return {};

    // Get the dimensions of the image and verify they are OK.
    // Also verify there are enough bytes of data to read the entire image.
    texlump_header_t texInfo;
    std::memcpy(&texInfo, logoLumpBytes.data(), sizeof(texlump_header_t));

    if ((texInfo.width <= 0) || (texInfo.height <= 0))
        return {};

    const uint32_t numPixels = (uint32_t) texInfo.width * texInfo.height;
    const uint32_t expectedLumpSize = numPixels + (uint32_t) sizeof(texlump_header_t);

    if (logoLumpBytes.size() < expectedLumpSize)
        return {};

    // Save the image dimensions and decode the pixels
    LogoPlayer::Logo logo = {};
    logo.width = texInfo.width;
    logo.height = texInfo.height;
    logo.pPixels = std::make_unique<uint32_t[]>(numPixels);

    const uint8_t* const pSrcPixels = reinterpret_cast<uint8_t*>(logoLumpBytes.data() + sizeof(texlump_header_t));
    uint32_t* const pDstPixels = logo.pPixels.get();

    for (uint32_t pixelIdx = 0; pixelIdx < numPixels; ++pixelIdx) {
        // Get the source color in PlayStation 'T1B5G5R5' format
        const uint8_t colorIdx = pSrcPixels[pixelIdx];
        const uint16_t srcColor = pPalette[colorIdx];

        // Convert to 'X8B8G8R8' and save the pixel
        const uint32_t r8 = (uint32_t)((srcColor >> 0 ) & 0x1Fu) << 3;
        const uint32_t g8 = (uint32_t)((srcColor >> 5 ) & 0x1Fu) << 3;
        const uint32_t b8 = (uint32_t)((srcColor >> 10) & 0x1Fu) << 3;
        const uint32_t dstColor = 0xFF000000u | (b8 << 16) | (g8 << 8) | r8;

        pDstPixels[pixelIdx] = dstColor;
    }

    return logo;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Gets the Sony intro logo for Alpha 0.05
//------------------------------------------------------------------------------------------------------------------------------------------
static LogoPlayer::Logo getSonyLogo_Alpha_0_05() noexcept {
    ASSERT(Game::gGameType == GameType::Doom_Alpha_0_05);

    // Read the raw bytes for the logo, which is a 256x256 image @ 8bpp.
    // I don't know where the palette for this logo is stored so I will hard code instead.
    constexpr uint32_t NUM_LOGO_PIXELS = 256 * 256;
    std::unique_ptr<std::byte[]> rawLogoBytes = readFromDiscFile(getPsxDoomBootExePath(), 0x2008, NUM_LOGO_PIXELS);

    if (rawLogoBytes == nullptr)
        return {};

    // Define the palette for the logo in XBGR8888 format. Just needs 5 entries but allocate 256 to be safe:
    const uint32_t logoPalette[256] = {
        0xFF000000,
        0xFF313131,
        0xFF6B6B6B,
        0xFFB5B5B5,
        0xFFEFEFEF
    };

    // Convert the logo from 8 to 32-bit:
    std::unique_ptr<uint32_t[]> rawLogo32bpp = std::make_unique<uint32_t[]>(NUM_LOGO_PIXELS);
    ImageOps::Convert8bppTo32bit(
        reinterpret_cast<const uint8_t*>(rawLogoBytes.get()),
        rawLogo32bpp.get(),
        256,
        256,
        logoPalette
    );

    // We only want a small region of this image, the top left 167x73 rectangle.
    // That gets blitted into a 320x224 destination image to form the logo which is actually displayed.
    LogoPlayer::Logo logo = {};
    logo.pPixels = std::make_unique<uint32_t[]>(320 * 224);
    logo.width = 320;
    logo.height = 224;
    logo.holdTime = 3.0f;
    logo.fadeOutTime = 0.5f;

    std::memset(logo.pPixels.get(), 0, logo.width * logo.height * sizeof(uint32_t));    // Clear the entire image before the blit

    ImageOps::Blit(
        ImageOps::Image<uint32_t>{ rawLogo32bpp.get(), 256, 256, 256 },
        ImageOps::Image<uint32_t>{ logo.pPixels.get(), logo.width, logo.height, logo.width },
        ImageOps::Rect{{ 0, 0 }, { 167, 73 }},
        ImageOps::Vec2i{ 76, 65 } // Centered horizontally, but slightly more to the top vertically
    );

    return logo;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Gets the Sony intro logo for Alpha 0.30/0.32
//------------------------------------------------------------------------------------------------------------------------------------------
static LogoPlayer::Logo getSonyLogo_Alpha_0_30_0_32() noexcept {
    ASSERT((Game::gGameType == GameType::Doom_Alpha_0_30) || (Game::gGameType == GameType::Doom_Alpha_0_32));

    // Read the raw bytes for the logo, which is a 220x44 image @ 8bpp.
    // I don't know where the palette for this logo is stored so I will hard code instead.
    constexpr uint32_t RAW_LOGO_W = 220;
    constexpr uint32_t RAW_LOGO_H = 44;
    constexpr uint32_t NUM_LOGO_PIXELS = RAW_LOGO_W * RAW_LOGO_H;
    std::unique_ptr<std::byte[]> rawLogoBytes = readFromDiscFile(getPsxDoomBootExePath(), 0x2500, NUM_LOGO_PIXELS);

    if (rawLogoBytes == nullptr)
        return {};

    // Define the palette for the logo in XBGR8888 format. Just needs 6 entries but allocate 256 to be safe:
    const uint32_t logoPalette[256] = {
        0xFF000000,
        0xFF000000,
        0xFF313131,
        0xFF6B6B6B,
        0xFFB5B5B5,
        0xFFEFEFEF,
    };

    // Convert the logo from 8 to 32-bit:
    std::unique_ptr<uint32_t[]> rawLogo32bpp = std::make_unique<uint32_t[]>(NUM_LOGO_PIXELS);
    ImageOps::Convert8bppTo32bit(
        reinterpret_cast<const uint8_t*>(rawLogoBytes.get()),
        rawLogo32bpp.get(),
        RAW_LOGO_W,
        RAW_LOGO_H,
        logoPalette
    );

    // Now blit parts of the SCEA logo onto a 320x240 image.
    // In the image we read from the boot EXE there are 3 rows of text, each 14px tall with 1px of padding in-between.
    // The 1st row says "Published by", 2nd row says "Sony Computer Entertainment of" and the 3rd row says "America Europe".
    // Makeup the logo image by blitting the first line, and the 2nd line made up of "Sony Computer Entertainment of America".
    // Center both lines in the output image.
    LogoPlayer::Logo logo = {};
    logo.pPixels = std::make_unique<uint32_t[]>(320 * 240);
    logo.width = 320;
    logo.height = 240;
    logo.holdTime = 3.0f;
    logo.fadeOutTime = 0.5f;

    std::memset(logo.pPixels.get(), 0, logo.width * logo.height * sizeof(uint32_t));    // Clear the entire image before the blit
    
    ImageOps::Blit(
        // Blit "Published by"
        ImageOps::Image<uint32_t>{ rawLogo32bpp.get(), RAW_LOGO_W, RAW_LOGO_H, RAW_LOGO_W },
        ImageOps::Image<uint32_t>{ logo.pPixels.get(), logo.width, logo.height, logo.width },
        ImageOps::Rect{{ 0, 0 }, { 86, 14 }},
        ImageOps::Vec2i{ 117, 68 }
    );
    
    ImageOps::Blit(
        // Blit "Sony Computer Entertainment of"
        ImageOps::Image<uint32_t>{ rawLogo32bpp.get(), RAW_LOGO_W, RAW_LOGO_H, RAW_LOGO_W },
        ImageOps::Image<uint32_t>{ logo.pPixels.get(), logo.width, logo.height, logo.width },
        ImageOps::Rect{{ 0, 15 }, { 220, 14 }},
        ImageOps::Vec2i{ 20, 82 }
    );
    
    ImageOps::Blit(
        // Blit "America"
        ImageOps::Image<uint32_t>{ rawLogo32bpp.get(), RAW_LOGO_W, RAW_LOGO_H, RAW_LOGO_W },
        ImageOps::Image<uint32_t>{ logo.pPixels.get(), logo.width, logo.height, logo.width },
        ImageOps::Rect{{ 0, 30 }, { 57, 14 }},
        ImageOps::Vec2i{ 243, 82 }
    );

    return logo;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Helper: gets the Sony intro logo for the current variant of the game by extracting it from the boot executable
//------------------------------------------------------------------------------------------------------------------------------------------
static LogoPlayer::Logo getSonyLogo_FromBootExe() noexcept {
    // Only if there are embedded logos defined for this version of the game
    const PsxDoomBootExeLogos* const pLogos = getPsxDoomBootExeLogos();

    if (pLogos == nullptr) {
        // Other special cases:
        if (Game::gGameType == GameType::Doom_Alpha_0_05)
            return getSonyLogo_Alpha_0_05();

        if ((Game::gGameType == GameType::Doom_Alpha_0_30) || (Game::gGameType == GameType::Doom_Alpha_0_32))
            return getSonyLogo_Alpha_0_30_0_32();

        return {};
    }

    // This logo is only used for the US and Europe editions of PSX Doom.
    // The Japanese version of the game doesn't show any Sony specific logo:
    const bool bGameVariantHasSonyLogo = (
        (Game::gGameVariant == GameVariant::NTSC_U) ||
        (Game::gGameVariant == GameVariant::PAL)
    );

    if (!bGameVariantHasSonyLogo)
        return {};

    // Read the logo and define it's parameters
    LogoPlayer::Logo logo = {};
    const uint32_t logoExeOffset = (Game::gGameVariant == GameVariant::NTSC_U) ? pLogos->sceaLogoOffset : pLogos->sceeLogoOffset;

    if (!decodePsxDoomBootExeLogo(logoExeOffset, logo))
        return {};

    logo.width = 256;
    logo.height = 240;
    logo.holdTime = 3.0f;
    logo.fadeOutTime = 0.5f;
    return logo;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Helper: gets the Sony intro logo for a 'GEC Master Edition' disc
//------------------------------------------------------------------------------------------------------------------------------------------
static LogoPlayer::Logo getSonyLogo_GEC_ME() noexcept {
    LogoPlayer::Logo logo = decodeWadLogo("SONY", readWadPalette("GECINPAL", 3).get());
    logo.holdTime = 3.0f;
    logo.fadeOutTime = 0.5f;
    return logo;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Helper: gets the single PSX Doom legals/copyright intro logo by extracting it from the boot executable
//------------------------------------------------------------------------------------------------------------------------------------------
static LogoList getLegalLogos_FromBootExe() noexcept {
    // Only if there are embedded logos defined for this version of the game
    const PsxDoomBootExeLogos* const pLogos = getPsxDoomBootExeLogos();

    if (pLogos == nullptr)
        return {};

    // Read the single legals logo used for this game and define it's display settings
    LogoList logoList = {};
    LogoPlayer::Logo& logo = logoList.logos[0];

    if (!decodePsxDoomBootExeLogo(pLogos->legalsLogoOffset, logo))
        return {};

    logo.width = 256;
    logo.height = 240;
    logo.holdTime = 3.5f;
    return logoList;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Produces the textual legal logo for alpha builds of PSX Doom.
// This legal logo is produced programmatically.
//------------------------------------------------------------------------------------------------------------------------------------------
static LogoPlayer::Logo getDoomAlphaTextualLegalLogo(const std::string_view versionText) noexcept {
    LogoPlayer::Logo logo = {};
    logo.pPixels = std::make_unique<uint32_t[]>(256 * 240);
    logo.width = 256;
    logo.height = 240;
    logo.holdTime = 3.5f;
    
    ImageOps::Image<uint32_t> logoImg = {};
    logoImg.pPixels = logo.pPixels.get();
    logoImg.width = logo.width;
    logoImg.height = logo.height;
    logoImg.rowStridePx = logo.width;
    
    std::memset(logoImg.pPixels, 0, sizeof(uint32_t) * 256 * 240); // Clear the image to black
    
    const std::string_view logoLine1("SONY PSX DOOM");
    const std::string_view logoLine2("(C) WILLIAMS ENTERTAINMENT");
    const std::string_view logoLine3("LICENSED FROM ID SOFTWARE");
    const std::string_view logoLine4(versionText);
    constexpr uint32_t FONT_COLOR = 0xFFFFFFFF;
 
    ImageOps::DebugPrint(logoLine1.data(), logoLine1.length(), 66, 17, FONT_COLOR, logoImg);
    ImageOps::DebugPrint(logoLine2.data(), logoLine2.length(), 10, 25, FONT_COLOR, logoImg);
    ImageOps::DebugPrint(logoLine3.data(), logoLine3.length(), 18, 33, FONT_COLOR, logoImg);
    ImageOps::DebugPrint(logoLine4.data(), logoLine4.length(), 50, 49, FONT_COLOR, logoImg);
    
    return logo;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Gets the legal logos for PSX Doom Alpha 0.05
//------------------------------------------------------------------------------------------------------------------------------------------
static LogoList getLegalLogos_Doom_Alpha_0_05() noexcept {
    LogoList logoList = {};
    logoList.logos[0] = getDoomAlphaTextualLegalLogo("ALPHA VERSION 0.05");
    return logoList;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Gets the legal logos for PSX Doom Alpha 0.30
//------------------------------------------------------------------------------------------------------------------------------------------
static LogoList getLegalLogos_Doom_Alpha_0_30() noexcept {
    // Note: this is slightly different to the actual Alpha 0.30.
    // In the real alpha the message with the "ALPHA VERSION 0.30" text is printed on top of the "LEGALS" logo, but its very hard to read.
    // Separate this message out in PsyDoom, so we can make it easier to see. Also helps to make some Alpha 0.05 code reusable:
    LogoList logoList = {};
    logoList.logos[0] = getDoomAlphaTextualLegalLogo("ALPHA VERSION 0.30");
    logoList.logos[1] = decodeWadLogo("LEGALS", readWadPalette("PLAYPAL", 0).get());
    logoList.logos[1].holdTime = 3.5f;
    return logoList;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Gets the legal logos for PSX Doom Alpha 0.32
//------------------------------------------------------------------------------------------------------------------------------------------
static LogoList getLegalLogos_Doom_Alpha_0_32() noexcept {
    // Note: this is slightly different to the actual Alpha 0.32.
    // In the real alpha the message with the "ALPHA VERSION 0.32" text is printed on top of the "LEGALS" logo, but its very hard to read.
    // Also the 'LEGALS' logo scrolls upwards...
    // Separate this message out in PsyDoom, so we can make it easier to see. Also helps to make some Alpha 0.05 code reusable:
    LogoList logoList = {};
    logoList.logos[0] = getDoomAlphaTextualLegalLogo("ALPHA VERSION 0.32");
    logoList.logos[1] = decodeWadLogo("LEGALS", readWadPalette("PLAYPAL", 0).get());
    logoList.logos[1].holdTime = 3.5f;
    return logoList;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Helper: gets the legals/copyright intro logos for a 'GEC Master Edition' disc
//------------------------------------------------------------------------------------------------------------------------------------------
static LogoList getLegalLogos_GEC_ME() noexcept {
    LogoList logoList = {};
    logoList.logos[0] = decodeWadLogo("LEGALS", readWadPalette("GECINPAL", 3).get());
    logoList.logos[1] = decodeWadLogo("LEGALS2", readWadPalette("GECINPAL", 3).get());

    for (LogoPlayer::Logo& logo : logoList.logos) {
        logo.holdTime = 3.5f;
    }

    return logoList;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Initializes the intro logos module
//------------------------------------------------------------------------------------------------------------------------------------------
void init() noexcept {
    determinePsxDoomBootExeHash();
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Does cleanup/teardown for the intro logos module
//------------------------------------------------------------------------------------------------------------------------------------------
void shutdown() noexcept {
    gPsxDoomBootExeHashWord1 = {};
    gPsxDoomBootExeHashWord2 = {};
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Helper: gets the Sony intro logo for the current game
//------------------------------------------------------------------------------------------------------------------------------------------
LogoPlayer::Logo getSonyLogo() noexcept {
    if (Game::isGameTypeGecMe()) {
        return getSonyLogo_GEC_ME();
    } else {
        return getSonyLogo_FromBootExe();
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Helper: gets the PSX Doom legals/copyright intro logos for the current game
//------------------------------------------------------------------------------------------------------------------------------------------
LogoList getLegalLogos() noexcept {
    if (Game::isGameTypeGecMe()) {
        return getLegalLogos_GEC_ME();
    }
    else if (Game::gGameType == GameType::Doom_Alpha_0_05) {
        return getLegalLogos_Doom_Alpha_0_05();
    }
    else if (Game::gGameType == GameType::Doom_Alpha_0_30) {
        return getLegalLogos_Doom_Alpha_0_30();
    }
    else if (Game::gGameType == GameType::Doom_Alpha_0_32) {
        return getLegalLogos_Doom_Alpha_0_32();
    }
    else {
        return getLegalLogos_FromBootExe();
    }
}

END_NAMESPACE(IntroLogos)
