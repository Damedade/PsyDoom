#include "MusicStreamer.h"

#include "Asserts.h"
#include "DiscInfo.h"
#include "DiscReader.h"
#include "PsxVm.h"

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
        if (mBufferOffset + 1 >= NUM_BUFFER_MONO_SAMPLES) {
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
        ASSERT(mBufferOffset + 2 <= NUM_BUFFER_MONO_SAMPLES);
        const Spu::StereoSample sample = { mBuffer[mBufferOffset], mBuffer[mBufferOffset + 1] };
        mBufferOffset += 2;
        
        return { sample, ReadSampleResult::OK };
    }
    
    // Get the current track position in terms of stereo samples
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
            
        mDiscReader.trackSeekAbs(0);
        invalidateBuffer();
        return true;
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
// MusicStreamer ctor/dtor
//------------------------------------------------------------------------------------------------------------------------------------------
MusicStreamer::MusicStreamer() noexcept
    : mCurTrack(0)
    , mLoopTrack(0)
    , mbPaused(false)
    , mpMusicSource(nullptr)
{
}

MusicStreamer::~MusicStreamer() noexcept {
    stop();
}
    
//------------------------------------------------------------------------------------------------------------------------------------------
// Initializes the music streamer
//------------------------------------------------------------------------------------------------------------------------------------------
void MusicStreamer::init() noexcept {
    // Nothing to do here yet...
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Reverse call to 'init()': shuts down the music streamer and frees up some resources
//------------------------------------------------------------------------------------------------------------------------------------------
void MusicStreamer::shutdown() noexcept {
    stop();
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
    // Create and initialize the music source
    {
        std::unique_ptr<MusicSource_CDDA> pSource_cdda = std::make_unique<MusicSource_CDDA>();
        
        if (pSource_cdda->openTrack(mCurTrack)) {
            mpMusicSource.reset(pSource_cdda.release());
            return true;
        }
        else {
            return false;
        }
    }
}
