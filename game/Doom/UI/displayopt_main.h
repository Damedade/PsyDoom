#pragma once

#if PSYDOOM_MODS

#include "Doom/doomdef.h"

void DisplayOpt_Init() noexcept;
void DisplayOpt_Shutdown(const gameaction_t exitAction) noexcept;
gameaction_t DisplayOpt_Update() noexcept;
void DisplayOpt_Draw() noexcept;

#endif  // #if PSYDOOM_MODS
