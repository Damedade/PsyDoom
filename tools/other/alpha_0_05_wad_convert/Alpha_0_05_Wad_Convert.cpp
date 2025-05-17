//------------------------------------------------------------------------------------------------------------------------------------------
// Alpha_0_05_Wad_Convert:
//      Converts .WAD files in the PSX Doom Alpha 0.05 format to the retail format for the original PSX DOOM.
//------------------------------------------------------------------------------------------------------------------------------------------
#include "ByteInputStream.h"
#include "Endian.h"
#include "FileOutputStream.h"
#include "FileUtils.h"

#include <algorithm>
#include <cstring>
#include <cstdint>
#include <optional>
#include <vector>

//------------------------------------------------------------------------------------------------------------------------------------------
// Data structures
//------------------------------------------------------------------------------------------------------------------------------------------

// Header for a WAD file
struct WadHdr {
    char        fileid[4]; // Always "IWAD" for PSX DOOM
    int32_t     numLumps;
    int32_t     lumpHdrsOffset;
};

static_assert(sizeof(WadHdr) == 12);

// Header for a WAD file lump
struct WadLumpHdr {
    uint32_t    wadFileOffset;
    uint32_t    uncompressedSize;
    char        name[8];
};

static_assert(sizeof(WadLumpHdr) == 16);

// A sector in a WAD file: original PSX Doom format
struct mapsector_t {
    int16_t     floorheight;
    int16_t     ceilingheight;
    char        floorpic[8];
    char        ceilingpic[8];
    uint8_t     lightlevel;
    uint8_t     colorid;
    int16_t     special;
    int16_t     tag;
    uint16_t    flags;
};

static_assert(sizeof(mapsector_t) == 28);

// A sector in a WAD file: Doom 0.05 Alpha format
struct mapsector_alpha_0_05_t {
    int16_t     floorheight;
    int16_t     ceilingheight;
    char        floorpic[8];
    char        ceilingpic[8];
    uint8_t     lightlevel;
    uint8_t     colorid;
    int16_t     special;
    int16_t     tag;
};

static_assert(sizeof(mapsector_alpha_0_05_t) == 26);

// Header for the 'LEAFS' WAD lump: original PSX Doom format
struct mapleaf_t {
    uint16_t    numedges;
};

static_assert(sizeof(mapleaf_t) == 2);

// Header for the 'LEAFS' WAD lump: Doom 0.05 Alpha format
struct mapleaf_alpha_0_05_t {
    uint32_t    numedges;
};

static_assert(sizeof(mapleaf_alpha_0_05_t) == 4);

// Definition for an edge in a leaf in the 'LEAFS' lump
struct mapleafedge_t {
    int16_t     vertexnum;
    int16_t     segnum;
};

static_assert(sizeof(mapleafedge_t) == 4);

// Holds the data for the WAD file
struct WadFileData {
    std::vector<WadLumpHdr>     lumpHdrs;
    std::vector<FileData>       lumps;
};

//------------------------------------------------------------------------------------------------------------------------------------------
// Help/usage printing
//------------------------------------------------------------------------------------------------------------------------------------------
static const char* const HELP_STR =
R"(Usage: Alpha_0_05_Wad_Convert <INPUT WAD FILE PATH> <OUTPUT WAD FILE PATH>

Converts .WAD files in the PSX Doom Alpha 0.05 format to the retail format for the original PSX DOOM.
)";

