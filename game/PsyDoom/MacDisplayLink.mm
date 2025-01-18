//------------------------------------------------------------------------------------------------------------------------------------------
// A module that allows the game to synchronize to the refresh rate of the display on macOS.
// The synchronization is performed using the 'DisplayLink' mechanism.
//------------------------------------------------------------------------------------------------------------------------------------------
#include "MacDisplayLink.h"

#if TARGET_OS_MAC

#include "Asserts.h"
#include "Utils.h"
#include "Video.h"

#include <SDL.h>
#include <SDL_syswm.h>

#include <condition_variable>
#include <memory>
#include <mutex>

#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 1400
#define USE_CA_DISPLAY_LINK 1
#endif

#if __MAC_OS_X_VERSION_MIN_REQUIRED < 1400
#define USE_CV_DISPLAY_LINK 1
#endif

//------------------------------------------------------------------------------------------------------------------------------------------
// Helper: gets the NSWindow for the game
//------------------------------------------------------------------------------------------------------------------------------------------
NSWindow* GetGameNSWindow() noexcept {
    if (Video::gpSdlWindow) {
        SDL_SysWMinfo wmInfo = {};
        wmInfo.version = { SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL };
        
        if (SDL_GetWindowWMInfo(Video::gpSdlWindow, &wmInfo))
            return wmInfo.info.cocoa.window;
    }
    
    return nullptr;
}

#if USE_CA_DISPLAY_LINK
//------------------------------------------------------------------------------------------------------------------------------------------
// DisplayLink implementation via 'CADisplayLink' (macOS 14+)
//------------------------------------------------------------------------------------------------------------------------------------------

// Forward declare the DisplayLink listener
@interface DisplayLinkListener : NSObject
-(void) displayLinkCallback;
@end

// Core object managing CADisplayLink.
// Receives both notifications to draw, and queries about whether to draw from the main thread (no synchronization needed).
class CADisplayLinkMgr
{
public:
    CADisplayLinkMgr() noexcept
        : mbIsValid(false)
        , mbIsReadyToDraw(false)
        , mpCADisplayLink(nullptr)
        , mpListener(nullptr)
    {
    }
    
    ~CADisplayLinkMgr() noexcept {
        destroy();
    }
    
    // Initializes the DisplayLink and returns 'true' if successful
    bool init() noexcept {
        destroy();
        
        if (@available(macOS 14, *)) {
            mpListener = [[DisplayLinkListener alloc] init];
            NSWindow* const pNSWindow = GetGameNSWindow();
            
            if (mpListener && pNSWindow) {
                mpCADisplayLink = [pNSWindow displayLinkWithTarget:mpListener selector:@selector(displayLinkCallback)];
                
                if (mpCADisplayLink) {
                    [mpCADisplayLink retain];
                    mbIsValid = true;
                    // N.B: the DisplayLink callback will happen on the main thread!
                    [mpCADisplayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];
                }
            }
        }
        
        if (!mbIsValid) {
            destroy(); // Don't half initialize, cleanup on failure!
        }
        
        return mbIsValid;
    }
    
    // Shuts down the DisplayLink
    void destroy() noexcept {
        if (mpCADisplayLink) {
            [mpCADisplayLink removeFromRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];
            mbIsValid = false;
            [mpCADisplayLink release];
            mpCADisplayLink = nil;
        }
        
        if (mpListener) {
            [mpListener release];
            mpListener = nil;
        }
        
        mbIsReadyToDraw = false;
        mbIsValid = false;
    }

    // Query if the DisplayLink was initialized successfully
    bool isValid() const noexcept {
        return mbIsValid;
    }
    
    // Notify this object that the game is ready to be redrawn
    void notifyReadyToRedraw() noexcept {
        ASSERT(mbIsValid);
        mbIsReadyToDraw = true;
    }
    
