#pragma once

#if PSYDOOM_MODS

struct anim_t;

void P_InitLiquids() noexcept;
void P_AnimLiquid_Water(const anim_t& anim) noexcept;
void P_AnimLiquid_Slime(const anim_t& anim) noexcept;
void P_AnimLiquid_Blood(const anim_t& anim) noexcept;
void P_AnimLiquid_Lava(const anim_t& anim) noexcept;

#endif  // #if PSYDOOM_MODS