static void printHelp() noexcept {
    std::printf("%s\n", HELP_STR);
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Decompresses the given compressed lump data into the given output buffer.
// The compression algorithm used is a form of LZSS.
// Assumes the output buffer is sized big enough to hold all of the decompressed data.
//------------------------------------------------------------------------------------------------------------------------------------------
void decompressLump(const void* const pSrc, void* const pDst) noexcept {
    const uint8_t* pSrcByte = (const uint8_t*) pSrc;
    uint8_t* pDstByte = (uint8_t*) pDst;

    uint32_t idByte = 0;        // Controls whether there is compressed or uncompressed data ahead
    uint32_t haveIdByte = 0;    // Controls when to read an id byte, when '0' we need to read another one

    while (true) {
        // Read the id byte if required.
        // We need 1 id byte for every 8 bytes of uncompressed output, or every 8 runs of compressed data.
        if (haveIdByte == 0) {
            idByte = *pSrcByte;
            ++pSrcByte;
        }

        haveIdByte = (haveIdByte + 1) & 7;

        if (idByte & 1) {
            // Compressed data ahead: the first 12-bits tells where to take repeated data from.
            // The remaining 4-bits tell how many bytes of repeated data to take.
            const uint32_t srcByte1 = pSrcByte[0];
            const uint32_t srcByte2 = pSrcByte[1];
            pSrcByte += 2;

            const int32_t srcOffset = ((srcByte1 << 4) | (srcByte2 >> 4)) + 1;
            const int32_t numRepeatedBytes = (srcByte2 & 0xF) + 1;

            // A value of '1' is a special value and means we have reached the end of the compressed stream
            if (numRepeatedBytes == 1)
                break;

            const uint8_t* const pRepeatedBytes = pDstByte - srcOffset;

            for (int32_t i = 0; i < numRepeatedBytes; ++i) {
                *pDstByte = pRepeatedBytes[i];
                ++pDstByte;
            }
        } else {
            // Uncompressed data: just copy the input byte
            *pDstByte = *pSrcByte;
            ++pSrcByte;
            ++pDstByte;
        }

        idByte >>= 1;
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Reads and returns the specified WAD file.
// Returns no WAD file on failure.
//------------------------------------------------------------------------------------------------------------------------------------------
std::optional<WadFileData> readWadFile(const char* const filePath) noexcept {
    // Get the raw bytes for the WAD file
    const FileData fileData = FileUtils::getContentsOfFile(filePath);
    
    if ((fileData.bytes == nullptr) || (fileData.size < sizeof(WadHdr)))
        return {};

    // Validate the WAD header is OK
    WadHdr wadHdr;
    std::memcpy(&wadHdr, fileData.bytes.get(), sizeof(WadHdr));

    wadHdr.numLumps = Endian::littleToHost(wadHdr.numLumps);
    wadHdr.lumpHdrsOffset = Endian::littleToHost(wadHdr.lumpHdrsOffset);
        
    const bool bValidHdr = (
        ((wadHdr.fileid[0] = 'I') || (wadHdr.fileid[0] = 'P')) &&
        (wadHdr.fileid[1] = 'W') &&
        (wadHdr.fileid[2] = 'A') &&
        (wadHdr.fileid[3] = 'D')
    );
    
    if (!bValidHdr)
        return {};
        
    // Read the lump headers
    const size_t lumpHdrsSize = wadHdr.numLumps * sizeof(WadLumpHdr);
    
    if (wadHdr.lumpHdrsOffset + lumpHdrsSize > fileData.size)
        return {};
    
    std::vector<WadLumpHdr> lumpHdrs;
    lumpHdrs.resize(wadHdr.numLumps);
    std::memcpy(lumpHdrs.data(), fileData.bytes.get() + wadHdr.lumpHdrsOffset, lumpHdrsSize);
    
    // Sort the lump headers by their offset in the file
    std::sort(
        lumpHdrs.begin(),
        lumpHdrs.end(),
        [](const WadLumpHdr& h1, const WadLumpHdr& h2) { return (h1.wadFileOffset < h2.wadFileOffset); }
    );
    
    // Determine the compressed size of each lump.
    // If a lump is uncompressed then this will be the same as the uncompressed size.
    std::vector<uint32_t> lumpCompressedSizes;
    lumpCompressedSizes.reserve(lumpHdrs.size());
    
    for (size_t lumpIdx = 0; lumpIdx < lumpHdrs.size(); ++lumpIdx) {
        const WadLumpHdr& lumpHdr = lumpHdrs[lumpIdx];
        const bool bIsCompressedLump = (lumpHdr.name[0] & 0x80);
        
        if (!bIsCompressedLump) {
            lumpCompressedSizes.push_back(Endian::littleToHost(lumpHdr.uncompressedSize));
            continue;
        }
        
        uint32_t lumpEndOffset;
        
        if (lumpIdx + 1u < lumpHdrs.size()) {
            lumpEndOffset = lumpHdrs[lumpIdx + 1].wadFileOffset;
        }
        else {
            if (wadHdr.lumpHdrsOffset < lumpHdr.wadFileOffset) {
                // End of lump is end of file
                lumpEndOffset = (uint32_t) fileData.size;
            } else {
                // End of lump is the start of the lump headers (since they are at the end of the file)
                lumpEndOffset = wadHdr.lumpHdrsOffset;
            }
        }
        
        lumpCompressedSizes.push_back(lumpEndOffset - Endian::hostToLittle(lumpHdr.wadFileOffset));
    }
    
    // Read each lump
    std::vector<FileData> lumps;
    lumps.reserve(wadHdr.numLumps);
    
    for (size_t lumpIdx = 0; lumpIdx < lumpHdrs.size(); ++lumpIdx) {
        WadLumpHdr& lumpHdr = lumpHdrs[lumpIdx];
        lumpHdr.wadFileOffset = Endian::littleToHost(lumpHdr.wadFileOffset);
        lumpHdr.uncompressedSize = Endian::littleToHost(lumpHdr.uncompressedSize);

        // Read and save the lump data
        FileData& lumpData = lumps.emplace_back();
        const uint32_t lumpCompressedSize = lumpCompressedSizes[lumpIdx];
        
        if (lumpHdr.wadFileOffset + lumpCompressedSize > fileData.size)
            return {};
            
        lumpData.size = lumpHdr.uncompressedSize;
        
        if (lumpHdr.uncompressedSize > 0) {
            lumpData.bytes = std::make_unique<std::byte[]>(lumpHdr.uncompressedSize);
            const bool bIsLumpCompressed = (lumpHdr.name[0] & 0x80);
            
            if (!bIsLumpCompressed) {
                // Easy case: uncompressed lump
                std::memcpy(lumpData.bytes.get(), fileData.bytes.get() + lumpHdr.wadFileOffset, lumpHdr.uncompressedSize);
            }
            else {
                // Need to decompress the lump
                std::unique_ptr<std::byte[]> compressedData = std::make_unique<std::byte[]>(lumpCompressedSize);
                std::memcpy(compressedData.get(), fileData.bytes.get() + lumpHdr.wadFileOffset, lumpCompressedSize);
                decompressLump(compressedData.get(), lumpData.bytes.get());
                
                // Mark it uncompressed
                lumpHdr.name[0] &= 0x7F;
            }
        }
    }
    
    // Return the WAD data
    WadFileData wadData;
    wadData.lumpHdrs = std::move(lumpHdrs);
    wadData.lumps = std::move(lumps);
    return wadData;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Tells if a WAD lump has the specified name
//------------------------------------------------------------------------------------------------------------------------------------------
static bool lumpNameMatches(const WadLumpHdr& lumpHdr, const char* checkName) noexcept {
    const size_t checkNameLen = std::strlen(checkName);
    size_t lumpNameLen = 0;
    
    for (char c : lumpHdr.name) {
        if (c) {
            lumpNameLen++;
        } else {
            break;
        }
    }
    
    if (checkNameLen != lumpNameLen)
        return false;
        
    return (std::memcmp(lumpHdr.name, checkName, checkNameLen) == 0);
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Convert a 'SECTORS' lump from the PSX Doom Alpha 0.05 format to the retail data format
//------------------------------------------------------------------------------------------------------------------------------------------
static bool transformSectorsLump(FileData& fileData) noexcept {
    try {
        // Setup input data stream
        const FileData inputData = std::move(fileData);
        ByteInputStream bytesIn(inputData.bytes.get(), inputData.size);
        
        // Setup output data block
        const size_t numSectors = inputData.size / sizeof(mapsector_alpha_0_05_t);
        fileData.bytes = std::make_unique<std::byte[]>(numSectors * sizeof(mapsector_t));
        fileData.size = sizeof(mapsector_t) * numSectors;
        
        for (uint32_t sectorIdx = 0; sectorIdx < numSectors; ++sectorIdx) {
            mapsector_alpha_0_05_t alphaSec = {};
            bytesIn.read(alphaSec);
            
            static_assert(C_ARRAY_SIZE(mapsector_alpha_0_05_t::floorpic) == C_ARRAY_SIZE(mapsector_t::floorpic), "Unexpected floorpic name size!");
            static_assert(C_ARRAY_SIZE(mapsector_alpha_0_05_t::ceilingpic) == C_ARRAY_SIZE(mapsector_t::ceilingpic), "Unexpected ceilingpic name size!");
            
            mapsector_t retailSec = {};
            retailSec.floorheight = alphaSec.floorheight;
            retailSec.ceilingheight = alphaSec.ceilingheight;
            std::memcpy(retailSec.floorpic, alphaSec.floorpic, C_ARRAY_SIZE(alphaSec.floorpic));
            std::memcpy(retailSec.ceilingpic, alphaSec.ceilingpic, C_ARRAY_SIZE(alphaSec.ceilingpic));
            retailSec.lightlevel = alphaSec.lightlevel;
            retailSec.colorid = alphaSec.colorid;
            retailSec.special = alphaSec.special;
            retailSec.tag = alphaSec.tag;
            retailSec.flags = 0; // Not in Alpha 0.05 sector!
            
            std::memcpy(fileData.bytes.get() + sectorIdx * sizeof(mapsector_t), &retailSec, sizeof(mapsector_t));
        }
        
        return true;
    }
    catch (...) {
        return false;
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Convert a 'LEAFS' lump from the PSX Doom Alpha 0.05 format to the retail data format
//------------------------------------------------------------------------------------------------------------------------------------------
static bool transformLeafsLump(FileData& fileData) noexcept {
    try {
        // Setup input data stream
        const FileData inputData = std::move(fileData);
        ByteInputStream bytesIn(inputData.bytes.get(), inputData.size);
        
        // Store output here
        std::vector<mapleaf_t> leafs;
        std::vector<mapleafedge_t> leafEdges;
        
        // Convert each leaf
        while (!bytesIn.isAtEnd()) {
            mapleaf_alpha_0_05_t alphaLeaf = bytesIn.read<mapleaf_alpha_0_05_t>();
            alphaLeaf.numedges = Endian::littleToHost(alphaLeaf.numedges);
            
            mapleaf_t& retailLeaf = leafs.emplace_back();
            retailLeaf.numedges = (uint16_t) alphaLeaf.numedges;
            
            for (uint32_t edgeIdx = 0; edgeIdx < alphaLeaf.numedges; ++edgeIdx) {
                leafEdges.emplace_back(bytesIn.read<mapleafedge_t>());
            }
            
            // Note: Doom 0.05 Alpha always has a minimum of 18 edges allocated per leaf.
            // Skip past the padding if we are using less than that.
            if (alphaLeaf.numedges < 18) {
                bytesIn.skipBytes(sizeof(mapleafedge_t) * (18u - alphaLeaf.numedges));
            }
        }
        
        // Make up the binary data block for the retail leafs
        fileData.size = sizeof(mapleaf_t) * leafs.size() + sizeof(mapleafedge_t) * leafEdges.size();
        fileData.bytes = std::make_unique<std::byte[]>(fileData.size);
        std::byte* pCurOutputBytes = fileData.bytes.get();
        const mapleafedge_t* pCurLeafEdge = leafEdges.data();
        
        for (mapleaf_t& leaf : leafs) {
            const uint32_t numLeafEdges = leaf.numedges;
        
            leaf.numedges = Endian::hostToLittle(numLeafEdges);
            std::memcpy(pCurOutputBytes, &leaf, sizeof(mapleaf_t));
            pCurOutputBytes += sizeof(mapleaf_t);
            
            std::memcpy(pCurOutputBytes, pCurLeafEdge, sizeof(mapleafedge_t) * numLeafEdges);
            pCurOutputBytes += sizeof(mapleafedge_t) * numLeafEdges;
            pCurLeafEdge += numLeafEdges;
        }
        
        return true;
    }
    catch (...) {
        return false;
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Transforms a lump from the PSX Doom Alpha 0.05 data format to the retail data format.
// Returns 'false' on failure.
//------------------------------------------------------------------------------------------------------------------------------------------
static bool transformLump(WadLumpHdr& lumpHdr, FileData& lumpData) noexcept {
    if (lumpNameMatches(lumpHdr, "SECTORS"))
        return transformSectorsLump(lumpData);

    if (lumpNameMatches(lumpHdr, "LEAFS"))
        return transformLeafsLump(lumpData);

    return true; // No transformation needed!
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Program entrypoint
//------------------------------------------------------------------------------------------------------------------------------------------
int main(int argc, const char* const argv[]) noexcept {
    // Not enough arguments?
    if (argc != 3) {
        printHelp();
        return 1;
    }
    
    // Read the input WAD file data
    std::optional<WadFileData> wadData = readWadFile(argv[1]);
    
    if (!wadData.has_value()) {
        std::printf("Failed to read input WAD file '%s'! WAD file might not exist, or be in a valid format.\n", argv[1]);
        return 1;
    }
    
    // Transform various lumps that need transforming
    for (size_t lumpIdx = 0; lumpIdx < wadData->lumps.size(); ++lumpIdx) {
        WadLumpHdr& lumpHdr = wadData->lumpHdrs[lumpIdx];
        FileData& lumpData = wadData->lumps[lumpIdx];
        
        if (!transformLump(lumpHdr, lumpData)) {
            std::printf("Failed to convert input WAD file '%s'! WAD file might be in an invalid format.\n", argv[1]);
            return 1;
        }
    }
    
    // Set the offset and uncompressed size of each lump in the WAD file (in little endian format)
    {
        uint32_t lumpOffset = (uint32_t)(sizeof(WadHdr) + wadData->lumpHdrs.size() * sizeof(WadLumpHdr));
        
        for (size_t lumpIdx = 0; lumpIdx < wadData->lumps.size(); ++lumpIdx) {
            const FileData& lumpData = wadData->lumps[lumpIdx];

            WadLumpHdr& lumpHdr = wadData->lumpHdrs[lumpIdx];
            lumpHdr.wadFileOffset = Endian::hostToLittle(lumpOffset);
            lumpHdr.uncompressedSize = Endian::hostToLittle((uint32_t) lumpData.size);
            lumpOffset += (uint32_t) lumpData.size;
        }
    }
    
    // Write the output file
    try {
        FileOutputStream fileOut(argv[2], false);
        
        // Write the header
        WadHdr wadHdr = {};
        wadHdr.fileid[0] = 'I';
        wadHdr.fileid[1] = 'W';
        wadHdr.fileid[2] = 'A';
        wadHdr.fileid[3] = 'D';
        wadHdr.numLumps = Endian::hostToLittle((uint32_t) wadData->lumpHdrs.size());
        wadHdr.lumpHdrsOffset = Endian::hostToLittle((uint32_t) sizeof(WadHdr));
        
        fileOut.write(wadHdr);
        
        // Write the lump headers
        fileOut.writeArray(wadData->lumpHdrs.data(), wadData->lumpHdrs.size());
        
        // Write each of the lumps
        for (const FileData& fileData : wadData->lumps) {
            fileOut.writeBytes(fileData.bytes.get(), fileData.size);
        }
    }
    catch (...) {
        std::printf("Failed to write the output WAD file '%s'!\n", argv[2]);
        return 1;
    }

    return 0;
}

