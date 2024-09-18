#include "SoundCache.h"

#include "Doom/Base/sounds.h"
#include "Doom/Base/w_wad.h"
#include "Doom/Game/info.h"
#include "Doom/Game/p_tick.h"
#include "WavReader.h"

#include <cctype>
#include <cstdio>

// A value that can be returned to signify invalid audio data
static const AudioData INVALID_AUDIO_DATA = {};

//----------------------------------------------------------------------------------------------------------------------
// Helper: tells if a sound is a menu sound.
// Note that this sound might also be used in maps too.
//----------------------------------------------------------------------------------------------------------------------
static bool isMenuSound(const uint16_t soundId) noexcept {
    switch (soundId) {
        case sfx_sgcock:
        case sfx_itmbk:
        case sfx_barexp:
        case sfx_firxpl:
        case sfx_pistol:
        case sfx_shotgn:
        case sfx_pstop:
        case sfx_stnmov:
        case sfx_swtchn:
        case sfx_swtchx:
        case sfx_itemup:
            return true;
    }

    return false;
}

//----------------------------------------------------------------------------------------------------------------------
// Helper: tells if a sound is a built-in menu or map sound
//----------------------------------------------------------------------------------------------------------------------
static bool isMenuOrMapSound(const uint16_t soundId) noexcept {
    // It's a menu or map sound if it's in one of the built-in sound id ranges:
    return (
        ((soundId >= (uint32_t) SFX_RANGE1_BEG) && (soundId < (uint32_t) SFX_RANGE1_END)) ||
        ((soundId >= (uint32_t) SFX_RANGE2_BEG) && (soundId < (uint32_t) SFX_RANGE2_END))
    );
}

//----------------------------------------------------------------------------------------------------------------------
// Helper: tells if a sound belongs exclusively to the map and is not used in menus
//----------------------------------------------------------------------------------------------------------------------
static bool isMapExclusiveSound(const uint16_t soundId) noexcept {
    return (isMenuOrMapSound(soundId) && (!isMenuSound(soundId)));
}

//----------------------------------------------------------------------------------------------------------------------
// Helper: tells if a sound is a custom/user defined sound not built into PsyDoom
//----------------------------------------------------------------------------------------------------------------------
static bool isCustomSound(const uint16_t soundId) noexcept {
    return (!isMenuOrMapSound(soundId));
}

//----------------------------------------------------------------------------------------------------------------------
// Create and destroy the sound cache
//----------------------------------------------------------------------------------------------------------------------
SoundCache::SoundCache() noexcept
    : mbValidSoundIdx()
    , mSoundIdToIdx{}
    , mSounds()
    , mFreeSoundSlots()
{
    for (uint16_t& idx : mSoundIdToIdx) {
        idx = INVALID_SOUND_IDX;
    }

    mSounds.reserve(1024);
    mFreeSoundSlots.reserve(1024);
}

SoundCache::~SoundCache() noexcept = default;

//----------------------------------------------------------------------------------------------------------------------
// Attempts to load/cache a specified sound.
// Returns 'true' on success, or if the sound is already loaded.
//----------------------------------------------------------------------------------------------------------------------
bool SoundCache::cacheSound(const uint16_t soundId) noexcept {
    return (cacheAndGetSoundIdx(soundId) != INVALID_SOUND_IDX);
}

bool SoundCache::cacheSound(const sfxenum_t soundId) noexcept {
    return ((soundId >= 0) && (soundId <= MAX_SOUND_ID)) ? cacheSound((uint16_t) soundId) : false;
}

