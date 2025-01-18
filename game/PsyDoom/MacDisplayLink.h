#pragma once

#if __APPLE__

#include "Macros.h"

#include <TargetConditionals.h>

#if TARGET_OS_MAC

BEGIN_NAMESPACE(MacDisplayLink)

bool init() noexcept;
void shutdown() noexcept;
void synchronize() noexcept;

END_NAMESPACE(MacDisplayLink)

#endif  // #if TARGET_OS_MAC
#endif  // #if __APPLE__
