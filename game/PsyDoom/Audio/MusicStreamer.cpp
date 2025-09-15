#include "MusicStreamer.h"

#include "Asserts.h"
#include "Endian.h"
#include "PsyDoom/DiscInfo.h"
#include "PsyDoom/DiscReader.h"
#include "PsyDoom/ModMgr.h"
#include "PsyDoom/ProgArgs.h"
#include "PsyDoom/PsxVm.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string_view>

BEGIN_DISABLE_HEADER_WARNINGS
    #include <vorbis/vorbisfile.h>
END_DISABLE_HEADER_WARNINGS

//------------------------------------------------------------------------------------------------------------------------------------------
// Represents a music source from a CD 'Digital Audio' track on the current game disc.
// Most of the time (outside of mods) this is what will be used to play streaming music.
//------------------------------------------------------------------------------------------------------------------------------------------
class MusicSource_CDDA : public IMusicSource {
public:
    static constexpr uint32_t CDDA_SECTOR_SIZE = 2352;
    static constexpr uint32_t MONO_SAMPLE_SIZE = sizeof(int16_t);
    static constexpr uint32_t NUM_BUFFER_SECTORS = 1;
    static constexpr uint32_t NUM_BUFFER_MONO_SAMPLES = (CDDA_SECTOR_SIZE * NUM_BUFFER_SECTORS) / MONO_SAMPLE_SIZE;

    static_assert(NUM_BUFFER_MONO_SAMPLES % 2 == 0);

    MusicSource_CDDA() noexcept
        : mDiscReader(PsxVm::gDiscInfo)
        , mBufferOffset(NUM_BUFFER_MONO_SAMPLES)
        , mBuffer{}
    {
    }

    virtual ~MusicSource_CDDA() noexcept override {
        closeTrack();
    }

    // Attempts to open the specified CD-DA track for reading and returns 'true' if successful
    bool openTrack(const int32_t trackNum) noexcept {
        const bool bSuccess = mDiscReader.setTrackNum(trackNum);
        invalidateBuffer();
        return bSuccess;
    }

    // Closes the current CD-DA track
    void closeTrack() noexcept {
        mDiscReader.closeTrack();
        invalidateBuffer();
    }

    // Attempts to get the next sample from the music track
    std::tuple<Spu::StereoSample, ReadSampleResult> readNextSample() noexcept override {
        // No disc track open? If so then return silence and 'END_OF_STREAM':
        if (!mDiscReader.isTrackOpen())
            return { Spu::StereoSample{}, ReadSampleResult::END_OF_STEAM };

        // Check if we need to read any more data into the internal buffer firstly (because it has been exhausted)
        if (mBufferOffset + 1 >= (int32_t) NUM_BUFFER_MONO_SAMPLES) {
            // There is no data left in the internal buffer - need to read some more.
            // But is there any data left in the track itself to read? If not then return silence and 'END_OF_STREAM'.
            const DiscTrack* const pTrack = mDiscReader.getOpenTrack();
            const int32_t trackSize = pTrack->trackPayloadSize;
            const int32_t trackOffset = mDiscReader.tell();

            if (trackOffset >= trackSize)
                return { Spu::StereoSample{}, ReadSampleResult::END_OF_STEAM };

            // Read what we can and zero anything we can't (in case the last sector is short for some reason)
            const uint32_t samplesToRead = std::min<uint32_t>((uint32_t)(trackSize - trackOffset) / MONO_SAMPLE_SIZE, NUM_BUFFER_MONO_SAMPLES);
            const uint32_t samplesToZero = NUM_BUFFER_MONO_SAMPLES - samplesToRead;

            if (!mDiscReader.read(mBuffer, samplesToRead * MONO_SAMPLE_SIZE)) {
                invalidateBuffer();
                return { Spu::StereoSample{}, ReadSampleResult::ERROR }; // Uh-oh! Let the caller decide how to handle it..
            }

            if (samplesToZero > 0) {
                std::memset(&mBuffer[samplesToRead], 0, (size_t) samplesToZero * MONO_SAMPLE_SIZE);
            }

            mBufferOffset = 0;  // We have a fresh buffer full of samples!
        }

        // Should have at least 2 samples in the buffer at this point, return the requested stereo sample:
        ASSERT(mBufferOffset + 2 <= (int32_t) NUM_BUFFER_MONO_SAMPLES);
        const Spu::StereoSample sample = { mBuffer[mBufferOffset], mBuffer[mBufferOffset + 1] };
        mBufferOffset += 2;

        return { sample, ReadSampleResult::OK };
    }