    // Wait for a signal to redraw the game
    void synchronize() noexcept {
        ASSERT(mbIsValid);
    
        // Continue pumping the Cocoa event loop (among other stuff) until we receive a notification on
        // the main thread's message queue that the game is ready to be redrawn.
        while (!mbIsReadyToDraw) {
            Utils::doPlatformUpdates();
        }
        
        // Clear for the next frame
        mbIsReadyToDraw = false;
    }

private:
    bool                    mbIsValid;          // True if DisplayLink was initialized successfully
    bool                    mbIsReadyToDraw;    // Set to true once the game should redraw
    CADisplayLink*          mpCADisplayLink;    // DisplayLink timer object
    DisplayLinkListener*    mpListener;         // Listener for DisplayLink callbacks
};

// The global instance of CADisplayLinkMgr
static std::unique_ptr<CADisplayLinkMgr> gpCADisplayLinkMgr;

// Implementation of DisplayLinkListener
@implementation DisplayLinkListener

- (void) displayLinkCallback {
    if (gpCADisplayLinkMgr) {
        gpCADisplayLinkMgr->notifyReadyToRedraw();
    }
}

@end

#endif  // #if USE_CA_DISPLAY_LINK

#if USE_CV_DISPLAY_LINK
//------------------------------------------------------------------------------------------------------------------------------------------
// DisplayLink implementation via 'CVDisplayLink' (macOS < 14)
//------------------------------------------------------------------------------------------------------------------------------------------

// Core object managing CVDisplayLink.
// This receives notification from the DisplayLink thread, which is not the main thread - hence requires synchronization.
class CVDisplayLinkMgr
{
public:
    CVDisplayLinkMgr() noexcept
        : mbIsValid(false)
        , mbIsReadyToDraw(false)
        , mSyncMutex()
        , mSyncCond()
        , mpCVDisplayLink(nullptr)
    {
    }
    
    ~CVDisplayLinkMgr() noexcept {
        destroy();
    }
    
    // Initializes the DisplayLink and returns 'true' if successful
    bool init() noexcept {
        destroy();
        
		if (CVDisplayLinkCreateWithActiveCGDisplays(&mpCVDisplayLink) == kCVReturnSuccess) {
            if (CVDisplayLinkSetOutputCallback(mpCVDisplayLink, &DisplayLinkCallback, this) == kCVReturnSuccess) {
                if (CVDisplayLinkStart(mpCVDisplayLink) == kCVReturnSuccess) {
                    mbIsValid = true;
                } else {
                    CVDisplayLinkRelease(mpCVDisplayLink);
                    mpCVDisplayLink = nullptr;
                }
            }
            else {
                CVDisplayLinkRelease(mpCVDisplayLink);
                mpCVDisplayLink = nullptr;
            }
        }
        
        if (!mbIsValid) {
            destroy(); // Don't half initialize, cleanup on failure!
        }
        
        return mbIsValid;
    }
    
    // Shuts down the DisplayLink
    void destroy() noexcept {
        if (mpCVDisplayLink) {
            CVDisplayLinkStop(mpCVDisplayLink);
            CVDisplayLinkRelease(mpCVDisplayLink);
            mpCVDisplayLink = nullptr;
        }
        
        mbIsReadyToDraw = false;
        mbIsValid = false;
    }

    // Query if the DisplayLink was initialized successfully
    bool isValid() const noexcept {
        return mbIsValid;
    }
    
    // Notify this object that the game is ready to be redrawn
    void notifyReadyToRedraw() noexcept {
        ASSERT(mbIsValid);
        
        // Set the ready to draw flag
        {
            std::lock_guard<std::mutex> lockMutex(mSyncMutex);
            mbIsReadyToDraw = true;
        }
        
        // Notify the main thread if it is listening
        mSyncCond.notify_one();
    }
    