//----------------------------------------------------------------------------------------------------------------------
// Cache all sounds used in menus
//----------------------------------------------------------------------------------------------------------------------
void SoundCache::cacheMenuSounds() noexcept {
    cacheSound(sfx_sgcock);     // 1: Weapon pickup sound
    cacheSound(sfx_itmbk);      // 3: Deathmatch item respawn
    cacheSound(sfx_barexp);     // 5: Barrel/rocket explode
    cacheSound(sfx_firxpl);     // 6: Demon fireball hit
    cacheSound(sfx_pistol);     // 7: Pistol fire
    cacheSound(sfx_shotgn);     // 8: Shotgun fire
    cacheSound(sfx_pstop);      // 18: Elevator/mover stop (also menu up/down sound)
    cacheSound(sfx_stnmov);     // 21: Floor/crusher move sound
    cacheSound(sfx_swtchn);     // 22: Switch activate
    cacheSound(sfx_swtchx);     // 23: Exit switch activate
    cacheSound(sfx_itemup);     // 24: Bonus pickup
}

//----------------------------------------------------------------------------------------------------------------------
// Cache all sounds needed for the current map
//----------------------------------------------------------------------------------------------------------------------
void SoundCache::cacheCurrentMapSounds() noexcept {
    //------------------------------------------------------------------------------------------------------------------
    // Always cache these weapon and environmental sounds.
    // Technically we could probably skip loading SOME of the door sounds depending on what triggers the map has.
    // Determining that could get complex though, and probably not worth the effort.
    //------------------------------------------------------------------------------------------------------------------
    cacheSound(sfx_sgcock);     // 1: Weapon pickup sound
    cacheSound(sfx_punch);      // 2: Punch hit
    cacheSound(sfx_itmbk);      // 3: Deathmatch item respawn
    cacheSound(sfx_firsht2);    // 4: Demon/Baron/Cacodemon etc. fireball sound
    cacheSound(sfx_barexp);     // 5: Barrel/rocket explode
    cacheSound(sfx_firxpl);     // 6: Demon fireball hit
    cacheSound(sfx_pistol);     // 7: Pistol fire
    cacheSound(sfx_shotgn);     // 8: Shotgun fire
    cacheSound(sfx_plasma);     // 9: Plasma rifle fire
    cacheSound(sfx_bfg);        // 10: BFG start firing
    cacheSound(sfx_sawup);      // 11: Chainsaw being started up
    cacheSound(sfx_sawidl);     // 12: Chainsaw idle loop
    cacheSound(sfx_sawful);     // 13: Chainsaw saw
    cacheSound(sfx_sawhit);     // 14: Chainsaw hit
    cacheSound(sfx_rlaunc);     // 15: Rocket fire sound
    cacheSound(sfx_rxplod);     // 16: BFG explosion sound
    cacheSound(sfx_pstart);     // 17: Elevator start
    cacheSound(sfx_pstop);      // 18: Elevator/mover stop (also menu up/down sound)
    cacheSound(sfx_doropn);     // 19: Regular/slow door open
    cacheSound(sfx_dorcls);     // 20: Regular/slow door close
    cacheSound(sfx_stnmov);     // 21: Floor/crusher move sound
    cacheSound(sfx_swtchn);     // 22: Switch activate
    cacheSound(sfx_swtchx);     // 23: Exit switch activate
    cacheSound(sfx_itemup);     // 24: Bonus pickup
    cacheSound(sfx_wpnup);      // 25: Weapon pickup sound
    cacheSound(sfx_oof);        // 26: Ooof sound after falling hard, or when trying to use unusable wall
    cacheSound(sfx_telept);     // 27: Teleport sound
    cacheSound(sfx_noway);      // 28: Ooof sound after falling hard, or when trying to use unusable wall
    cacheSound(sfx_dshtgn);     // 29: Super shotgun fire
    cacheSound(sfx_dbopn);      // 30: SSG open barrel
    cacheSound(sfx_dbload);     // 31: SSG load shells
    cacheSound(sfx_dbcls);      // 32: SSG close barrel
    cacheSound(sfx_plpain);     // 33: Player pain sound
    cacheSound(sfx_pldeth);     // 34: Player death sound
    cacheSound(sfx_slop);       // 35: Gib/squelch sound
    cacheSound(sfx_firsht);     // 74: Demon/Baron/Cacodemon etc. fireball sound
    cacheSound(sfx_bdopn);      // 87: Fast/blaze door open
    cacheSound(sfx_bdcls);      // 88: Fast/blaze door close
    cacheSound(sfx_getpow);     // 89: Powerup pickup

    //------------------------------------------------------------------------------------------------------------------
    // Demon pain/idle is used by many monsters: cache it always rather than trying to be smart
    //------------------------------------------------------------------------------------------------------------------
    cacheSound(sfx_dmpain);     // 44: Demon pain
    cacheSound(sfx_dmact);      // 45: Demon idle/growl

    //------------------------------------------------------------------------------------------------------------------
    // Run through the map and see what enemy sounds we need to cache
    //------------------------------------------------------------------------------------------------------------------
    bool bCacheSfx_formerHuman = false;
    bool bCacheSfx_revenant = false;
    bool bCacheSfx_mancubus = false;
    bool bCacheSfx_imp = false;
    bool bCacheSfx_demon = false;
    bool bCacheSfx_cacodemon = false;
    bool bCacheSfx_baron = false;
    bool bCacheSfx_hellKnight = false;
    bool bCacheSfx_lostSoul = false;
    bool bCacheSfx_mastermind = false;
    bool bCacheSfx_arachnotron = false;
    bool bCacheSfx_cyberdemon = false;
    bool bCacheSfx_painElemental = false;
    bool bCacheSfx_archVile = false;
    bool bCacheSfx_wolfSS = false;
    bool bCacheSfx_keen = false;
    bool bCacheSfx_iconOfSin = false;

    for (const mobj_t* pMobj = gMobjHead.next; pMobj != &gMobjHead; pMobj = pMobj->next) {
        const mobjtype_t type = pMobj->type;

        switch (type) {
            case MT_POSSESSED:
            case MT_SHOTGUY:
            case MT_CHAINGUY:
                bCacheSfx_formerHuman = true;
                break;

            case MT_UNDEAD:     bCacheSfx_revenant      = true; break;
            case MT_FATSO:      bCacheSfx_mancubus      = true; break;
            case MT_TROOP:      bCacheSfx_imp           = true; break;
            case MT_SERGEANT:   bCacheSfx_demon         = true; break;
            case MT_HEAD:       bCacheSfx_cacodemon     = true; break;
            case MT_BRUISER:    bCacheSfx_baron         = true; break;
            case MT_KNIGHT:     bCacheSfx_hellKnight    = true; break;
            case MT_SKULL:      bCacheSfx_lostSoul      = true; break;
            case MT_SPIDER:     bCacheSfx_mastermind    = true; break;
            case MT_BABY:       bCacheSfx_arachnotron   = true; break;
            case MT_CYBORG:     bCacheSfx_cyberdemon    = true; break;
            case MT_PAIN:       bCacheSfx_painElemental = true; break;
            case MT_VILE:       bCacheSfx_archVile      = true; break;
            case MT_WOLFSS:     bCacheSfx_wolfSS        = true; break;
            case MT_KEEN:       bCacheSfx_keen          = true; break;
            case MT_BOSSBRAIN:  bCacheSfx_iconOfSin     = true; break;
        }
    }

    //------------------------------------------------------------------------------------------------------------------
    // Cache all required enemy sounds
    //------------------------------------------------------------------------------------------------------------------
    if (bCacheSfx_formerHuman) {
        cacheSound(sfx_posit1);     // 36: Former human sight: 1
        cacheSound(sfx_posit2);     // 37: Former human sight: 2
        cacheSound(sfx_posit3);     // 38: Former human sight: 3 (unused)
        cacheSound(sfx_podth1);     // 39: Former human death: 1
        cacheSound(sfx_podth2);     // 40: Former human death: 2
        cacheSound(sfx_podth3);     // 41: Former human death: 3 (unused)
        cacheSound(sfx_posact);     // 42: Former human idle
        cacheSound(sfx_popain);     // 43: Former human pain
    }

    if (bCacheSfx_imp) {
        cacheSound(sfx_claw);       // 46: Imp/Baron etc. melee claw
        cacheSound(sfx_bgsit1);     // 47: Imp sight: 1
        cacheSound(sfx_bgsit2);     // 48: Imp sight: 2
        cacheSound(sfx_bgdth1);     // 49: Imp death: 1
        cacheSound(sfx_bgdth2);     // 50: Imp death: 2
        cacheSound(sfx_bgact);      // 51: Imp idle
    }

    if (bCacheSfx_demon) {
        cacheSound(sfx_sgtsit);     // 52: Demon sight
        cacheSound(sfx_sgtatk);     // 53: Demon attack
        cacheSound(sfx_sgtdth);     // 54: Demon death
    }

    if (bCacheSfx_baron) {
        cacheSound(sfx_brssit);     // 55: Baron sight
        cacheSound(sfx_brsdth);     // 56: Baron death
    }

    if (bCacheSfx_cacodemon) {
        cacheSound(sfx_cacsit);     // 57: Cacodemon sight
        cacheSound(sfx_cacdth);     // 58: Cacodemon death
    }

    if (bCacheSfx_lostSoul) {
        cacheSound(sfx_sklatk);     // 59: Lost Soul attack
        cacheSound(sfx_skldth);     // 60: (Unused) Intended for Lost Soul death?
    }

    if (bCacheSfx_hellKnight) {
        cacheSound(sfx_kntsit);     // 61: Knight sight
        cacheSound(sfx_kntdth);     // 62: Knight death
    }

    if (bCacheSfx_painElemental) {
        cacheSound(sfx_pesit);      // 63: Pain Elemental sight
        cacheSound(sfx_pepain);     // 64: Pain Elemental pain
        cacheSound(sfx_pedth);      // 65: Pain Elemental death
    }

    if (bCacheSfx_arachnotron) {
        cacheSound(sfx_bspsit);     // 66: Arachnotron sight
        cacheSound(sfx_bspdth);     // 67: Arachnotron death
        cacheSound(sfx_bspact);     // 68: Arachnotron idle
        cacheSound(sfx_bspwlk);     // 69: Arachnotron hoof
    }

    if (bCacheSfx_mancubus) {
        cacheSound(sfx_manatk);     // 70: Mancubus attack
        cacheSound(sfx_mansit);     // 71: Mancubus sight
        cacheSound(sfx_mnpain);     // 72: Mancubus pain
        cacheSound(sfx_mandth);     // 73: Mancubus death
    }

    if (bCacheSfx_revenant) {
        cacheSound(sfx_skesit);     // 75: Revenant sight
        cacheSound(sfx_skedth);     // 76: Revenant death
        cacheSound(sfx_skeact);     // 77: Revenant idle
        cacheSound(sfx_skeatk);     // 78: Revenant missile fire
        cacheSound(sfx_skeswg);     // 79: Revenant throw punch
        cacheSound(sfx_skepch);     // 80: Revenant punch land
    }

    if (bCacheSfx_cacodemon) {
        cacheSound(sfx_cybsit);     // 81: Cyberdemon sight
        cacheSound(sfx_cybdth);     // 82: Cyberdemon death
        cacheSound(sfx_hoof);       // 83: Cyberdemon hoof up
        cacheSound(sfx_metal);      // 84: Cyberdemon thud down (metal)
    }

    if (bCacheSfx_mastermind) {
        cacheSound(sfx_spisit);     // 85: Spider Mastermind sight
        cacheSound(sfx_spidth);     // 86: Spider Mastermind death
    }

    if (bCacheSfx_archVile) {
        cacheSound(sfx_vilsit);     // 120: Arch-vile sight
        cacheSound(sfx_vipain);     // 121: Arch-vile pain
        cacheSound(sfx_vildth);     // 122: Arch-vile death
        cacheSound(sfx_vilact);     // 123: Arch-vile idle
        cacheSound(sfx_vilatk);     // 124: Arch-vile attack
        cacheSound(sfx_flamst);     // 125: Arch-vile flames (start)
        cacheSound(sfx_flame);      // 126: Arch-vile flames burn
    }

    if (bCacheSfx_wolfSS) {
        cacheSound(sfx_sssit);      // 127: Wolfenstein-SS sight
        cacheSound(sfx_ssdth);      // 128: Wolfenstein-SS death
    }

    if (bCacheSfx_keen) {
        cacheSound(sfx_keenpn);     // 129: Commander Keen pain
        cacheSound(sfx_keendt);     // 130: Commander Keen death
    }

    if (bCacheSfx_iconOfSin) {
        cacheSound(sfx_bossit);     // 131: Icon of Sin sight
        cacheSound(sfx_bospit);     // 132: Icon of Sin cube spit
        cacheSound(sfx_bospn);      // 133: Icon of Sin pain
        cacheSound(sfx_bosdth);     // 134: Icon of Sin death
        cacheSound(sfx_boscub);     // 135: Icon of Sin spawn cube fly
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Query if a particular sound is loaded
//----------------------------------------------------------------------------------------------------------------------
bool SoundCache::isSoundLoaded(const uint16_t soundId) const noexcept {
    if (soundId > MAX_SOUND_ID)
        return false;

    const uint16_t soundIdx = mSoundIdToIdx[soundId];
    return (soundIdx != INVALID_SOUND_IDX);
}

bool SoundCache::isSoundLoaded(const sfxenum_t soundId) const noexcept {
    return ((soundId >= 0) && (soundId <= MAX_SOUND_ID)) ? isSoundLoaded((uint16_t) soundId) : false;
}

//----------------------------------------------------------------------------------------------------------------------
// Return the 'AudioData' for the requested sound or an invalid 'AudioData' if the sound is not loaded.
//----------------------------------------------------------------------------------------------------------------------
const AudioData& SoundCache::getSound(const uint16_t soundId) const noexcept {
    if (soundId > MAX_SOUND_ID)
        return INVALID_AUDIO_DATA;

    const uint16_t soundIdx = mSoundIdToIdx[soundId];
    return (soundIdx != INVALID_SOUND_IDX) ? mSounds[soundIdx] : INVALID_AUDIO_DATA;
}

const AudioData& SoundCache::getSound(const sfxenum_t soundId) const noexcept {
    return ((soundId >= 0) && (soundId <= MAX_SOUND_ID)) ? getSound((uint16_t) soundId) : INVALID_AUDIO_DATA;
}

//----------------------------------------------------------------------------------------------------------------------
// Return the 'AudioData' for the requested sound or an invalid 'AudioData' on failure.
// Will attempt to cache the sound first if the sound is not loaded.
//----------------------------------------------------------------------------------------------------------------------
const AudioData& SoundCache::cacheAndGetSound(const uint16_t soundId) noexcept {
    if (soundId > MAX_SOUND_ID)
        return INVALID_AUDIO_DATA;

    const uint16_t soundIdx = mSoundIdToIdx[soundId];

    if (soundIdx != INVALID_SOUND_IDX)
        return mSounds[soundIdx];

    const uint16_t newSoundIdx = cacheAndGetSoundIdx(soundId);
    return (newSoundIdx != INVALID_SOUND_IDX) ? mSounds[newSoundIdx] : INVALID_AUDIO_DATA;
}

const AudioData& SoundCache::cacheAndGetSound(const sfxenum_t soundId) noexcept {
    return ((soundId >= 0) && (soundId <= MAX_SOUND_ID)) ? cacheAndGetSound((uint16_t) soundId) : INVALID_AUDIO_DATA;
}

//----------------------------------------------------------------------------------------------------------------------
// Completely unloads all sounds in the cache
//----------------------------------------------------------------------------------------------------------------------
void SoundCache::unloadAllSounds() noexcept {
    for (uint16_t& idx : mSoundIdToIdx) {
        idx = INVALID_SOUND_IDX;
    }

    mSounds.clear();
    mFreeSoundSlots.clear();
}

//----------------------------------------------------------------------------------------------------------------------
// Unloads a single sound and returns 'true' if it was unloaded.
// Returns 'false' if the sound was already unloaded.
//----------------------------------------------------------------------------------------------------------------------
bool SoundCache::unloadSound(const uint16_t soundId) noexcept {
    if (soundId > MAX_SOUND_ID)
        return false;

    const uint16_t soundIdx = mSoundIdToIdx[soundId];

    if (soundIdx == INVALID_SOUND_IDX)
        return false;

    mFreeSoundSlots.push_back(soundIdx);
    mSounds[soundIdx] = {};
    mSoundIdToIdx[soundId] = INVALID_SOUND_IDX;
    return true;
}

bool SoundCache::unloadSound(const sfxenum_t soundId) noexcept {
    return ((soundId >= 0) && (soundId <= MAX_SOUND_ID)) ? unloadSound((uint16_t) soundId) : false;
}

//----------------------------------------------------------------------------------------------------------------------
// Unload all sounds used exclusively for maps.
// Sounds that are needed for the menu remain loaded.
//----------------------------------------------------------------------------------------------------------------------
void SoundCache::unloadAllMapExclusiveSounds() noexcept {
    unloadSoundRange(SFX_RANGE1_BEG, SFX_RANGE1_END, isMapExclusiveSound);
    unloadSoundRange(SFX_RANGE2_BEG, SFX_RANGE2_END, isMapExclusiveSound);
}

//----------------------------------------------------------------------------------------------------------------------
// Unloads all sounds outside of the built-in ones provided by PsyDoom (user/custom sounds)
//----------------------------------------------------------------------------------------------------------------------
void SoundCache::unloadAllCustomSounds() noexcept {
    unloadSoundRange(0, MAX_SOUND_ID + 1, isCustomSound);
}

//----------------------------------------------------------------------------------------------------------------------
// Figures out which sound ids can be loaded from WAD files.
// Sounds in the WAD files must have the following naming convention where 'xxxx' is the sound id: WSNDxxxx
//----------------------------------------------------------------------------------------------------------------------
void SoundCache::determineValidSoundIds() noexcept {
    mbValidSoundIdx.reset();

    // Examine all available WAD lumps and see if they are .wav sound lumps (WSND)
    const int32_t numLumps = W_NumLumps();

    for (int32_t lumpIdx = 0;lumpIdx < numLumps; ++lumpIdx) {
        // Is this a .wav format sound lump?
        const WadLumpName lumpName = W_GetLumpName(lumpIdx);
        const bool bIsSoundLumpName = (
            (std::toupper(lumpName.chars[0]) == 'W') &&
            (std::toupper(lumpName.chars[1]) == 'S') &&
            (std::toupper(lumpName.chars[2]) == 'N') &&
            (std::toupper(lumpName.chars[3]) == 'D') &&
            std::isdigit(lumpName.chars[4]) &&
            std::isdigit(lumpName.chars[5]) &&
            std::isdigit(lumpName.chars[6]) &&
            std::isdigit(lumpName.chars[7])
        );

        if (!bIsSoundLumpName)
            continue;

        // Parse the sound id and ensure it is valid
        const uint32_t soundId = (
            (uint32_t)(lumpName.chars[4] - '0') * 1000u +
            (uint32_t)(lumpName.chars[5] - '0') * 100u +
            (uint32_t)(lumpName.chars[6] - '0') * 10u +
            (uint32_t)(lumpName.chars[7] - '0')
        );

        if (soundId > MAX_SOUND_ID)
            continue;

        // If we get to here then this is a valid sound
        mbValidSoundIdx.set(soundId, true);
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Returns the lump index for the specified sound id.
// Returns '-1' if the given sound is not present in any WAD lump.
// Note: this function relies on 'determineValidSoundIds()' being called first to determine which sounds are in WADs.
//----------------------------------------------------------------------------------------------------------------------
int32_t SoundCache::getSoundLumpIdx(const uint16_t soundId) noexcept {
    // Check to see if the sound actually has a WAD lump associated with it and return an invalid index if it doesn't
    const bool bSoundHasWadLump = ((soundId <= MAX_SOUND_ID) && mbValidSoundIdx[soundId]);

    if (!bSoundHasWadLump)
        return -1;

    // Make up the lump name for the sound
    char soundIdStr[8];
    std::snprintf(soundIdStr, sizeof(soundIdStr), "%04u", soundId);
    const WadLumpName lumpName('W', 'S', 'N', 'D', soundIdStr[0], soundIdStr[1], soundIdStr[2], soundIdStr[3]);

    // Return the lump index which SHOULD be valid
    return W_GetNumForName(lumpName);
}

//----------------------------------------------------------------------------------------------------------------------
// Attempts to load/cache a specified sound.
// Returns the index of the sound on success, or if the sound is already loaded.
// Returns 'INVALID_SOUND_IDX' on failure.
//----------------------------------------------------------------------------------------------------------------------
uint16_t SoundCache::cacheAndGetSoundIdx(const uint16_t soundId) noexcept {
    // Can't precache an invalid sound id
    if (soundId > MAX_SOUND_ID)
        return INVALID_SOUND_IDX;

    // If the sound is already loaded then all we need to do is return the sound index
    if (const uint16_t existingSoundIdx = mSoundIdToIdx[soundId]; existingSoundIdx != INVALID_SOUND_IDX)
        return existingSoundIdx;

    // Which WAD lump is this sound in? If it isn't in any lump then we cannot load it
    const int32_t wadLumpIdx = getSoundLumpIdx(soundId);

    if (wadLumpIdx < 0)
        return INVALID_SOUND_IDX;

    // Read the .wav format sound from the WAD lump
    const int32_t wadLumpSize = W_LumpLength(wadLumpIdx);
    AudioData audioData = {};

    if (wadLumpSize > 0) {
        std::unique_ptr<std::byte[]> pWadLumpBytes = std::make_unique<std::byte[]>((size_t) wadLumpSize);
        W_ReadLump(wadLumpIdx, pWadLumpBytes.get(), true);
        audioData = WavReader::readWav_Pcm16(pWadLumpBytes.get(), (size_t) wadLumpSize);
    }

    // Stick the sound in the cache if it loaded successfully.
    // Otherwise, mark the sound as invalid so we don't try to load it again after it fails.
    if (audioData.pData.get()) {
        // Loading succeeded: put the sound in the cache and return the sound index
        if (mFreeSoundSlots.empty()) {
            const uint16_t soundIdx = (uint16_t) mSounds.size();
            mSounds.push_back(std::move(audioData));
            mSoundIdToIdx[soundId] = soundIdx;
            return soundIdx;
        }
        else {
            const uint16_t soundIdx = mFreeSoundSlots.back();
            mFreeSoundSlots.pop_back();
            mSounds[soundIdx] = std::move(audioData);
            mSoundIdToIdx[soundId] = soundIdx;
            return soundIdx;
        }
    }
    else {
        // Loading failed: don't try to load this sound again as it will likely fail again
        mbValidSoundIdx[soundId] = false;
        return INVALID_SOUND_IDX;
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Unload a range of sounds matching a specified predicate.
// Note: the given range of sound ids is expected to be <= MAX_SOUND_ID.
//----------------------------------------------------------------------------------------------------------------------
void SoundCache::unloadSoundRange(
    const uint32_t begSoundId,
    const uint32_t endSoundId,
    bool (*unloadSoundPredicate)(const uint16_t soundId) noexcept
) noexcept
{
    ASSERT(begSoundId <= MAX_SOUND_ID);
    ASSERT(endSoundId <= MAX_SOUND_ID);
    ASSERT(unloadSoundPredicate);

    // Examine all the requested sounds
    for (uint32_t soundId = begSoundId; soundId < endSoundId; ++soundId) {
        // Skip past the sound if it's not loaded
        if (mSoundIdToIdx[soundId] == INVALID_SOUND_IDX)
            continue;

        // Otherwise unload the sound if it matches
        if (unloadSoundPredicate((uint16_t) soundId)) {
            unloadSound((uint16_t) soundId);
        }
    }
}
