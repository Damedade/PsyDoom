//------------------------------------------------------------------------------------------------------------------------------------------
// Utilities for sampling audio data
//------------------------------------------------------------------------------------------------------------------------------------------
#include "AudioSampler.h"

#include "Asserts.h"
#include "AudioData.h"
#include "Endian.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <type_traits>

BEGIN_NAMESPACE(AudioSampler)

//------------------------------------------------------------------------------------------------------------------------------------------
// Helper type alias for mono or stereo samples in 'int16_t' or 'float' format
//------------------------------------------------------------------------------------------------------------------------------------------
template <bool STEREO, class DataT> 
using SampleT = std::conditional_t<STEREO, std::tuple<DataT, DataT>, DataT>;

//------------------------------------------------------------------------------------------------------------------------------------------
// Converts a 16-bit audio sample to floating point
//------------------------------------------------------------------------------------------------------------------------------------------
static float toFloatSample(const int16_t sample) noexcept {
    return (float) sample * (1.0f / 32768.0f);
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Converts a floating point audio sample to 16-bit
//------------------------------------------------------------------------------------------------------------------------------------------
static int16_t toInt16Sample(const float sample) noexcept {
    return (int16_t) std::clamp(sample * 32768.0f, float(INT16_MIN), float(INT16_MAX));
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Helper: does cubic interpolation for the specified set of samples.
// The 't' value should be between '0' and '1'.
// 
// For more info, see: https://www.paulinternet.nl/?page=bicubic
//------------------------------------------------------------------------------------------------------------------------------------------
static Spu::Sample cubicInterpolateAudioSample(
    const float s0,
    const float s1,
    const float s2,
    const float s3,
    const float t
) noexcept
{
    const float a1 = ((s1 - s2) * 3.0f + s3 - s0) * t;
    const float a2 = (s0 * 2.0f - s1 * 5.0f + s2 * 4.0f - s3 + a1) * t;
    const float a3 = (s2 - s0 + a2) * (t * 0.5f);
    const float interpolated = s1 + a3;

    return interpolated;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Reads a single raw/unfiltered mono or stereo sample from the given audio data.
// Assumes the audio data is valid.
//------------------------------------------------------------------------------------------------------------------------------------------
template <bool STEREO>
static SampleT<STEREO, int16_t> rawSample_i16(const AudioData& audioData, const int64_t sampleIdx, const bool bLoop) noexcept {
    // Grab the info we need from the audio block and validate
    const int16_t* const pSampleData = audioData.pData.get();
    const uint32_t numChannels = audioData.numChannels;
    [[maybe_unused]] const uint32_t numSamples = audioData.numSamples;

    ASSERT(pSampleData);
    ASSERT(numChannels >= 1);
    ASSERT(numSamples >= 1);

    // Determine which sample to read
    uint64_t readSampleIdx;

    if (bLoop) {
        readSampleIdx = (uint64_t) sampleIdx % audioData.numSamples;
    } else {
        readSampleIdx = (uint64_t) std::clamp(sampleIdx, (int64_t) 0, (int64_t) audioData.numSamples - 1);
    }

    // Do the sampling!
    const int16_t* const pSamples = &pSampleData[readSampleIdx * numChannels];

    if constexpr (STEREO) {
        // Stereo sample requested
        if (numChannels >= 2) {
            // Happy case: sample is already stereo (if there are more channels, ignore)
            return { pSamples[0], pSamples[1] };
        }
        else {
            // Duplicate mono sample across both channels
            const int16_t monoSample = pSamples[0];
            return { monoSample, monoSample };
        }
    }
    else {
        // Mono sample requested
        if (numChannels >= 2) {
            // Average to convert to mono
            const int32_t sampleAverage = ((int32_t) pSamples[0] + pSamples[1]) / 2;
            return (int16_t) sampleAverage;
        }
        else {
            // Happy case: sample is already mono
            return pSamples[0];
        }
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Reads a filtered mono or stereo sample from the given audio data.
// Assumes the audio data is valid, but the sample number CAN be out of range.
//------------------------------------------------------------------------------------------------------------------------------------------
template <bool STEREO>
static SampleT<STEREO, float> smoothSample_f32(const AudioData& audioData, const double sampleIdx, const bool bLoop) noexcept {
    // Grab the info we need from the audio block and validate
    const int16_t* const pSampleData = audioData.pData.get();
    const uint32_t numChannels = audioData.numChannels;
    [[maybe_unused]] const uint32_t numSamples = audioData.numSamples;

    ASSERT(pSampleData);
    ASSERT(numChannels >= 1);
    ASSERT(numSamples >= 1);

    // Figure out the whole and fractional parts of the sample number
    const int64_t sampleIdxWhole = (int64_t) sampleIdx;
    const float sampleIdxFrac = (float)(sampleIdx - (double) sampleIdxWhole);

    // Determine which samples to read
    int64_t readSampleIdx[4];

    if (bLoop) {
        readSampleIdx[0] = (uint64_t)(sampleIdxWhole    ) % audioData.numSamples;
        readSampleIdx[1] = (uint64_t)(sampleIdxWhole + 1) % audioData.numSamples;
        readSampleIdx[2] = (uint64_t)(sampleIdxWhole + 2) % audioData.numSamples;
        readSampleIdx[3] = (uint64_t)(sampleIdxWhole + 3) % audioData.numSamples;
    }
    else {
        readSampleIdx[0] = (uint64_t) std::clamp(sampleIdxWhole,     (int64_t) 0, (int64_t) audioData.numSamples - 1);
        readSampleIdx[1] = (uint64_t) std::clamp(sampleIdxWhole + 1, (int64_t) 0, (int64_t) audioData.numSamples - 1);
        readSampleIdx[2] = (uint64_t) std::clamp(sampleIdxWhole + 2, (int64_t) 0, (int64_t) audioData.numSamples - 1);
        readSampleIdx[3] = (uint64_t) std::clamp(sampleIdxWhole + 3, (int64_t) 0, (int64_t) audioData.numSamples - 1);
    }

    // Gather the 4 samples
    const int16_t* const pSamples0 = &pSampleData[readSampleIdx[0] * numChannels];
    const int16_t* const pSamples1 = &pSampleData[readSampleIdx[1] * numChannels];
    const int16_t* const pSamples2 = &pSampleData[readSampleIdx[2] * numChannels];
    const int16_t* const pSamples3 = &pSampleData[readSampleIdx[3] * numChannels];

    SampleT<STEREO, float> rawSamples[4];

    if constexpr (STEREO) {
        // Stereo sample requested
        if (numChannels >= 2) {
            // Happy case: sample is already stereo (if there are more channels, ignore)
            rawSamples[0] = { toFloatSample(pSamples0[0]), toFloatSample(pSamples0[1]) };
            rawSamples[1] = { toFloatSample(pSamples1[0]), toFloatSample(pSamples1[1]) };
            rawSamples[2] = { toFloatSample(pSamples2[0]), toFloatSample(pSamples2[1]) };
            rawSamples[3] = { toFloatSample(pSamples3[0]), toFloatSample(pSamples3[1]) };
        }
        else {
            // Duplicate mono sample across both channels
            const float monoSample0 = toFloatSample(pSamples0[0]);
            const float monoSample1 = toFloatSample(pSamples1[0]);
            const float monoSample2 = toFloatSample(pSamples2[0]);
            const float monoSample3 = toFloatSample(pSamples3[0]);

            rawSamples[0] = { monoSample0, monoSample0 };
            rawSamples[1] = { monoSample1, monoSample1 };
            rawSamples[2] = { monoSample2, monoSample2 };
            rawSamples[3] = { monoSample3, monoSample3 };
        }
    }
    else {
        // Mono sample requested
        if (numChannels >= 2) {
            // Average to convert to mono
            rawSamples[0] = toFloatSample(((int32_t) pSamples0[0] + pSamples0[1]) / 2);
            rawSamples[1] = toFloatSample(((int32_t) pSamples1[0] + pSamples1[1]) / 2);
            rawSamples[2] = toFloatSample(((int32_t) pSamples2[0] + pSamples2[1]) / 2);
            rawSamples[3] = toFloatSample(((int32_t) pSamples3[0] + pSamples3[1]) / 2);
        }
        else {
            // Happy case: samples are already mono
            rawSamples[0] = toFloatSample(pSamples0[0]);
            rawSamples[1] = toFloatSample(pSamples1[0]);
            rawSamples[2] = toFloatSample(pSamples2[0]);
            rawSamples[3] = toFloatSample(pSamples3[0]);
        }
    }

    // Do the cubic interpolation.
    // If dealing with stereo then interpolate both channels separately.
    if constexpr (STEREO) {
        return SampleT<true, float> {
            cubicInterpolateAudioSample(
                std::get<0>(rawSamples[0]),
                std::get<0>(rawSamples[1]),
                std::get<0>(rawSamples[2]),
                std::get<0>(rawSamples[3]),
                sampleIdxFrac
            ),
            cubicInterpolateAudioSample(
                std::get<1>(rawSamples[0]),
                std::get<1>(rawSamples[1]),
                std::get<1>(rawSamples[2]),
                std::get<1>(rawSamples[3]),
                sampleIdxFrac
            )
        };
    }
    else {
        return cubicInterpolateAudioSample(
            rawSamples[0],
            rawSamples[1],
            rawSamples[2],
            rawSamples[3],
            sampleIdxFrac
        );
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Read a raw sample in 'int16_t' or 'float' format, mono or stereo.
// Assumes the audio data is valid.
//------------------------------------------------------------------------------------------------------------------------------------------
int16_t rawSample_mono_i16(const AudioData& audioData, const int64_t sampleIdx, const bool bLoop) noexcept {
    return rawSample_i16<false>(audioData, sampleIdx, bLoop);
}

float rawSample_mono_f32(const AudioData& audioData, const int64_t sampleIdx, const bool bLoop) noexcept {
    const int16_t i16Sample = rawSample_i16<false>(audioData, sampleIdx, bLoop);
    return toFloatSample(i16Sample);
}

std::tuple<int16_t, int16_t> rawSample_stereo_i16(const AudioData& audioData, const int64_t sampleIdx, const bool bLoop) noexcept {
    return rawSample_i16<true>(audioData, sampleIdx, bLoop);
}

std::tuple<float, float> rawSample_stereo_f32(const AudioData& audioData, const int64_t sampleIdx, const bool bLoop) noexcept {
    const auto [leftSample, rightSample] = rawSample_i16<true>(audioData, sampleIdx, bLoop);
    return { toFloatSample(leftSample), toFloatSample(rightSample) };
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Reads a filtered sample in 'int16_t' or 'float' format, mono or stereo.
// Assumes the audio data is valid, but the sample number CAN be out of range.
//------------------------------------------------------------------------------------------------------------------------------------------
int16_t smoothSample_mono_i16(const AudioData& audioData, const double sampleIdx, const bool bLoop) noexcept {
    return toInt16Sample(smoothSample_f32<false>(audioData, sampleIdx, bLoop));
}

std::tuple<int16_t, int16_t> smoothSample_stereo_i16(const AudioData& audioData, const double sampleIdx, const bool bLoop) noexcept {
    const auto [leftSample, rightSample] = smoothSample_f32<true>(audioData, sampleIdx, bLoop);
    return { toInt16Sample(leftSample), toInt16Sample(rightSample) };
}

float smoothSample_mono_f32(const AudioData& audioData, const double sampleIdx, const bool bLoop) noexcept {
    return smoothSample_f32<false>(audioData, sampleIdx, bLoop);
}

std::tuple<float, float> smoothSample_stereo_f32(const AudioData& audioData, const double sampleIdx, const bool bLoop) noexcept {
    return smoothSample_f32<true>(audioData, sampleIdx, bLoop);
}

END_NAMESPACE(AudioSampler)
