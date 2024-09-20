#include "AudioVoice.h"

#include "Asserts.h"
#include "AudioSampler.h"
#include "SoundCache.h"
#include "Spu.h"

#include <algorithm>

BEGIN_NAMESPACE(AudioVoiceUtils)

// Sanity check we can fit any valid sound into the 14-bits of 'AudioVoice::soundId'
static_assert(SoundCache::MAX_SOUND_ID <= 0x3FFF);

//------------------------------------------------------------------------------------------------------------------------------------------
// Steps a single audio voice and returns the sample for the current step
//------------------------------------------------------------------------------------------------------------------------------------------
template <bool LOOPING>
static Spu::SpuCallbackOutput stepVoice(AudioVoice& voice, const SoundCache& soundCache) noexcept {
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

    // Step the voice along and handle end/loop conditions
    const double numSamples = (double) audioData.numSamples;
    double newSamplePos = voice.samplePos + voice.playbackRate;

    if constexpr (LOOPING) {
        newSamplePos = std::fmod(newSamplePos, numSamples);
        newSamplePos = (newSamplePos >= 0.0) ? newSamplePos : numSamples - newSamplePos;
    }
    else {
        const double clampedSamplePos = std::clamp(newSamplePos, 0.0, numSamples);
        voice.bKillVoice = (clampedSamplePos != newSamplePos); // Did we reach the end? Kill the voice if so.
        newSamplePos = clampedSamplePos;
    }

    // Return the finished output
    return output;
}

Spu::SpuCallbackOutput stepNonLoopingVoice(AudioVoice& voice, const SoundCache& soundCache) noexcept {
    return stepVoice<false>(voice, soundCache);
}

Spu::SpuCallbackOutput stepLoopingVoice(AudioVoice& voice, const SoundCache& soundCache) noexcept {
    return stepVoice<true>(voice, soundCache);
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Steps a list of audio voices and returns the combined output for all the voices
//------------------------------------------------------------------------------------------------------------------------------------------
template <bool LOOPING, class VoiceListT>
static Spu::SpuCallbackOutput stepVoices(VoiceListT& voiceList, const SoundCache& soundCache) noexcept {
    Spu::SpuCallbackOutput allVoicesOutput = {};

    // Run through all of the batches of voices
    constexpr uint32_t NUM_VOICE_GROUPS = C_ARRAY_SIZE(VoiceListT::voices);

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
            const Spu::SpuCallbackOutput voiceOutput = stepVoice<LOOPING>(voice, soundCache);

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
    return stepVoices<false>(voiceList, soundCache);
}

Spu::SpuCallbackOutput stepLoopingVoices(LoopingAudioVoiceList& voiceList, const SoundCache& soundCache) noexcept {
    return stepVoices<true>(voiceList, soundCache);
}

END_NAMESPACE(AudioVoiceUtils)
