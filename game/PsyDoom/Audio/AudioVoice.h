#pragma once

#include "Macros.h"

#include <cstdint>

namespace Spu {
    struct SpuCallbackOutput;
}

class SoundCache;

//----------------------------------------------------------------------------------------------------------------------
// Holds details for a single audio voice
//----------------------------------------------------------------------------------------------------------------------
struct AudioVoice {
    // Values for the 'pan' field below
    static constexpr uint8_t PAN_LEFT = 0;
    static constexpr uint8_t PAN_CENTER = 64;
    static constexpr uint8_t PAN_RIGHT = 128;

    // Values for the 'volume' field below
    static constexpr uint8_t VOLUME_MAX = 128;

    uint16_t    soundId : 14;       // Which sound to play: this is just enough bits for 'SoundCache::MAX_SOUND_ID'.
    uint16_t    bReverbEnable : 1;  // Whether the voice should have reverb applied
    uint16_t    bKillVoice : 1;     // When this bit is set the voice should be killed
    uint8_t     volume;             // The current volume of the sound
    uint8_t     pan;                // Current pan value for the voice
    float       playbackRate;       // How much to advance 'samplePos' per sample requested (E.G: '0.5' for a 22 KHz sample playing at normal pitch since the output rate is 44.1 KHz)
    double      samplePos;          // Which sample the audio voice is currently on
};

//----------------------------------------------------------------------------------------------------------------------
// A list of audio voices and whether they are active
//----------------------------------------------------------------------------------------------------------------------
template <uint32_t MAX_VOICES>
struct AudioVoiceList {
    // Voice count must be a multiple of '32'!
    static_assert(MAX_VOICES % 32u == 0);

    // A bitmask of which voices are currently in use
    uint32_t bVoiceActive[MAX_VOICES / 32u];

    // The list of voices
    AudioVoice voices[MAX_VOICES];
};

//----------------------------------------------------------------------------------------------------------------------
// This is how many voices we allow for looping and non-looping sounds
//----------------------------------------------------------------------------------------------------------------------
using NonLoopingAudioVoiceList = AudioVoiceList<64>;
using LoopingAudioVoiceList = AudioVoiceList<32>;

//----------------------------------------------------------------------------------------------------------------------
// Utilities for stepping audio voices and also for finding free voices, or stealing currently active voices
//----------------------------------------------------------------------------------------------------------------------
BEGIN_NAMESPACE(AudioVoiceUtils)

Spu::SpuCallbackOutput stepNonLoopingVoice(AudioVoice& voice, const SoundCache& soundCache) noexcept;
Spu::SpuCallbackOutput stepLoopingVoice(AudioVoice& voice, const SoundCache& soundCache) noexcept;

Spu::SpuCallbackOutput stepNonLoopingVoices(NonLoopingAudioVoiceList& voiceList, const SoundCache& soundCache) noexcept;
Spu::SpuCallbackOutput stepLoopingVoices(LoopingAudioVoiceList& voiceList, const SoundCache& soundCache) noexcept;

END_NAMESPACE(AudioVoiceUtils)