    // Gets the current track position in terms of stereo samples
    virtual size_t getCurrentStereoSampleIndex() const noexcept override {
        const uint32_t numBytesBuffered = (mDiscReader.isTrackOpen()) ? mDiscReader.tell() : 0;

        if (numBytesBuffered < sizeof(mBuffer))
            return 0;

        const size_t numBytesConsumed = numBytesBuffered - sizeof(mBuffer) + mBufferOffset;
        const size_t monoSampleIndex = numBytesConsumed / MONO_SAMPLE_SIZE;
        return monoSampleIndex / 2u;
    }

    // Rewinds the current track to the beginning
    virtual bool rewind() noexcept override {
        if (!mDiscReader.isTrackOpen())
            return false;

        invalidateBuffer();
        return mDiscReader.trackSeekAbs(0);
    }

private:
    // Invalidates the contents of the buffer and marks that it must be filled again
    void invalidateBuffer() noexcept {
        mBufferOffset = NUM_BUFFER_MONO_SAMPLES;
    }

    DiscReader  mDiscReader;                        // The disc reader used to stream the audio
    int32_t     mBufferOffset;                      // Current read position in the audio buffer
    int16_t     mBuffer[NUM_BUFFER_MONO_SAMPLES];   // The CD audio buffer: we read CD audio in chunks
};

//------------------------------------------------------------------------------------------------------------------------------------------
// Represents a music source from an Ogg Vorbis file on disc.
// This can be used for modding purposes to add new music to levels.
//------------------------------------------------------------------------------------------------------------------------------------------
class MusicSource_OggVorbis : public IMusicSource {
public:
    static constexpr uint32_t MAX_BUFFER_SIZE = 1024 * 16; // 16KiB
    static constexpr uint32_t STEREO_SAMPLE_SIZE = sizeof(int16_t) * 2;
    static constexpr uint32_t MAX_BUFFER_STEREO_SAMPLES = MAX_BUFFER_SIZE / STEREO_SAMPLE_SIZE;

    static_assert(MAX_BUFFER_SIZE % STEREO_SAMPLE_SIZE == 0);

    MusicSource_OggVorbis() noexcept
        : mVorbisFile{}
        , mpVorbisInfo(nullptr)
        , mSampleIndex(0)
        , mBufferSize(0)
        , mBufferOffset(0)
        , mLoopStart(0)
        , mLoopEnd(SIZE_MAX)
        , mBuffer{}
    {
    }

    virtual ~MusicSource_OggVorbis() noexcept override {
        closeTrack();
    }

    // Attempts to open an Ogg Vorbis file at the given path as the current track.
    // Returns 'true' if that was succesful.
    bool openTrack(const char* const filePath) noexcept {
        // Close up the current track (if any)
        closeTrack();

        // Open the file itself and abort if failed
        FILE* const pFile = std::fopen(filePath, "rb");

        if (pFile == nullptr)
            return false;

        if (ov_open(pFile, &mVorbisFile, nullptr, 0) < 0) {
            std::fclose(pFile);
            return false;
        }

        // Get the metadata for the track and abort if failed
        mpVorbisInfo = ov_info(&mVorbisFile, -1);

        if (mpVorbisInfo == nullptr) {
            closeTrack();
            return false;
        }

        // Verify the file is in the correct format.
        // Only 2 channel 44.1KHz audio is supported!
        const bool bValidFormat = (
            (mpVorbisInfo->channels == 2) &&
            (mpVorbisInfo->rate == 44100)
        );

        if (!bValidFormat) {
            closeTrack();
            return false;
        }

        // Parse any info controlling looping
        parseLoopMetadata(*mpVorbisInfo);
        return true;
    }

