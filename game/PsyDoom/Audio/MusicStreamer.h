#include "Doom/cdmaptbl.h"
#include "Spu.h"
#include "Macros.h"

#include <memory>
#include <string>
#include <vector>

//------------------------------------------------------------------------------------------------------------------------------------------
// Abstract representation of a music source used by the music streamer.
// The stream is closed by destroying the stream.
//------------------------------------------------------------------------------------------------------------------------------------------
class IMusicSource {
public:
    // Result of trying to read a sample
    enum class ReadSampleResult {
        OK,             // The sample requested was read successfully
        END_OF_STEAM,   // Couldn't read the requested sample because the end of the current track was reached
        ERROR           // An error occurred when trying to read the requested sample
    };

    virtual ~IMusicSource() = default;
    
    // Attempt to read the next sample from the stream.
    // Also return a status code indicating what errors occurred, if any.
    virtual std::tuple<Spu::StereoSample, ReadSampleResult> readNextSample() noexcept = 0;
    
    // Tells how many stereo samples were read from the stream
    virtual size_t getCurrentStereoSampleIndex() const noexcept = 0;
    
    // Rewind the stream back to the start - used for looping.
    // Returns 'true' if successful.
    virtual bool rewind() noexcept = 0;
};

//------------------------------------------------------------------------------------------------------------------------------------------
// A class that allows streaming music from a number of sources.
//
// These include:
//  (1) CD Digital Audio (CDDA) from the original game disc.
//  (2) Ogg-vorbis audio from a 'MUSICXXX.OGG' file in the current mod directory.
//  (3) Raw audio from a 'MUSICXXX.RAW' file in the current mod directory.
//------------------------------------------------------------------------------------------------------------------------------------------
class MusicStreamer {
public:
    MusicStreamer() noexcept;
    ~MusicStreamer() noexcept;

    void init() noexcept;
    void shutdown() noexcept;

    bool playTrack(const int32_t trackNum, const int32_t loopTrackNum) noexcept;
    uint16_t getCurrentTrack() const noexcept;
    bool isTrackPlaying() const noexcept;
    bool isTrackPlayingAndUnpaused() const noexcept;
    void stop() noexcept;

    bool pause() noexcept;
    bool isPaused() const noexcept;
    bool resume() noexcept;

    Spu::StereoSample readNextSample() noexcept;
    size_t getCurrentStereoSampleIndex() noexcept;

private:
    bool playCurrentTrack() noexcept;

    uint16_t    mCurTrack;      // The current track being played by the music streamer (0 = none, not playing)
    uint16_t    mLoopTrack;     // Which track to play to loop when the current one ends (0 = none, don't loop)
    bool        mbPaused;       // If 'true' then playback is currently paused

    // The source of the music itself
    std::unique_ptr<IMusicSource> mpMusicSource;

    // Which music tracks are available as Ogg Vorbis files and the paths to them
    struct OggVorbisTrack {
        int32_t         trackNum;
        std::string     filePath;
    };

    std::vector<OggVorbisTrack> mOggVorbisTracks;
};
