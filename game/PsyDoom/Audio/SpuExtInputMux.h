#pragma once

#include "Spu.h"

BEGIN_NAMESPACE(SpuExtInputMux)

// RAII exclusive access lock for the input multiplexer.
// Needed so we can access the multiplexer safely on different threads.
struct LockSpuInputMux {
    LockSpuInputMux() noexcept;
    ~LockSpuInputMux() noexcept;
};

void init() noexcept;
void shutdown() noexcept;
void addInput(const Spu::ExtInputCallback callback) noexcept;
void removeInput(const Spu::ExtInputCallback callback) noexcept;

END_NAMESPACE(SpuExtInputMux)