    // Closes the currently opened Ogg Vorbis file (if any)
    void closeTrack() noexcept {
        if (mpVorbisInfo) {
            ov_clear(&mVorbisFile);
            mVorbisFile = {};
            mpVorbisInfo = nullptr;
        }

        mSampleIndex = 0;
        invalidateBuffer();
    }

    // Attempts to get the next sample from the music track
    std::tuple<Spu::StereoSample, ReadSampleResult> readNextSample() noexcept override {
        // No track open? If so then return silence and 'END_OF_STREAM':
        if (mpVorbisInfo == nullptr)
            return { Spu::StereoSample{}, ReadSampleResult::END_OF_STEAM };

        // Reached the end of the looping region?
        if (mSampleIndex >= mLoopEnd)
            return { Spu::StereoSample{}, ReadSampleResult::END_OF_STEAM };

        // Check if we need to read any more data into the internal buffer firstly (because it has been exhausted)
        if (mBufferOffset + STEREO_SAMPLE_SIZE > mBufferSize) {
            // Fill the buffer and abort if failed
            if (!fillAudioBuffer()) {
                invalidateBuffer();
                return { Spu::StereoSample{}, ReadSampleResult::ERROR }; // Uh-oh! Let the caller decide how to handle it..
            }

            // If there is no data left then we have reached EOF
            if (mBufferSize < STEREO_SAMPLE_SIZE)
                return { Spu::StereoSample{}, ReadSampleResult::END_OF_STEAM };
        }

        // Should have at least 2 samples in the buffer at this point, return the requested stereo sample:
        ASSERT(mBufferOffset + STEREO_SAMPLE_SIZE <= mBufferSize);

        int16_t channelSamples[2];
        std::memcpy(channelSamples, mBuffer + mBufferOffset, sizeof(int16_t) * 2);
        const Spu::StereoSample sample = { channelSamples[0], channelSamples[1] };
        mBufferOffset += STEREO_SAMPLE_SIZE;
        mSampleIndex++;

        return { sample, ReadSampleResult::OK };
    }

    // Gets the current track position in terms of stereo samples
    virtual size_t getCurrentStereoSampleIndex() const noexcept override {
        return mSampleIndex;
    }

    // Rewinds the current track to the loop start point, which will the beginning of the track if none is specified
    virtual bool rewind() noexcept override {
        if (mpVorbisInfo == nullptr)
            return false;

        invalidateBuffer();
        mSampleIndex = mLoopStart;
        return (ov_pcm_seek(&mVorbisFile, mLoopStart) == 0);
    }

private:
    // Invalidates the contents of the buffer and marks that it must be filled again
    void invalidateBuffer() noexcept {
        mBufferSize = 0;
        mBufferOffset = 0;
    }

    // Attempts to fill the audio buffer as much as possible.
    // Returns 'false' if there was an error.
    bool fillAudioBuffer() noexcept {
        // Empty the buffer
        mBufferSize = 0;
        mBufferOffset = 0;

        // Keep reading until there is an error or EOF (result == 0)
        while (true) {
            [[maybe_unused]] int curBitstreamOut = {};
            const int maxReadSize  = MAX_BUFFER_SIZE - mBufferSize;
            const long readResult = ov_read(
                &mVorbisFile,
                mBuffer + mBufferSize,
                maxReadSize,
                (Endian::isBig()) ? 1 : 0,
                sizeof(int16_t),
                1, // Signed data format
                &curBitstreamOut
            );

            // A result of less than zero means an error
            if (readResult < 0)
                return false;

            // A result of '0' means EOF
            if (readResult == 0)
                return true;

            // Every other result is the number of bytes filled into the buffer
            mBufferSize += (uint32_t) readResult;

            // Is the buffer now full?
            if (mBufferSize >= MAX_BUFFER_SIZE)
                return true;
        }

        // Should never reach here but if we do assume failure
        return false;
    }

