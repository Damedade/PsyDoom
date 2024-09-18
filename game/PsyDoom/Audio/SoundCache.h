#pragma once

#include "AudioData.h"
#include "Spu.h"

#include <bitset>
#include <cstdint>
#include <unordered_map>
#include <vector>

enum sfxenum_t : int32_t;
struct AudioData;

//------------------------------------------------------------------------------------------------------------------------------------------
// Part of PsyDoom's new sound effect playback system, which operates in parallel to the original 'WESS' sound system.
// Loads, stores and provides access to 'AudioData' entries for various sounds in the game.
//------------------------------------------------------------------------------------------------------------------------------------------
class SoundCache {
public:
    // Maximum sound id the audio cache can support.
    // Up to 10,000 sounds can be loaded - should be more than enough for any project.
    static constexpr uint16_t MAX_SOUND_ID = 9999;

    SoundCache() noexcept;
    ~SoundCache() noexcept;

    bool cacheSound(const uint16_t soundId) noexcept;
    bool cacheSound(const sfxenum_t soundId) noexcept;
    void cacheMenuSounds() noexcept;
    void cacheCurrentMapSounds() noexcept;

    bool isSoundLoaded(const uint16_t soundId) const noexcept;
    bool isSoundLoaded(const sfxenum_t soundId) const noexcept;
    const AudioData& getSound(const uint16_t soundId) const noexcept;
    const AudioData& getSound(const sfxenum_t soundId) const noexcept;
    const AudioData& cacheAndGetSound(const uint16_t soundId) noexcept;
    const AudioData& cacheAndGetSound(const sfxenum_t soundId) noexcept;

    void unloadAllSounds() noexcept;
    bool unloadSound(const uint16_t soundId) noexcept;
    bool unloadSound(const sfxenum_t soundId) noexcept;
    void unloadAllMapExclusiveSounds() noexcept;
    void unloadAllCustomSounds() noexcept;

private:
    // Represents an invalid/unmapped sound index in 'mSoundIdToIdx'
    static constexpr uint16_t INVALID_SOUND_IDX = UINT16_MAX;

    void determineValidSoundIds() noexcept;
    int32_t getSoundLumpIdx(const uint16_t soundId) noexcept;
    uint16_t cacheAndGetSoundIdx(const uint16_t soundId) noexcept;

    void unloadSoundRange(
        const uint32_t begSoundId,
        const uint32_t endSoundId,
        bool (*unloadSoundPredicate)(const uint16_t soundId) noexcept
    ) noexcept;

    // Which sound indexes are actually valid and in the WAD files.
    // These may be set to 'false' if loading fails.
    std::bitset<MAX_SOUND_ID + 1> mbValidSoundIdx;

    // Maps from a sound id to an index in the 'mSounds' array.
    // One slot for every allowable sound index.
    // If a sound has not yet been loaded then the array entry will be 'INVALID_SOUND_IDX'.
    uint16_t mSoundIdToIdx[MAX_SOUND_ID + 1];

    // The array of loaded sounds.
    // This may contain gaps if some sounds have been unloaded.
    std::vector<AudioData> mSounds;

    // Which slots are free in 'mSounds'
    std::vector<uint16_t> mFreeSoundSlots;
};
