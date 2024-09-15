#include "WavReader.h"

#include "Asserts.h"
#include "ByteInputStream.h"
#include "FileUtils.h"
#include "InputStream.h"

BEGIN_NAMESPACE(WavReader)

//------------------------------------------------------------------------------------------------------------------------------------------
// Wav file format related definitions
//------------------------------------------------------------------------------------------------------------------------------------------

// Header for all RIFF chunks and sub-chunks in the file
struct RiffChunk {
    char        chunkId[4];
    uint32_t    chunkSize;

    // Check if the chunk has a particular id
    bool checkChunkId(const char c1, const char c2, const char c3, const char c4) const noexcept {
        return ((chunkId[0] == c1) && (chunkId[1] == c2) && (chunkId[2] == c3) && (chunkId[3] == c4));
    };
};

static_assert(sizeof(RiffChunk) == 8);

// Wave file format tags
enum WaveFmtTag : uint16_t {
    FORMAT_PCM  = 0x0001,           // PCM: the only format supported by PsyDoom
    FORMAT_IEEE_FLOAT = 0x0003,     // Floating point format
    FORMAT_ALAW = 0x0006,           // A-law encoded
    FORMAT_MULAW = 0x0007,          // U-law encoded
    FORMAT_EXTENSIBLE = 0xFFFE,     // Some other format
};

// Contains the standard/base data that all format chunk variants contain.
// This is also the exact format chunk data for the 'PCM' format, which is all we are interested in.
// There are other extensions (additional fields) for non-PCM formats and 'extensible' format wave files, but we don't need those!
struct FormatChunkBaseData {
    WaveFmtTag  formatCode;         // Format type
    uint16_t    numChannels;        // Number of channels
    uint32_t    sampleRate;         // Samples per second (Hz)
    uint32_t    avgBytesPerSec;     // Average bytes per second data rate
    uint16_t    dataBlockAlign;     // Alignment helper for the data (can be ignored)
    uint16_t    bitsPerSample;      // Bit rate per sample (Note: PsyDoom only supports 16-bit!)
};

//------------------------------------------------------------------------------------------------------------------------------------------
// Helper: tries to find the next RIFF chunk of the specified type, determined by a predicate.
// Skips over all other chunks until it finds the desired chunk.
// Returns a valid 'ByteInputStream' to the chunk's data if a matching RIFF chunk was found.
//------------------------------------------------------------------------------------------------------------------------------------------
ByteInputStream findNextRiffChunk(
    ByteInputStream& input,
    bool (* const chunkMatchPredicate)(const RiffChunk& chunk, ByteInputStream chunkData) noexcept
) noexcept
{
    ASSERT(chunkMatchPredicate);

    while (!input.isAtEnd()) {
        // Read the header for the chunk: if there is not enough bytes left then abort the search.
        const RiffChunk chunk = input.read<RiffChunk>();

        if (chunk.chunkSize > input.bytesLeft())
            break;

        // Does the chunk match what we are looking for? If so then return the stream for the chunk's data.
        const ByteInputStream chunkDataStream(input.data() + input.tell(), chunk.chunkSize);
        const bool bIsMatchingChunk = chunkMatchPredicate(chunk, ByteInputStream(chunkDataStream)); // Note: provide a copy of the stream which can be freely altered if needed.

        if (bIsMatchingChunk)
            return chunkDataStream;

        // Otherwise skip the data for the chunk
        input.skipBytes(chunk.chunkSize);
    }

    // Found no matching chunk if we get to here
    return ByteInputStream();
}

// This version doesn't modify the input stream
ByteInputStream findNextRiffChunk(
    const ByteInputStream& input,
    bool (* const chunkMatchPredicate)(const RiffChunk& chunk, ByteInputStream chunkData) noexcept
) noexcept
{
    ByteInputStream inputCopy = input;
    return findNextRiffChunk(inputCopy, chunkMatchPredicate);
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Chunk matching predicate: matches 'RIFF' chunks with a 'WAVE' form type
//------------------------------------------------------------------------------------------------------------------------------------------
static bool isRiffWaveChunk(const RiffChunk& chunk, ByteInputStream chunkData) noexcept {
    if (chunk.checkChunkId('R', 'I', 'F', 'F')) {
        // It's a 'RIFF' chunk, but is it of form type 'WAVE'?
        char formType[4];
        chunkData.readArray(formType, 4);
        const bool bIsWaveFormType = ((formType[0] == 'W') && (formType[1] == 'A') && (formType[2] == 'V') && (formType[3] == 'E'));

        if (bIsWaveFormType)
            return true;
    }

    return false;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Chunk matching predicates: check for 'fmt ' and 'data' chunks
//------------------------------------------------------------------------------------------------------------------------------------------
static bool isRiffFmtChunk(const RiffChunk& chunk, ByteInputStream chunkData) noexcept {
    return chunk.checkChunkId('f', 'm', 't', ' ');
}

static bool isRiffDataChunk(const RiffChunk& chunk, ByteInputStream chunkData) noexcept {
    return chunk.checkChunkId('d', 'a', 't', 'a');
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Read the audio data from a .wav file which is 16-bit PCM encoded.
// If the file is in another format then reading will fail.
//------------------------------------------------------------------------------------------------------------------------------------------
AudioData readWav_Pcm16(const std::byte* const pBytes, const size_t numBytes) noexcept {
    // No data?
    ASSERT((numBytes > 0) || (pBytes == nullptr));

    if ((pBytes == nullptr) || (numBytes <= 0))
        return {};

    try {
        // Find a 'RIFF' chunk with form type 'WAVE':
        const ByteInputStream fileInputStream = ByteInputStream(pBytes, numBytes);
        ByteInputStream waveChunkData = findNextRiffChunk(ByteInputStream(fileInputStream), isRiffWaveChunk);
        waveChunkData.skipBytes(4); // Skip past the 4-byte form type (which has the value 'WAVE')

        // Find the 'fmt' chunk and read the 'FormatChunkBaseData' from it:
        const ByteInputStream fmtChunkData = findNextRiffChunk(waveChunkData, isRiffFmtChunk);
        const FormatChunkBaseData fmt = fmtChunkData.peek<FormatChunkBaseData>();

        // Verify the format is what we support
        const bool bValidFmt = (
            (fmt.formatCode == WaveFmtTag::FORMAT_PCM) && 
            (fmt.numChannels >= 1) &&
            (fmt.numChannels <= 15) &&
            (fmt.sampleRate > 0) &&
            (fmt.bitsPerSample == 16) // PsyDoom only supports 16-bit audio!
        );

        if (!bValidFmt)
            return {};

        // Find the 'data' chunk and check that it is not too big:
        ByteInputStream dataChunkData = findNextRiffChunk(waveChunkData, isRiffDataChunk);

        if (dataChunkData.size() >= UINT32_MAX) 
            return {};

        // Read and return the data for the audio
        AudioData audioData = {};
        audioData.sampleRate = fmt.sampleRate & 0x0FFFFFFF;
        audioData.numChannels = fmt.numChannels & 0xF;
        audioData.numSamples = (uint32_t)(dataChunkData.size() / (sizeof(int16_t) * fmt.numChannels));
        audioData.pData = std::make_unique<int16_t[]>((size_t) audioData.numSamples * fmt.numChannels);

        dataChunkData.readArray<int16_t>(audioData.pData.get(), (size_t) audioData.numSamples * fmt.numChannels);
        return audioData;
    }
    catch (...) {
        // A corrupt wav file or some sort of IO error: ignore!
    }

    // If we got here then something went wrong
    return {};
}

END_NAMESPACE(WavReader)