    // Parse metadata specifying where to loop in the Ogg Vorbis file
    void parseLoopMetadata(const vorbis_info& vorbisInfo) noexcept {
        // Get all the comments in the vorbis file
        const vorbis_comment* const pVorbisComment = ov_comment(&mVorbisFile, -1);

        if (pVorbisComment == nullptr)
            return;

        // Keep the 'LOOP_LENGTH' metadata here if it is found
        std::optional<size_t> loopLength;

        // Iterate through all comments and find ones describing looping
        const char* const* const pComments = pVorbisComment->user_comments;
        const int numComments = pVorbisComment->comments;
        const int* pCommentLengths = pVorbisComment->comment_lengths;
        char ucaseCommentBuf[512];

        for (int i = 0; i < numComments; ++i) {
            // Is this comment too big to process or does it have a bad length?
            const int commentLength = pCommentLengths[i];

            if ((commentLength <= 0) || (commentLength > (int) C_ARRAY_SIZE(ucaseCommentBuf)))
                continue;

            // Otherwise uppercase it and turn it into a string view
            const char* const pCommentStart = pComments[i];
            const char* const pCommentEnd = pCommentStart + commentLength;
            std::transform(pCommentStart, pCommentEnd, ucaseCommentBuf, std::toupper);
            const std::string_view ucaseComment(ucaseCommentBuf, pCommentEnd - pCommentStart);

            // The comment should that the format 'NAME=VALUE' - split up the fields:
            const size_t firstEqualsIdx = ucaseComment.find_first_of('=');

            if (firstEqualsIdx == std::string_view::npos)
                continue;

            const std::string_view commentName = ucaseComment.substr(0, firstEqualsIdx);
            const std::string_view commentValue = ucaseComment.substr(firstEqualsIdx + 1);

            // Now see if it's one of the looping control fields
            if ((commentName == "LOOP_START") || (commentName == "LOOPSTART")) {
                std::optional<size_t> timestamp = parseTimestamp(commentValue, vorbisInfo);

                if (timestamp.has_value()) {
                    mLoopStart = timestamp.value();
                }
            }
            else if ((commentName == "LOOP_END") || (commentName == "LOOPEND")) {
                std::optional<size_t> timestamp = parseTimestamp(commentValue, vorbisInfo);

                if (timestamp.has_value()) {
                    mLoopEnd = timestamp.value();
                }
            }
            else if ((commentName == "LOOP_LENGTH") || (commentName == "LOOPLENGTH")) {
                loopLength = parseTimestamp(commentValue, vorbisInfo);
            }
        }

        // Did we get a loop length? If so then use that compute the loop end:
        if (loopLength.has_value()) {
            mLoopEnd = mLoopStart + loopLength.value();
        }
    }

