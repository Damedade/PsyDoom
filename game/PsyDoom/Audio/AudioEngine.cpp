//------------------------------------------------------------------------------------------------------------------------------------------
// PsyDoom's own audio playback engine, which runs in parallel to the original WESS sound system.
// Allows for regular PCM sound effects to be played, and for the .wav sound format to be used.
// The output from this engine is piped into the SPU and will receive reverb effects for voices that have reverb enabled.
//------------------------------------------------------------------------------------------------------------------------------------------
#include "AudioEngine.h"

#include "Asserts.h"
#include "AudioVoice.h"
#include "SoundCache.h"
#include "SpuExtInputMux.h"

#include <mutex>

BEGIN_NAMESPACE(AudioEngine)

//------------------------------------------------------------------------------------------------------------------------------------------
// Note: usually a lock must be used when accessing all of these structures.
// See the comments for 'LockAudioEngine' for appropriate usage details.
//------------------------------------------------------------------------------------------------------------------------------------------
SoundCache                  gSoundCache;
LoopingAudioVoiceList       gLoopingVoiceList;
NonLoopingAudioVoiceList    gNonLoopingVoiceList;

//------------------------------------------------------------------------------------------------------------------------------------------
// The lock for the Audio Engine and implementation of an RAII helper class to lock/unlock
//------------------------------------------------------------------------------------------------------------------------------------------
static std::recursive_mutex gAudioEngineMutex;

#ifndef NDEBUG
static size_t gAudioEngineLockCount = 0;
#endif

LockAudioEngine::LockAudioEngine() noexcept {
    gAudioEngineMutex.lock();
#ifndef NDEBUG
    gAudioEngineLockCount++;
#endif
}

LockAudioEngine::~LockAudioEngine() noexcept {
#ifndef NDEBUG
    ASSERT(gAudioEngineLockCount > 0);
    gAudioEngineLockCount--;
#endif
    gAudioEngineMutex.unlock();
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Called from the audio thread to get a sample from the Audio Engine
//------------------------------------------------------------------------------------------------------------------------------------------
static Spu::SpuCallbackOutput SpuAudioCallback() noexcept {
    // The audio thread should have a lock at this point!
    ASSERT(gAudioEngineLockCount > 0);

    // Step all the voices and collect their output
    const Spu::SpuCallbackOutput output1 = AudioVoiceUtils::stepLoopingVoices(gLoopingVoiceList, gSoundCache);
    const Spu::SpuCallbackOutput output2 = AudioVoiceUtils::stepNonLoopingVoices(gNonLoopingVoiceList, gSoundCache);

    // Combine the output and return the result
    Spu::SpuCallbackOutput combinedOutput;
    combinedOutput.sample = output1.sample + output2.sample;
    combinedOutput.reverbSample = output1.reverbSample + output2.reverbSample;
    return combinedOutput;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Init and shutdown the Audio Engine
//------------------------------------------------------------------------------------------------------------------------------------------
void init() noexcept {
    gSoundCache.init();
    // N.B: Doing this at the very end ensures we don't need to do any locking for all the other code in this function.
    // The Audio Engine will only be accessed on other threads once the SPU multiplexor callback is added.
    SpuExtInputMux::addInput(SpuAudioCallback);
}

void shutdown() noexcept {
    const AudioEngine::LockAudioEngine lockAudioEngine;
    SpuExtInputMux::removeInput(SpuAudioCallback);

    gSoundCache.unloadAllSounds();
    gLoopingVoiceList = {};
    gNonLoopingVoiceList = {};
    gSoundCache.shutdown();
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Request a non-looping sound be played.
// Returns 'true' if the sound could be played, 'false' if the sound is not valid.
// Note: this function is only expected to be called on the main thread.
//------------------------------------------------------------------------------------------------------------------------------------------
bool playSound(
    const sfxenum_t soundId,
    const uint8_t volume,
    const uint8_t pan,
    const float pitch,
    const bool bEnableReverb
) noexcept
{
    // Bad sound id?
    if (!gSoundCache.isValidSoundId(soundId))
        return false;

    // Need a lock for the rest of this function.
    // Note: checking for a valid sound id is OK because that data is read-only once initialized.
    const AudioEngine::LockAudioEngine lockAudioEngine;

    // Cache the sound and abort if we can't load it
    if (!gSoundCache.cacheSound(soundId))
        return false;

    // Try to get a free voice, otherwise steal one:
    uint32_t voiceIdx = AudioVoiceUtils::findFirstFreeVoice(gNonLoopingVoiceList);

    if (voiceIdx == UINT32_MAX) {
        voiceIdx = AudioVoiceUtils::pickNonLoopingVoiceToSteal(gNonLoopingVoiceList);
    }

    // Still don't have a valid voice index?
    if (voiceIdx >= C_ARRAY_SIZE(gNonLoopingVoiceList.voices))
        return false;

    // Begin playing the sound
    const bool bSuccess = AudioVoiceUtils::beginVoicePlayback(
        gNonLoopingVoiceList,
        voiceIdx,
        (uint16_t) soundId,
        gSoundCache,
        volume,
        pan,
        pitch,
        bEnableReverb
    );

    return bSuccess;
}

END_NAMESPACE(AudioEngine)
