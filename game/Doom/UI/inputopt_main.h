#pragma once

#if PSYDOOM_MODS

#include "Doom/doomdef.h"

void InputOpt_Init() noexcept;
void InputOpt_Shutdown(const gameaction_t exitAction) noexcept;
gameaction_t InputOpt_Update() noexcept;
void InputOpt_Draw() noexcept;

#endif  // #if PSYDOOM_MODS
