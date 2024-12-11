#include "AudioVoice.h"

#include "Asserts.h"
#include "AudioSampler.h"
#include "SoundCache.h"
#include "Spu.h"

#include <algorithm>
#include <cmath>

BEGIN_NAMESPACE(AudioVoiceUtils)

// Sanity check we can fit any valid sound into the 14-bits of 'AudioVoice::soundId'
static_assert(SoundCache::MAX_SOUND_ID <= 0x3FFF);

//------------------------------------------------------------------------------------------------------------------------------------------
// Steps a single audio voice and returns the sample for the current step
//------------------------------------------------------------------------------------------------------------------------------------------
template <bool LOOPING>
static Spu::SpuCallbackOutput stepVoiceImpl(AudioVoice& voice, const SoundCache& soundCache) noexcept {
    // Note: assuming left pan is always '0' to simplify the calculations below
    static_assert(AudioVoice::PAN_LEFT == 0);
    constexpr float PAN_RANGE = (float) AudioVoice::PAN_RIGHT;
    constexpr float INV_PAN_RANGE = 1.0f / PAN_RANGE;

    // Compute the left and right volume multipliers
    const float masterVol = (float) voice.volume / (float) AudioVoice::VOLUME_MAX;
    const float volL = std::max(((float) AudioVoice::PAN_RIGHT - (float) voice.pan) * INV_PAN_RANGE, 0.0f) * masterVol;
    const float volR = std::min((float) voice.pan * INV_PAN_RANGE, 1.0f) * masterVol;

    // Get the sound for the voice: if it's invalid then just return silence.
    // Note: not checking all the fields here for validity. If the sound has data then assume the rest of the fields are OK.
    const AudioData& audioData = soundCache.getSound(voice.soundId);
    const bool bValidSound = (audioData.pData != nullptr);
    ASSERT((audioData.numSamples > 0) || (!bValidSound));
    ASSERT((audioData.numChannels > 0) || (!bValidSound));

    if (!bValidSound)
        return {};

    // Sample the wave and attenuate by volume for the final output
    Spu::SpuCallbackOutput output;

    const auto [sampleL, sampleR] = AudioSampler::smoothSample_stereo_f32(audioData, voice.samplePos, LOOPING);
    output.sample.left = sampleL * volL;
    output.sample.right = sampleR * volR;
    output.reverbSample = (voice.bReverbEnable) ? output.sample : Spu::StereoSample{};

    // Step the voice along and handle end/loop conditions.
    //
    // Note: if the voice is playing in reverse then allow the sample position to go negative.
    // This allows 'abs(samplePos)' to be used as the duration that the sample was playing for, for voice prioritization purposes.
    const double numSamples = (double) audioData.numSamples;
    double newSamplePos = voice.samplePos + voice.playbackRate;

    if constexpr (LOOPING) {
        newSamplePos = std::fmod(newSamplePos, numSamples);
    }
    else {
        const double clampedSamplePos = std::clamp(newSamplePos, -numSamples, +numSamples);
        voice.bKillVoice = (clampedSamplePos != newSamplePos); // Did we reach the end? Kill the voice if so.
        newSamplePos = clampedSamplePos;
    }

    voice.samplePos = newSamplePos;

    // Return the finished output
    return output;
}

Spu::SpuCallbackOutput stepNonLoopingVoice(AudioVoice& voice, const SoundCache& soundCache) noexcept {
    return stepVoiceImpl<false>(voice, soundCache);
}

