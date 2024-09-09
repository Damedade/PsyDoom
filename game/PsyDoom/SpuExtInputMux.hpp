#pragma once

#include "Spu.h"

BEGIN_NAMESPACE(SpuExtInputMux)

void init() noexcept;
void shutdown() noexcept;
void addInput(const Spu::ExtInputCallback callback) noexcept;
void removeInput(const Spu::ExtInputCallback callback) noexcept;

END_NAMESPACE(SpuExtInputMux)
