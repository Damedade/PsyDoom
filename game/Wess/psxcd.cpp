//------------------------------------------------------------------------------------------------------------------------------------------
// Williams Entertainment Sound System (WESS): PlayStation CD-ROM handling utilities
//
// Note: this module has been almost completely rewritten for PsyDoom due to massive differences in how file I/O and CD audio playback work
// for this port. A lot of the old PsyQ 'LIBCD' code simply did not make sense anymore, hence this is all marked as 'PsyDoom modifications'.
// For the original code consult the version of this file in the 'Old' folder.
//------------------------------------------------------------------------------------------------------------------------------------------
#include "psxcd.h"

#include "Asserts.h"
#include "FatalErrors.h"
#include "psxspu.h"
#include "PsyDoom/Audio/MusicStreamer.h"
#include "PsyDoom/Audio/SpuExtInputMux.h"
#include "PsyDoom/DiscInfo.h"
#include "PsyDoom/DiscReader.h"
#include "PsyDoom/ModMgr.h"
#include "PsyDoom/ProgArgs.h"
#include "PsyDoom/PsxVm.h"
#include "PsyDoom/Utils.h"
#include "Spu.h"

#include <cstring>
#include <mutex>

// PsyDoom: raise the open file limit
#if PSYDOOM_MODS
    static constexpr int32_t MAX_OPEN_FILES = 16;   // Maximum number of open files
#else
    static constexpr int32_t MAX_OPEN_FILES = 4;    // Maximum number of open files
#endif

static constexpr int32_t FADE_TIME_MS = 250;    // Time it takes to fade out CD audio (milliseconds)

// If true then the 'psxcd' module has been initialized
static bool gbPSXCD_IsCdInit;

// Used to hold a file temporarily after opening
static PsxCd_File gPSXCD_cdfile;

// The Music Streamer: used to stream CD audio (among other sources).
// Access to this is guarded by the music streamer mutex.
static MusicStreamer gMusicStreamer;

// The volume for CD music (and other music sources) and whether music, and reverb on music, is enabled.
// Access all these variables is guarded by the music streamer mutex.
static Spu::Volume  gCdMusicVolume          = {};
static bool         gCdMusicEnable          = false;
static bool         gCdMusicReverbEnable    = false;

// The lock for the Music Streamer and a helper to lock/unlock via RAII.
// N.B: this *CANNOT* be held the same time as the SPU lock, otherwise deadlock MIGHT occur!
// The SPU can request audio from the CD and thus needs access to the Music Streamer lock also.
static std::recursive_mutex gMusicStreamerMutex;

struct LockMusicStreamer {
    LockMusicStreamer() noexcept { gMusicStreamerMutex.lock(); }
    ~LockMusicStreamer() noexcept { gMusicStreamerMutex.unlock(); }
};

// Disc readers used for each open file
static DiscReader gFileDiscReaders[MAX_OPEN_FILES] = {
    PsxVm::gDiscInfo, PsxVm::gDiscInfo, PsxVm::gDiscInfo, PsxVm::gDiscInfo,
    PsxVm::gDiscInfo, PsxVm::gDiscInfo, PsxVm::gDiscInfo, PsxVm::gDiscInfo,
    PsxVm::gDiscInfo, PsxVm::gDiscInfo, PsxVm::gDiscInfo, PsxVm::gDiscInfo,
    PsxVm::gDiscInfo, PsxVm::gDiscInfo, PsxVm::gDiscInfo, PsxVm::gDiscInfo,
};