    // Wait for a signal to redraw the game
    void synchronize() noexcept {
        ASSERT(mbIsValid);
        std::unique_lock<std::mutex> lockMutex(mSyncMutex);
        
        // Already ready to draw?
        if (mbIsReadyToDraw) {
            mbIsReadyToDraw = false; // Clear for the next frame
            return;
        }

        // Not ready yet: wait until the game is ready to redraw, then clear the flag for the next frame
        mSyncCond.wait(lockMutex, [this](){ return mbIsReadyToDraw; });

        // Clear for the next frame
        mbIsReadyToDraw = false;
    }

private:
    // Receives notifications from the DisplayLink thread that the game is ready to display
    static CVReturn DisplayLinkCallback(
		const CVDisplayLinkRef CV_NONNULL pDisplayLink,
		const CVTimeStamp* const CV_NONNULL pInNow,
		const CVTimeStamp* const CV_NONNULL pInOutputTime,
		const CVOptionFlags flagsIn,
		CVOptionFlags* const CV_NONNULL pFlagsOut,
		void* const CV_NULLABLE pDisplayLinkContext
    ) noexcept
    {
        CVDisplayLinkMgr* const pMgr = static_cast<CVDisplayLinkMgr*>(pDisplayLinkContext);
        pMgr->notifyReadyToRedraw();
        return kCVReturnSuccess;
    }

    bool                        mbIsValid;          // True if DisplayLink was initialized successfully
    bool                        mbIsReadyToDraw;    // Set to true once the game should redraw
    std::mutex                  mSyncMutex;         // Mutex used to synchronize access to 'mbIsReadyToDraw' and the condition variable
    std::condition_variable     mSyncCond;          // Condition variable used to wait on and notify when 'bReadyToDraw' is true
    CVDisplayLinkRef            mpCVDisplayLink;    // DisplayLink reference
};

// The global instance of CVDisplayLinkMgr
static std::unique_ptr<CVDisplayLinkMgr> gpCVDisplayLinkMgr;

#endif  // #if USE_CV_DISPLAY_LINK


BEGIN_NAMESPACE(MacDisplayLink)

//------------------------------------------------------------------------------------------------------------------------------------------
// Initializes DisplayLink synchronization
//------------------------------------------------------------------------------------------------------------------------------------------
bool init() noexcept {
    bool bHaveDisplayLink = false;

    #if USE_CA_DISPLAY_LINK
        ASSERT(gpCADisplayLinkMgr == nullptr); // Shouldn't already be initialized!
    
        if (!bHaveDisplayLink) {
            gpCADisplayLinkMgr = std::make_unique<CADisplayLinkMgr>();
            
            if (gpCADisplayLinkMgr->init()) {
                bHaveDisplayLink = true;
            } else {
                gpCADisplayLinkMgr.reset();
            }
        }
    #endif
    
    #if USE_CV_DISPLAY_LINK
        ASSERT(gpCVDisplayLinkMgr == nullptr); // Shouldn't already be initialized!
    
        if (!bHaveDisplayLink) {
            gpCVDisplayLinkMgr = std::make_unique<CVDisplayLinkMgr>();
            
            if (gpCVDisplayLinkMgr->init()) {
                bHaveDisplayLink = true;
            } else {
                gpCVDisplayLinkMgr.reset();
            }
        }
    #endif    
    
    return bHaveDisplayLink;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Shuts down DisplayLink synchronization
//------------------------------------------------------------------------------------------------------------------------------------------
void shutdown() noexcept {
    #if USE_CA_DISPLAY_LINK
        gpCADisplayLinkMgr.reset();
    #endif
    #if USE_CV_DISPLAY_LINK
        gpCVDisplayLinkMgr.reset();
    #endif
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Waits for a signal to redraw the game from DisplayLink.
// If DisplayLink was not initialized successfully, returns immediately.
//------------------------------------------------------------------------------------------------------------------------------------------
void synchronize() noexcept {
    #if USE_CA_DISPLAY_LINK
        if (gpCADisplayLinkMgr) {
            gpCADisplayLinkMgr->synchronize();
            return;
        }
    #endif
    
    #if USE_CV_DISPLAY_LINK
        if (gpCVDisplayLinkMgr) {
            gpCVDisplayLinkMgr->synchronize();
            return;
        }
    #endif
}

END_NAMESPACE(MacDisplayLink)

#endif  // #if TARGET_OS_MAC
