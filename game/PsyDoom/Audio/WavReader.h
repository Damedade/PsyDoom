#pragma once

#include "AudioData.h"
#include "Macros.h"

BEGIN_NAMESPACE(WavReader)

AudioData readWav_Pcm16(const std::byte* const pBytes, const size_t numBytes) noexcept;

END_NAMESPACE(WavReader)
