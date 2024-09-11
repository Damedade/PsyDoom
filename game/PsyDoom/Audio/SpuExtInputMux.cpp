//------------------------------------------------------------------------------------------------------------------------------------------
// Spu External Input Multiplexer.
// This module hooks itself up to the global PSX SPU.
// It provides a flexible way to add and remove audio inputs which that get routed through the Spu.
// For example it can be used to pipe CD music or an external sound playback system through the Spu.
//------------------------------------------------------------------------------------------------------------------------------------------
#include "SpuExtInputMux.h"

#include "Asserts.h"
#include "PsyDoom/PsxVm.h"

#include <mutex>

BEGIN_NAMESPACE(SpuExtInputMux)

//------------------------------------------------------------------------------------------------------------------------------------------
// Globals
//------------------------------------------------------------------------------------------------------------------------------------------
static constexpr uint32_t MAX_MUX_INPUTS = 4;
static Spu::ExtInputCallback gMuxInputCallbacks[MAX_MUX_INPUTS];

//------------------------------------------------------------------------------------------------------------------------------------------
// The lock for the multiplexer and a helper to lock/unlock via RAII
//------------------------------------------------------------------------------------------------------------------------------------------
static std::recursive_mutex gSpuInputMuxMutex;

struct LockSpuInputMux {
    LockSpuInputMux() noexcept { gSpuInputMuxMutex.lock(); }
    ~LockSpuInputMux() noexcept { gSpuInputMuxMutex.unlock(); }
};

//------------------------------------------------------------------------------------------------------------------------------------------
// Helper: returns the index of the specified input callback or UINT32_MAX if not found.
// Assumes the lock for the multiplexer is held.
//------------------------------------------------------------------------------------------------------------------------------------------
static uint32_t findSpuCallback(const Spu::ExtInputCallback callback) noexcept {
    for (uint32_t i = 0; i < MAX_MUX_INPUTS; ++i) {
        if (gMuxInputCallbacks[i] == callback)
            return i;
    }

    return UINT32_MAX;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// This is the callback invoked by the SPU to get external audio input
//------------------------------------------------------------------------------------------------------------------------------------------
static Spu::SpuCallbackOutput spuCallback() noexcept {
    Spu::SpuCallbackOutput combinedOutput = {};

    {
        LockSpuInputMux lockMux;

        for (uint32_t i = 0; i < MAX_MUX_INPUTS; ++i) {
            const Spu::ExtInputCallback inputCallback = gMuxInputCallbacks[i];

            if (inputCallback) {
                const Spu::SpuCallbackOutput callbackOutput = inputCallback();
                combinedOutput.sample += callbackOutput.sample;
                combinedOutput.reverbSample += callbackOutput.reverbSample;
            }
        }
    }

    return combinedOutput;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Initialize and shutdown the multiplexer
//------------------------------------------------------------------------------------------------------------------------------------------
void init() noexcept {
    // Connect the Multiplexer to the SPU via the external input callback
    {
        PsxVm::LockSpu lockSpu;
        ASSERT_LOG(PsxVm::gSpu.pExtInputCallback == nullptr, "The Multiplexer is wiping out the current Spu external input callback!");
        PsxVm::gSpu.pExtInputCallback = spuCallback;
    }
}

void shutdown() noexcept {
    // Disconnect the Multiplexer from the SPU's external input callback
    {
        PsxVm::LockSpu lockSpu;
        ASSERT_LOG(PsxVm::gSpu.pExtInputCallback == spuCallback, "Current Spu callback is not owned by the Multiplexer!");
        PsxVm::gSpu.pExtInputCallback = nullptr;
    }

    // Clear all input callbacks
    for (Spu::ExtInputCallback& callback : gMuxInputCallbacks) {
        callback = nullptr;
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Add an input to the multiplexer
//------------------------------------------------------------------------------------------------------------------------------------------
void addInput(const Spu::ExtInputCallback callback) noexcept {
    LockSpuInputMux lockMux;

    // Already added?
    if (findSpuCallback(callback) < MAX_MUX_INPUTS)
        return;

    // Add it if there is room
    for (uint32_t i = 0; i < MAX_MUX_INPUTS; ++i) {
        if (gMuxInputCallbacks[i] == nullptr) {
            gMuxInputCallbacks[i] = callback;
            break;
        }
    }

    // Debug sanity check that we didn't run out of callbacks
    ASSERT(findSpuCallback(callback) < MAX_MUX_INPUTS);
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Remove an input from the multiplexer
//------------------------------------------------------------------------------------------------------------------------------------------
void removeInput(const Spu::ExtInputCallback callback) noexcept {
    LockSpuInputMux lockMux;

    for (uint32_t i = 0; i < MAX_MUX_INPUTS; ++i) {
        if (gMuxInputCallbacks[i] == callback) {
            gMuxInputCallbacks[i] = nullptr;
            break;
        }
    }
}

END_NAMESPACE(SpuExtInputMux)
