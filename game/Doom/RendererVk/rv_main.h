#pragma once

#include "Doom/doomdef.h"
#include "Macros.h"
#include "Matrix4.h"

#if PSYDOOM_VULKAN_RENDERER

enum class VPipelineType_Main : uint8_t;

extern float                gViewXf;
extern float                gViewYf;
extern float                gViewZf;
extern float                gViewAnglef;
extern float                gViewCosf;
extern float                gViewSinf;
extern uint16_t             gClutX;
extern uint16_t             gClutY;
extern Matrix4f             gSpriteBillboardMatrix;
extern Matrix4f             gViewProjMatrix;
extern VPipelineType_Main   gOpaqueGeomPipeline;

void RV_RenderPlayerView() noexcept;

#endif  // #if PSYDOOM_VULKAN_RENDERER
