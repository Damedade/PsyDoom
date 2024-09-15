#pragma once

#include <cstdint>
#include <memory>

//------------------------------------------------------------------------------------------------------------------------------------------
// Simple container for a block of uncompressed audio data (PCM format, signed 16-bit little-endian)
//------------------------------------------------------------------------------------------------------------------------------------------
struct AudioData {
    uint32_t sampleRate  : 28;  // Sample rate in Hz: 44100 is CD quality.
    uint32_t numChannels : 4;   // The number of channels in the audio data: should be '1' or '2'.
    uint32_t numSamples;        // Number of samples per-channel.

    // The raw data for the audio in 16-bit format
    std::unique_ptr<int16_t[]> pData;
};