//------------------------------------------------------------------------------------------------------------------------------------------
// A callback invoked by the SPU (via the SPU input multiplexer) when it wants audio from the Music Streamer: returns a single sample
//------------------------------------------------------------------------------------------------------------------------------------------
static Spu::SpuCallbackOutput SpuAudioCallback() noexcept {
    // Lock the Music Streamer while we are doing this.
    // Note that this thread also has the SPU lock at this point too.
    // Therefore the main thread must NOT lock both the Music Streamer and the SPU at the same time, or otherwise a deadlock might occur!
    LockMusicStreamer musicStreamerLock;

    if (!gCdMusicEnable)
        return {};

    // Read the next sample from the Music Streamer
    Spu::SpuCallbackOutput out;
    out.sample = gMusicStreamer.readNextSample();
    out.sample *= gCdMusicVolume;
    out.reverbSample = (gCdMusicReverbEnable) ? out.sample : Spu::StereoSample{};
    return out;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Initialize the WESS (Williams Entertainment Sound System) CD handling module.
//------------------------------------------------------------------------------------------------------------------------------------------
void psxcd_init() noexcept {
    // If we've already done this then just no-op
    if (gbPSXCD_IsCdInit)
        return;

    gbPSXCD_IsCdInit = true;

    // Initialize the SPU and install the CD player as an external input to the SPU (via the multiplexer)
    psxspu_init();
    SpuExtInputMux::addInput(SpuAudioCallback);
    
    // Initialize the Music Streamer
    {
        LockMusicStreamer musicStreamerLock;
        gMusicStreamer.init();
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Shut down the WESS (Williams Entertainment Sound System) CD handling module
//------------------------------------------------------------------------------------------------------------------------------------------
void psxcd_exit() noexcept {
    // Shut down the Music Streamer
    {
        LockMusicStreamer musicStreamerLock;
        gMusicStreamer.shutdown();
    }
    
    // Uninstall the CD player as an external input to the SPU (via the multiplexer)
    SpuExtInputMux::removeInput(SpuAudioCallback);
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Open a specified CD file for reading
//------------------------------------------------------------------------------------------------------------------------------------------
PsxCd_File* psxcd_open(const CdFileId discFile) noexcept {
    // Zero init the temporary file structure
    gPSXCD_cdfile = {};

    // Modding mechanism: allow files to be overridden with user files in a specified directory.
    // Note that we do this check BEFORE validating if the file exists on-disc because PsyDoom now allows Doom format maps (.WAD)
    // to override maps in Final Doom (.ROM files). The .WAD map files of course won't exist on-disc in the case of Final Doom.
    if (ModMgr::areOverridesAvailableForFile(discFile))
        return (ModMgr::openOverridenFile(discFile, gPSXCD_cdfile)) ? &gPSXCD_cdfile : nullptr;

    // Figure out where the file is on disc and sanity check the file is valid
    const PsxCd_MapTblEntry fileTableEntry = CdMapTbl_GetEntry(discFile);

    if (fileTableEntry == PsxCd_MapTblEntry{}) {
        FatalErrors::raiseF("psxcd_open: attempt to open non-existing file '%s'!", discFile.c_str().data());
    }

    // Find a free disc reader slot to accomodate this file
    int32_t discReaderIdx = -1;

    for (int32_t i = 0; i < MAX_OPEN_FILES; ++i) {
        if (!gFileDiscReaders[i].isTrackOpen()) {
            discReaderIdx = i;
            break;
        }
    }

    if (discReaderIdx < 0) {
        FatalErrors::raise("psxcd_open: out of file handles!");
    }

    // Open up the disc reader for it and save it's details
    DiscReader& discReader = gFileDiscReaders[discReaderIdx];

    if (!discReader.setTrackNum(1)) {
        FatalErrors::raise("psxcd_open: failed to open a disc reader for the data track!");
    }

    if (!discReader.trackSeekAbs(fileTableEntry.startSector * CDROM_SECTOR_SIZE)) {
        FatalErrors::raise("psxcd_open: failed to seek to the specified file!");
    }

    gPSXCD_cdfile.size = fileTableEntry.size;
    gPSXCD_cdfile.startSector = fileTableEntry.startSector;
    gPSXCD_cdfile.fileHandle = discReaderIdx + 1;
    return &gPSXCD_cdfile;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Tells if the cdrom is currently seeking to a location for audio playback; the answer for this is always 'false' now for PsyDoom
//------------------------------------------------------------------------------------------------------------------------------------------
bool psxcd_seeking_for_play() noexcept { return false; }

//------------------------------------------------------------------------------------------------------------------------------------------
// Read the specified number of bytes synchronously from the given CD file and returns the number of bytes read
//------------------------------------------------------------------------------------------------------------------------------------------
int32_t psxcd_read(void* const pDest, int32_t numBytes, PsxCd_File& file) noexcept {
    // Modding mechanism: allow files to be overriden with user files in a specified directory
    if (ModMgr::isFileOverriden(file))
        return ModMgr::readFromOverridenFile(pDest, numBytes, file);

    // If the file does not have a valid handle then the read fails
    if ((file.fileHandle <= 0) || (file.fileHandle > MAX_OPEN_FILES))
        return -1;

    // Verify that the read is in bounds for the file and fail if it isn't
    DiscReader& reader = gFileDiscReaders[file.fileHandle - 1];

    const int32_t fileBegByteIdx = file.startSector * CDROM_SECTOR_SIZE;
    const int32_t fileEndByteIdx = fileBegByteIdx + file.size;
    const int32_t curByteIdx = reader.tell();

    if ((curByteIdx < fileBegByteIdx) || (curByteIdx + numBytes > fileEndByteIdx))
        return -1;

    // Do the actual read and return the number of bytes read
    return (reader.read(pDest, numBytes)) ? numBytes : -1;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Seek to a specified position in a file, relatively or absolutely.
// Returns '0' on success, any other value on failure.
//------------------------------------------------------------------------------------------------------------------------------------------
int32_t psxcd_seek(PsxCd_File& file, int32_t offset, const PsxCd_SeekMode mode) noexcept {
    // Modding mechanism: allow files to be overriden with user files in a specified directory
    if (ModMgr::isFileOverriden(file))
        return ModMgr::seekForOverridenFile(file, offset, mode);

    // If the file handle is invalid then the seek fails
    if ((file.fileHandle <= 0) || (file.fileHandle > MAX_OPEN_FILES))
        return -1;

    DiscReader& reader = gFileDiscReaders[file.fileHandle - 1];

    if (mode == PsxCd_SeekMode::SET) {
        // Seek to an absolute position in the file: make sure the offset is valid and try to go to it
        if ((offset < 0) || (offset > file.size))
            return -1;

        return (reader.trackSeekAbs(file.startSector * CDROM_SECTOR_SIZE + offset)) ? 0 : -1;
    }
    else if (mode == PsxCd_SeekMode::CUR) {
        // Seek relative to the current IO position: make sure the offset is valid and try to go to it
        const int32_t curOffset = reader.tell() - file.startSector * CDROM_SECTOR_SIZE;
        const int32_t newOffset = curOffset + offset;

        if ((newOffset < 0) || (newOffset > file.size))
            return -1;

        return (reader.trackSeekRel(offset)) ? 0 : -1;
    }
    else if (mode == PsxCd_SeekMode::END) {
        // Seek relative to the end: make sure the offset is valid and try to go to it
        const int32_t newOffset = file.size - offset;

        if ((newOffset < 0) || (newOffset > file.size))
            return -1;

        return (reader.trackSeekAbs(file.startSector * CDROM_SECTOR_SIZE + newOffset)) ? 0 : -1;
    }

    return -1;  // Bad seek mode!
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Returns the current IO offset within the given file
//------------------------------------------------------------------------------------------------------------------------------------------
int32_t psxcd_tell(const PsxCd_File& file) noexcept {
    // Modding mechanism: allow files to be overriden with user files in a specified directory
    if (ModMgr::isFileOverriden(file))
        return ModMgr::tellForOverridenFile(file);

    // If the file handle is invalid then the tell fails
    if ((file.fileHandle <= 0) || (file.fileHandle > MAX_OPEN_FILES))
        return -1;

    // Tell where we are in the file
    DiscReader& reader = gFileDiscReaders[file.fileHandle - 1];
    return reader.tell() - file.startSector * CDROM_SECTOR_SIZE;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Close a CD file and free up the file slot
//------------------------------------------------------------------------------------------------------------------------------------------
void psxcd_close([[maybe_unused]] PsxCd_File& file) noexcept {
    // Modding mechanism: allow files to be overriden with user files in a specified directory
    if (ModMgr::isFileOverriden(file)) {
        ModMgr::closeOverridenFile(file);
        return;
    }

    // If it's a file on the game CD then close out any open disc readers it has and then zero the struct
    if ((file.fileHandle > 0) && (file.fileHandle <= MAX_OPEN_FILES)) {
        gFileDiscReaders[file.fileHandle - 1].closeTrack();
    }

    file = {};
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Internal helper to eliminate the redundancy between 'psxcd_play_at_andloop' and 'psxcd_play_at'.
// This is a new addition for PsyDoom.
//------------------------------------------------------------------------------------------------------------------------------------------
static void psxcd_play_internal(
    const int32_t track,
    const int32_t vol,
    const int32_t fadeUpTime,
    const bool bLoop,
    const int32_t loopTrack
) noexcept {
    // Ignore the command in headless mode
    if (ProgArgs::gbHeadlessMode)
        return;

    // Switch to the specified track and if it fails then stop
    bool bStartedPlayingTrack;

    {
        // N.B: don't hold this lock in the main thread at the same time as the SPU lock - otherwise deadlock might occur!
        LockMusicStreamer musicStreamerLock;
        bStartedPlayingTrack = gMusicStreamer.playTrack(track, (bLoop) ? loopTrack : 0);
    }

    if (!bStartedPlayingTrack) {
        psxcd_stop();
        return;
    }

    // Start mixing in CD audio set the volume level and start fading (if requested)
    psxspu_setcdmixon();

    if (fadeUpTime <= 0) {
        psxspu_set_cd_vol(vol);
        psxspu_stop_cd_fade();
    } else {
        psxspu_set_cd_vol(0);
        psxspu_start_cd_fade(fadeUpTime, vol);
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Play the given cd track and loop another track afterwards using the specified parameters.
//
//  track:              The track to play
//  vol:                Track volume
//  sectorOffset:       To start past the normal track start
//                      NOTE: This is IGNORED by PsyDoom and was always '0' in PSX Doom.
//  fadeUpTime:         Milliseconds to fade in the track, or '0' if instant play.
//  loopTrack:          What track to play in loop after this track ends
//  loopVol:            What volume to play that looped track at.
//                      NOTE: This is IGNORED by PsyDoom and was always the same as the original volume in PSX DOOM.
//  loopSectorOffset:   What sector offset to use for the looped track
//                      NOTE: This is IGNORED by PsyDoom and was always '0' in PSX Doom.
//  loopFadeUpTime:     Fade up time for the looped track.
//                      NOTE: This is IGNORED by PsyDoom and was always '0' in PSX Doom.
//------------------------------------------------------------------------------------------------------------------------------------------
void psxcd_play_at_andloop(
    const int32_t track,
    const int32_t vol,
    [[maybe_unused]] const int32_t sectorOffset,
    const int32_t fadeUpTime,
    const int32_t loopTrack,
    [[maybe_unused]] const int32_t loopVol,
    [[maybe_unused]] const int32_t loopSectorOffset,
    [[maybe_unused]] const int32_t loopFadeUpTime
) noexcept {
    // PsyDoom: to simplify threading and very messy synchronization in the CD audio callback these fields are no longer supported.
    // Setting them to values other than this will no longer work! That's OK because Doom always followed these usage patterns:
    ASSERT(loopVol == vol);
    ASSERT(loopFadeUpTime == 0);
    
    // PsyDoom: not supporting sector offsets for starting playback and looping anymore.
    // This makes integrating new audio sources such as Ogg Vorbis easier.
    // These offsets were always '0' in the original PSX Doom anyway...
    ASSERT(sectorOffset == 0);
    ASSERT(loopSectorOffset == 0);

    psxcd_play_internal(track, vol, fadeUpTime, true, loopTrack);
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Begin playint the specified cd track at the given volume level.
// A sector offset can also be specified to begin from a certain location in the track.
//------------------------------------------------------------------------------------------------------------------------------------------
void psxcd_play_at(const int32_t track, const int32_t vol, [[maybe_unused]] const int32_t sectorOffset) noexcept {
    // PsyDoom: not supporting sector offsets for starting playback and looping anymore.
    // This makes integrating new audio sources such as Ogg Vorbis easier.
    // These offsets were always '0' in the original PSX Doom anyway...
    ASSERT(sectorOffset == 0);

    psxcd_play_internal(track, vol, 0, false, 0);
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Play the given audio track at the specified volume level
//------------------------------------------------------------------------------------------------------------------------------------------
void psxcd_play(const int32_t track, const int32_t vol) noexcept {
    psxcd_play_at(track, vol, 0);
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Stop playback of cd audio; unlike 'psxcd_pause' playback CANNOT be resumed by calling 'psxcd_restart' afterwards
//------------------------------------------------------------------------------------------------------------------------------------------
void psxcd_stop() noexcept {
    // Quickly fade out cd audio if playing
    bool bMightNeedFade = false;

    {
        // N.B: don't hold this lock in the main thread at the same time as the SPU lock - otherwise deadlock might occur!
        LockMusicStreamer musicStreamerLock;
        bMightNeedFade = gMusicStreamer.isTrackPlayingAndUnpaused();
    }

    if (bMightNeedFade) {
        const int32_t startCdVol = psxspu_get_cd_vol();

        if (startCdVol != 0) {
            psxspu_start_cd_fade(FADE_TIME_MS, 0);
            Utils::waitForCdAudioFadeOut();
        }
    }

    // Close the current music stream
    {
        // N.B: don't hold this lock in the main thread at the same time as the SPU lock - otherwise deadlock might occur!
        LockMusicStreamer musicStreamerLock;
        gMusicStreamer.stop();
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Pause cd audio playback and make a note of where we paused at
//------------------------------------------------------------------------------------------------------------------------------------------
void psxcd_pause() noexcept {
    // Quickly fade out cd audio if playing
    bool bMightNeedFade = false;

    {
        // N.B: don't hold this lock in the main thread at the same time as the SPU lock - otherwise deadlock might occur!
        LockMusicStreamer musicStreamerLock;
        bMightNeedFade = gMusicStreamer.isTrackPlayingAndUnpaused();
    }

    if (bMightNeedFade) {
        const int32_t startCdVol = psxspu_get_cd_vol();

        if (startCdVol != 0) {
            psxspu_start_cd_fade(FADE_TIME_MS, 0);
            Utils::waitForCdAudioFadeOut();
        }
    }

    // Pause the current music stream
    {
        // N.B: don't hold this lock in the main thread at the same time as the SPU lock - otherwise deadlock might occur!
        LockMusicStreamer musicStreamerLock;
        gMusicStreamer.pause();
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Restart cd audio playback: playback resumes from where the cd was last paused
//------------------------------------------------------------------------------------------------------------------------------------------
void psxcd_restart(const int32_t vol) noexcept {
    // Resume playing the current music stream.
    // Note: this won't do anything if nothing is actually playing, hence no status checking here.
    {
        // N.B: don't hold this lock in the main thread at the same time as the SPU lock - otherwise deadlock might occur!
        LockMusicStreamer musicStreamerLock;
        gMusicStreamer.resume();
    }

    // Set the audio volume
    psxspu_set_cd_vol(vol);
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Tells how many sectors have elapsed during cd playback
//------------------------------------------------------------------------------------------------------------------------------------------
int32_t psxcd_elapsed_sectors() noexcept {
    // Helpful constants
    constexpr uint32_t CD_SECTOR_SIZE = 2352;
    constexpr uint32_t STEREO_SAMPLE_SIZE = sizeof(int16_t) * 2;
    constexpr uint32_t STEREO_SAMPLES_PER_SECTOR = CD_SECTOR_SIZE / STEREO_SAMPLE_SIZE;
    
    // N.B: don't hold this lock in the main thread at the same time as the SPU lock - otherwise deadlock might occur!
    LockMusicStreamer lockMusicStreamer;
    const size_t elapsedSamples = gMusicStreamer.getCurrentStereoSampleIndex();
    const size_t elapsedSectors = elapsedSamples / STEREO_SAMPLES_PER_SECTOR;
    return (int32_t) elapsedSectors;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Get the size of a file
//------------------------------------------------------------------------------------------------------------------------------------------
int32_t psxcd_get_file_size(const CdFileId discFile) noexcept {
    // Modding mechanism: allow files to be overriden with user files in a specified directory
    if (ModMgr::areOverridesAvailableForFile(discFile))
        return ModMgr::getOverridenFileSize(discFile);

    return CdMapTbl_GetEntry(discFile).size;
}

int32_t psxcd_get_playing_track() noexcept {
    // N.B: don't hold this lock in the main thread at the same time as the SPU lock - otherwise deadlock might occur!
    LockMusicStreamer lockMusicStreamer;
    return gMusicStreamer.getCurrentTrack();
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Update the settings for music playback
//------------------------------------------------------------------------------------------------------------------------------------------
void psxcd_set_playback_attribs(
    const std::optional<bool> bMusicEnable,
    const std::optional<bool> bMusicReverbEnable,
    const std::optional<int16_t> musicVolumeL,
    const std::optional<int16_t> musicVolumeR
) noexcept
{
    LockMusicStreamer musicStreamerLock;

    if (bMusicEnable.has_value()) {
        gCdMusicEnable = bMusicEnable.value();
    }

    if (bMusicReverbEnable.has_value()) {
        gCdMusicReverbEnable = bMusicReverbEnable.value();
    }

    if (musicVolumeL.has_value()) {
        gCdMusicVolume.left = musicVolumeL.value();
    }

    if (musicVolumeR.has_value()) {
        gCdMusicVolume.right = musicVolumeR.value();
    }
}
