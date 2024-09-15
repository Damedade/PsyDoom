#pragma once

#include "Spu.h"

#include <tuple>

struct AudioData;

BEGIN_NAMESPACE(AudioSampler)

int16_t rawSample_mono_i16(const AudioData& audioData, const int64_t sampleIdx, const bool bLoop) noexcept;
float rawSample_mono_f32(const AudioData& audioData, const int64_t sampleIdx, const bool bLoop) noexcept;
std::tuple<int16_t, int16_t> rawSample_stereo_i16(const AudioData& audioData, const int64_t sampleIdx, const bool bLoop) noexcept;
std::tuple<float, float> rawSample_stereo_f32(const AudioData& audioData, const int64_t sampleIdx, const bool bLoop) noexcept;

int16_t smoothSample_mono_i16(const AudioData& audioData, const double sampleIdx, const bool bLoop) noexcept;
std::tuple<int16_t, int16_t> smoothSample_stereo_i16(const AudioData& audioData, const double sampleIdx, const bool bLoop) noexcept;
float smoothSample_mono_f32(const AudioData& audioData, const double sampleIdx, const bool bLoop) noexcept;
std::tuple<float, float> smoothSample_stereo_f32(const AudioData& audioData, const double sampleIdx, const bool bLoop) noexcept;

END_NAMESPACE(AudioSampler)
