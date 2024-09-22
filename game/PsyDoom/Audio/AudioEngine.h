#pragma once

#include "Macros.h"

#include <cstdint>

class SoundCache;
enum sfxenum_t : int32_t;
struct LoopingAudioVoiceList;
struct NonLoopingAudioVoiceList;

BEGIN_NAMESPACE(AudioEngine)

// An RAII lock for the audio engine.
// 
// This must be acquired in the following situations:
//  (1) Whenever the audio thread needs access to any part of the engine.
//  (2) Whenever the main thread wants to read or modify voices.
//      The audio thread might be reading or modifying these voices concurrently.
//  (3) Whenever the main thread wants to modify the sound cache.
//      Note: Read-only access is OK without a lock since the audio thread will only read this data structure.
//
struct LockAudioEngine {
    LockAudioEngine() noexcept;
    ~LockAudioEngine() noexcept;
};

extern SoundCache                   gSoundCache;
extern LoopingAudioVoiceList        gLoopingVoiceList;
extern NonLoopingAudioVoiceList     gNonLoopingVoiceList;

void init() noexcept;
void shutdown() noexcept;

bool playSound(
    const sfxenum_t soundId,
    const uint8_t volume,
    const uint8_t pan,
    const float pitch,
    const bool bEnableReverb
) noexcept;

END_NAMESPACE(AudioEngine)