    // Attempts to parse a timestamp. Used to parse looping timestamps from metadata in the Ogg Vorbis file.
    // The timestamp can either be specified either in terms of PCM samples (a simple integer), or in the format 'HH:MM:SS.ss'.
    // If a ':' or '.' is specified then the format is assumed to be 'HH:MM:SS.ss'.
    //
    // If parsing is successful then the timestamp is returned in terms of PCM samples.
    static std::optional<size_t> parseTimestamp(const std::string_view timestampStr, const vorbis_info& vorbisInfo) noexcept {
        // Which format is it?
        if (timestampStr.find_first_of(":.") != std::string_view::npos) {
            // Split up the string into substrings for hours, minutes, seconds and fractional seconds
            std::string_view hoursStr;
            std::string_view minutesStr;
            std::string_view secondsStr = timestampStr;
            std::string_view fracSecondsStr;

            if (const size_t colonIdx = secondsStr.find_last_of(':'); colonIdx != std::string_view::npos) {
                minutesStr = secondsStr.substr(0, colonIdx);
                secondsStr = secondsStr.substr(colonIdx + 1);
            }

            if (const size_t decimalPtIdx = secondsStr.find_first_of('.'); decimalPtIdx != std::string_view::npos) {
                fracSecondsStr = secondsStr.substr(decimalPtIdx + 1);
                secondsStr = secondsStr.substr(0, decimalPtIdx);
            }

            if (const size_t colonIdx = minutesStr.find_last_of(':'); colonIdx != std::string_view::npos) {
                hoursStr = minutesStr.substr(0, colonIdx);
                minutesStr = minutesStr.substr(colonIdx + 1);
            }

            // Now convert the time into seconds.
            // Note: there must be at least a seconds string!
            double timestampInSeconds = 0.0;

            if (const std::optional<size_t> seconds = parseUint(secondsStr); seconds.has_value()) {
                timestampInSeconds += (double) seconds.value();
            } else {
                return {}; // Parse error!
            }

            if (!fracSecondsStr.empty()) {
                if (const std::optional<size_t> fracSeconds = parseUint(fracSecondsStr); fracSeconds.has_value()) {
                    timestampInSeconds += (double) fracSeconds.value() / std::pow(10.0, (double) fracSecondsStr.length());
                } else {
                    return {}; // Parse error!
                }
            }

            if (!minutesStr.empty()) {
                if (const std::optional<size_t> minutes = parseUint(minutesStr); minutes.has_value()) {
                    timestampInSeconds += (double) minutes.value() * 60.0;
                } else {
                    return {}; // Parse error!
                }
            }

            if (!hoursStr.empty()) {
                if (const std::optional<size_t> hours = parseUint(hoursStr); hours.has_value()) {
                    timestampInSeconds += (double) hours.value() * (60.0 * 60.0);
                } else {
                    return {}; // Parse error!
                }
            }

            // Now that we the time in seconds, convert it to a sample index using the sample rate
            const size_t timestampInSamples = (size_t)(timestampInSeconds * (double) vorbisInfo.rate);
            return timestampInSamples;
        }
        else {
            // Simple PCM sample count as an unsigned integer
            return parseUint(timestampStr);
        }
    }

    // Parse an unsigned integer from the given std::string_view. Returns nothing on failure.
    // Note: if the string is empty then that is interpreted as '0'.
    static std::optional<size_t> parseUint(const std::string_view str) noexcept {
        // Get some info and sanity check the string is not too big
        const size_t strLen = str.length();
        char numBuffer[256];

        if (strLen > C_ARRAY_SIZE(numBuffer) - 1u)
            return {};

        // Empty string is always '0'
        if (strLen == 0)
            return size_t(0);

        // Sanity check the string doesn't contain illegal characters
        if (str.find_first_not_of("0123456789, ") != std::string_view::npos)
            return {};

        // Copy the string to the buffer and null terminate.
        // Ignore any ',' or ' ' characters as thousand separators.
        size_t numStrLen = 0;

        for (const char c : str) {
            if ((c != ',') && (c != ' ')) {
                numBuffer[numStrLen++] = c;
            }
        }

        numBuffer[numStrLen] = 0;

        // Now convert the number to an integer and return
        return (size_t) std::atoi(numBuffer);
    }