Spu::SpuCallbackOutput stepLoopingVoice(AudioVoice& voice, const SoundCache& soundCache) noexcept {
    return stepVoiceImpl<true>(voice, soundCache);
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Steps a list of audio voices and returns the combined output for all the voices
//------------------------------------------------------------------------------------------------------------------------------------------
template <bool LOOPING, class VoiceListT>
static Spu::SpuCallbackOutput stepVoicesImpl(VoiceListT& voiceList, const SoundCache& soundCache) noexcept {
    Spu::SpuCallbackOutput allVoicesOutput = {};

    // Run through all of the batches of voices
    constexpr uint32_t NUM_VOICE_GROUPS = C_ARRAY_SIZE(VoiceListT::bVoiceActive);

    for (uint32_t voiceGroupIdx = 0; voiceGroupIdx < NUM_VOICE_GROUPS; ++voiceGroupIdx) {
        // Happy case: can we skip over this group of 32 voices entirely?
        uint32_t activeVoiceMask = voiceList.bVoiceActive[voiceGroupIdx];

        if (activeVoiceMask == 0)
            continue;

        // Check all of the voices in the group to see if they are active
        static_assert(sizeof(voiceList.bVoiceActive[0]) == 4, "Assuming 32-bit voice mask here!");
        const uint32_t maxGroupVoices = (activeVoiceMask & 0xFFFF0000) ? 32u : ((activeVoiceMask & 0x0000FF00) ? 16u : 8u); // Reduce search space by 1/2 or 1/4 if we can!

        for (uint32_t voiceIdxInGroup = 0; voiceIdxInGroup < maxGroupVoices; ++voiceIdxInGroup) {
            // Ignore this voice if its not active
            if (((activeVoiceMask >> voiceIdxInGroup) & 0x1) == 0)
                continue;

            // Get the voice and step it
            const uint32_t voiceIdx = voiceGroupIdx * 32u + voiceIdxInGroup;
            AudioVoice& voice = voiceList.voices[voiceIdx];
            const Spu::SpuCallbackOutput voiceOutput = stepVoiceImpl<LOOPING>(voice, soundCache);

            // Mix the voice's output with the combined output
            allVoicesOutput.sample += voiceOutput.sample;
            allVoicesOutput.reverbSample += voiceOutput.reverbSample;

            // If the voice is non looping and finished then kill it
            if constexpr (!LOOPING) {
                if (voice.bKillVoice) {
                    activeVoiceMask ^= (1u << voiceIdxInGroup);
                }
            }
        }

        // Update the active voice mask at the end if we are not looping - we might have made adjustments:
        if constexpr (!LOOPING) {
            voiceList.bVoiceActive[voiceGroupIdx] = activeVoiceMask;
        }
    }

    return allVoicesOutput;
}

Spu::SpuCallbackOutput stepNonLoopingVoices(NonLoopingAudioVoiceList& voiceList, const SoundCache& soundCache) noexcept {
    return stepVoicesImpl<false>(voiceList, soundCache);
}

Spu::SpuCallbackOutput stepLoopingVoices(LoopingAudioVoiceList& voiceList, const SoundCache& soundCache) noexcept {
    return stepVoicesImpl<true>(voiceList, soundCache);
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Find the index of the first free voice in the specified voice list.
// Returns 'UINT32_MAX' if none was found.
//------------------------------------------------------------------------------------------------------------------------------------------
template <class VoiceListT>
static uint32_t findFirstFreeVoiceImpl(const VoiceListT& voiceList) noexcept {
    // Run through all of the batches of voices
    constexpr uint32_t NUM_VOICE_GROUPS = C_ARRAY_SIZE(VoiceListT::bVoiceActive);

    for (uint32_t voiceGroupIdx = 0; voiceGroupIdx < NUM_VOICE_GROUPS; ++voiceGroupIdx) {
        // Are all of the voices in this group active? If so then we can skip past the entire group.
        uint32_t activeVoiceMask = voiceList.bVoiceActive[voiceGroupIdx];

        if (activeVoiceMask == UINT32_MAX)
            continue;

        // There are some free voices in this group, find and return the first free one:
        uint32_t voiceIdx = voiceGroupIdx * 32u;
  
        while (activeVoiceMask & 0x1) {
            activeVoiceMask >>= 1;
            voiceIdx++;
        }

        return voiceIdx;
    }

    return UINT32_MAX;
}

uint32_t findFirstFreeVoice(const NonLoopingAudioVoiceList& voiceList) noexcept {
    return findFirstFreeVoiceImpl(voiceList);
}

uint32_t findFirstFreeVoice(const LoopingAudioVoiceList& voiceList) noexcept {
    return findFirstFreeVoiceImpl(voiceList);
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Picks a voice to steal for looping and non-looping voices.
// N.B: Assumes all voices in the list are in use! Should only be called if 'findFirstFreeVoice()' fails.
//------------------------------------------------------------------------------------------------------------------------------------------
template <class VoiceListT>
static uint32_t pickNonLoopingVoiceToStealImpl(const VoiceListT& voiceList) noexcept {
    // All voices should be in use
    #if ASSERTS_ENABLED
        for (const uint32_t& activeVoiceMask : voiceList.bVoiceActive) {
            ASSERT(activeVoiceMask == UINT32_MAX);
        }
    #endif

    // Pick the voice that has been playing the longest
    double longestSamplePos = 0.0;
    uint32_t stealVoiceIdx = UINT32_MAX;

    for (uint32_t voiceIdx = 0; voiceIdx < C_ARRAY_SIZE(voiceList.voices); ++voiceIdx) {
        const AudioVoice& voice = voiceList.voices[voiceIdx];

        // Note: use the 'abs()' of the sample position so that sounds can be played in reverse (if needed).
        // This allows us to use the sample position as a relative indicator of how long the sound has been playing, even if reversed.
        const double absSamplePos = std::abs(voice.samplePos);

        if (absSamplePos > longestSamplePos) {
            longestSamplePos = absSamplePos;
            stealVoiceIdx = voiceIdx;
        }
    }

    return stealVoiceIdx;
}

template <class VoiceListT>
static uint32_t pickLoopingVoiceToStealImpl(const VoiceListT& voiceList) noexcept {
    // All voices should be in use
    #if ASSERTS_ENABLED
        for (const uint32_t& activeVoiceMask : voiceList.bVoiceActive) {
            ASSERT(activeVoiceMask == UINT32_MAX);
        }
    #endif

    // Pick the quietest voice to steal
    uint8_t quietestVoiceVol = UINT8_MAX;
    uint32_t stealVoiceIdx = UINT32_MAX;

    for (uint32_t voiceIdx = 0; voiceIdx < C_ARRAY_SIZE(voiceList.voices); ++voiceIdx) {
        const AudioVoice& voice = voiceList.voices[voiceIdx];
        const uint8_t voiceVol = voice.volume;

        if (voiceVol < quietestVoiceVol) {
            quietestVoiceVol = voiceVol;
            stealVoiceIdx = voiceIdx;
        }
    }

    return stealVoiceIdx;
}

uint32_t pickNonLoopingVoiceToSteal(const NonLoopingAudioVoiceList& voiceList) noexcept {
    return pickNonLoopingVoiceToStealImpl(voiceList);
}

uint32_t pickLoopingVoiceToSteal(const LoopingAudioVoiceList& voiceList) noexcept {
    return pickLoopingVoiceToStealImpl(voiceList);
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Initializes the specified voice structure for playback.
// Leaves the structure unmodified if the specified sound does not exist in the cache.
// Returns 'true' on success.
//------------------------------------------------------------------------------------------------------------------------------------------
bool initVoiceForPlayback(
    AudioVoice& voice,
    const uint16_t soundId,
    const SoundCache& soundCache,
    const uint8_t volume,
    const uint8_t pan,
    const float pitch,
    const bool bEnableReverb
) noexcept
{
    // Is the sound valid?
    const AudioData& audioData = soundCache.getSound(soundId);

    if (audioData.pData.get() == nullptr)
        return false;

    // If the sound data is present then all the rest of these fields are expected to be valid
    ASSERT(audioData.sampleRate > 0);
    ASSERT(audioData.numChannels > 0);
    ASSERT(audioData.numSamples > 0);

    // Fill in the voice struct
    voice.soundId = soundId;
    voice.bReverbEnable = bEnableReverb; 
    voice.bKillVoice = false;
    voice.volume = volume;
    voice.pan = pan;
    voice.playbackRate = ((float) audioData.sampleRate / 44100.0f) * pitch; // PsyDoom output rate is always 44.1 KHz (PSX Spu sample rate)
    voice.samplePos = 0.0;

    return true;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Begin the playback of looping or non-looping voices.
// Note: the voice index given must be valid.
//------------------------------------------------------------------------------------------------------------------------------------------
template <class VoiceListT>
static bool beginVoicePlaybackImpl(
    VoiceListT& voiceList,
    const uint32_t voiceIdx,
    const uint16_t soundId,
    const SoundCache& soundCache,
    const uint8_t volume,
    const uint8_t pan,
    const float pitch,
    const bool bEnableReverb
) noexcept
{
    // Init the voice for playback and abort if failed
    ASSERT(voiceIdx < C_ARRAY_SIZE(voiceList.voices));
    AudioVoice& voice = voiceList.voices[voiceIdx];

    if (!initVoiceForPlayback(voice, soundId, soundCache, volume, pan, pitch, bEnableReverb))
        return false;

    // When successful mark the voice as active
    static_assert(sizeof(voiceList.bVoiceActive[0]) == 4); // Expect 32-bit masks
    voiceList.bVoiceActive[voiceIdx / 32u] |= 1u << (voiceIdx & 31u);
    return true;
}

bool beginVoicePlayback(
    NonLoopingAudioVoiceList& voiceList,
    const uint32_t voiceIdx,
    const uint16_t soundId,
    const SoundCache& soundCache,
    const uint8_t volume,
    const uint8_t pan,
    const float pitch,
    const bool bEnableReverb
) noexcept
{
    return beginVoicePlaybackImpl(voiceList, voiceIdx, soundId, soundCache, volume, pan, pitch, bEnableReverb);
}

bool beginVoicePlayback(
    LoopingAudioVoiceList& voiceList,
    const uint32_t voiceIdx,
    const uint16_t soundId,
    const SoundCache& soundCache,
    const uint8_t volume,
    const uint8_t pan,
    const float pitch,
    const bool bEnableReverb
) noexcept
{
    return beginVoicePlaybackImpl(voiceList, voiceIdx, soundId, soundCache, volume, pan, pitch, bEnableReverb);
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Stops playback of the specified voice
//------------------------------------------------------------------------------------------------------------------------------------------
template <class VoiceListT>
static void stopVoicePlaybackImpl(VoiceListT& voiceList, const uint32_t voiceIdx) noexcept {
    ASSERT(voiceIdx < C_ARRAY_SIZE(voiceList.voices));
    static_assert(sizeof(voiceList.bVoiceActive[0]) == 4);                  // Expect 'voice active' masks to be 32-bit
    voiceList.bVoiceActive[voiceIdx / 32u] &= ~(1u << (voiceIdx & 31u));    // Mark the voice as no longer active
}

void stopVoicePlayback(NonLoopingAudioVoiceList& voiceList, const uint32_t voiceIdx) noexcept {
    stopVoicePlaybackImpl(voiceList, voiceIdx);
}

void stopVoicePlayback(LoopingAudioVoiceList& voiceList, const uint32_t voiceIdx) noexcept {
    stopVoicePlaybackImpl(voiceList, voiceIdx);
}

END_NAMESPACE(AudioVoiceUtils)