    OggVorbis_File  mVorbisFile;                // The Ogg Vorbis stream
    vorbis_info*    mpVorbisInfo;               // Metadata for the Ogg Vorbis stream
    size_t          mSampleIndex;               // Index of the next sample to be read
    uint32_t        mBufferSize;                // How much data has been put into the buffer
    uint32_t        mBufferOffset;              // Current read position in the audio buffer
    size_t          mLoopStart;                 // Sample to start looping at (defaults to '0')
    size_t          mLoopEnd;                   // Sample to end looping at. If past the end of the stream then looping will happen at the end of the stream.
    char            mBuffer[MAX_BUFFER_SIZE];   // The audio buffer
};

//------------------------------------------------------------------------------------------------------------------------------------------
// MusicStreamer ctor/dtor
//------------------------------------------------------------------------------------------------------------------------------------------
MusicStreamer::MusicStreamer() noexcept
    : mCurTrack(0)
    , mLoopTrack(0)
    , mbPaused(false)
    , mpMusicSource(nullptr)
    , mOggVorbisTracks()
{
}

MusicStreamer::~MusicStreamer() noexcept {
    stop();
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Initializes the music streamer
//------------------------------------------------------------------------------------------------------------------------------------------
void MusicStreamer::init() noexcept {
    // Temporary string buffer
    std::string tempStr;
    tempStr.reserve(1023);

    // Determine the paths to all music tracks implemented as Ogg Vorbis files
    mOggVorbisTracks.clear();

    ModMgr::enumOggVorbisMusicFiles(
        [&](const int32_t trackNum, const CdFileId& fileId) noexcept {
            tempStr = ProgArgs::gDataDirPath;
            tempStr.push_back('/');
            tempStr += std::string_view(fileId.chars, fileId.length());

            mOggVorbisTracks.push_back(OggVorbisTrack{ trackNum, tempStr });
        }
    );
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Reverse call to 'init()': shuts down the music streamer and frees up some resources
//------------------------------------------------------------------------------------------------------------------------------------------
void MusicStreamer::shutdown() noexcept {
    stop();
    mOggVorbisTracks.clear();
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Attempts to play the specified track from the beginning and returns 'true' on success.
// For looping, a loop track can also be specified.
//------------------------------------------------------------------------------------------------------------------------------------------
bool MusicStreamer::playTrack(const int32_t trackNum, const int32_t loopTrackNum) noexcept {
    // Stop whatever is currently playing
    stop();

    // Bad track number?
    const bool bTrackNumValid = ((trackNum > 0) && (trackNum <= UINT16_MAX));
    const bool bLoopTrackNumValid = ((trackNum > 0) && (trackNum <= UINT16_MAX));

    if (!bTrackNumValid)
        return false;

    mCurTrack = (uint16_t) trackNum;
    mLoopTrack = (bLoopTrackNumValid) ? (uint16_t) loopTrackNum : uint16_t(0);
    mbPaused = false;

    // Attempt to start playing the current track
    const bool bSuccess = playCurrentTrack();

    if (!bSuccess) {
        stop();
    }

    return bSuccess;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Returns the track number for the currently playing track, or '0' if no track is playing
//------------------------------------------------------------------------------------------------------------------------------------------
uint16_t MusicStreamer::getCurrentTrack() const noexcept {
    return mCurTrack;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Tells if a music track is currently open for playing.
// Returns 'true' if this is the case, even if the music is paused.
//------------------------------------------------------------------------------------------------------------------------------------------
bool MusicStreamer::isTrackPlaying() const noexcept {
    return (mpMusicSource.get() != nullptr);
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Helper: returns 'true' if the music track is open for playback and is currently unpaused
//------------------------------------------------------------------------------------------------------------------------------------------
bool MusicStreamer::isTrackPlayingAndUnpaused() const noexcept {
    return (mpMusicSource && (!mbPaused));
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Stop playing the current music track and close up any streams
//------------------------------------------------------------------------------------------------------------------------------------------
void MusicStreamer::stop() noexcept {
    mCurTrack = 0;
    mLoopTrack = 0;
    mbPaused = false;
    mpMusicSource.reset();
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Pause the current music track and return 'true' if successful
//------------------------------------------------------------------------------------------------------------------------------------------
bool MusicStreamer::pause() noexcept {
    if (!mpMusicSource)
        return false;

    mbPaused = true;
    return true;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Returns 'true' if the current music track is paused
//------------------------------------------------------------------------------------------------------------------------------------------
bool MusicStreamer::isPaused() const noexcept {
    return mbPaused;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Resume playing the current music track and return 'true' if successful
//------------------------------------------------------------------------------------------------------------------------------------------
bool MusicStreamer::resume() noexcept {
    if (!mpMusicSource)
        return false;

    mbPaused = false;
    return true;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Attempts to read the next sample from the current music stream
//------------------------------------------------------------------------------------------------------------------------------------------
Spu::StereoSample MusicStreamer::readNextSample() noexcept {
    // No music source? If so then return silence:
    if (!mpMusicSource)
        return Spu::StereoSample{};

    // If the current track is paused then return silence
    if (mbPaused)
        return Spu::StereoSample{};

    // Attempt to read the next sample from the music source
    const auto [sample, result] = mpMusicSource->readNextSample();

    // This is the usual happy path: reading the sample was successful
    if (result == IMusicSource::ReadSampleResult::OK)
        return sample;

    // If we reached the end of the stream then try to loop around when there is a loop track specified
    if ((result == IMusicSource::ReadSampleResult::END_OF_STEAM) && (mLoopTrack > 0)) {
        bool bLoopSuccess;

        if (mLoopTrack == mCurTrack) {
            // Normal case: we are playing the same music track to loop, just rewind the current music source.
            // If that fails then close up the music stream.
            bLoopSuccess = mpMusicSource->rewind();
        }
        else {
            // Playing a different music track to loop.
            // Switch to that track now, and if this process fails then close up the music stream.
            mCurTrack = mLoopTrack;
            bLoopSuccess = playCurrentTrack();
        }

        // If looping was successful then try to get a sample again.
        // If THAT read fails then just defer to the error handling (or end of stream) logic below.
        if (bLoopSuccess) {
            const auto [sample2, result2] = mpMusicSource->readNextSample();

            if (result2 == IMusicSource::ReadSampleResult::OK)
                return sample2;
        }
    }

    // If we reached here then an error occurred or we simply reached the end of the stream with no looping.
    // In this case close up the music stream and return silence.
    stop();
    return Spu::StereoSample{};
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Returns where we are in the music stream
//------------------------------------------------------------------------------------------------------------------------------------------
size_t MusicStreamer::getCurrentStereoSampleIndex() noexcept {
    return (mpMusicSource) ? mpMusicSource->getCurrentStereoSampleIndex() : size_t(0);
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Attempts to play the current track number from the beginning and returns 'true' on success
//------------------------------------------------------------------------------------------------------------------------------------------
bool MusicStreamer::playCurrentTrack() noexcept {
    // Search for the current track number among the available Ogg Vorbis tracks.
    // If found then play the Ogg Vorbis file.
    for (const OggVorbisTrack& track : mOggVorbisTracks) {
        // Is this the track we want?
        if (track.trackNum != mCurTrack)
            continue;

        // This is the correct track, play the music:
        std::unique_ptr<MusicSource_OggVorbis> pSource_vorbis = std::make_unique<MusicSource_OggVorbis>();

        if (pSource_vorbis->openTrack(track.filePath.c_str())) {
            mpMusicSource.reset(pSource_vorbis.release());
            return true;
        }
    }

    // Usual case: try to play the track as a regular CD-DA track
    std::unique_ptr<MusicSource_CDDA> pSource_cdda = std::make_unique<MusicSource_CDDA>();

    if (pSource_cdda->openTrack(mCurTrack)) {
        mpMusicSource.reset(pSource_cdda.release());
        return true;
    }
    else {
        return false;
    }
}
