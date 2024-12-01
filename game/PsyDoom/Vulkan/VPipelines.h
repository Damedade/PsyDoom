#pragma once

#if PSYDOOM_VULKAN_RENDERER

#include "Asserts.h"
#include "Macros.h"
#include "Pipeline.h"
#include "VTypes.h"

namespace vgl {
    class DescriptorSetLayout;
    class LogicalDevice;
    class Pipeline;
    class PipelineLayout;
    class RenderPass;
    class Sampler;
}

class VRenderPath_Crossfade;
class VRenderPath_Main;
class VRenderPath_Psx;

template <class PipelineTypeEnum>
struct VPipelineSet {
    vgl::Pipeline pipelines[(uint32_t) PipelineTypeEnum::NUM_TYPES];
    
    inline vgl::Pipeline& get(const PipelineTypeEnum pipelineId) noexcept {
        ASSERT((uint32_t) pipelineId < (uint32_t) PipelineTypeEnum::NUM_TYPES);
        return pipelines[(uint32_t) pipelineId];
    }
    
    inline const vgl::Pipeline& get(const PipelineTypeEnum pipelineId) const noexcept {
        ASSERT((uint32_t) pipelineId < (uint32_t) PipelineTypeEnum::NUM_TYPES);
        return pipelines[(uint32_t) pipelineId];
    }
};

BEGIN_NAMESPACE(VPipelines)

extern vgl::Sampler                 gSampler_draw;
extern vgl::Sampler                 gSampler_normClampNearest;
extern vgl::DescriptorSetLayout     gDescSetLayout_draw;
extern vgl::DescriptorSetLayout     gDescSetLayout_msaaResolve;
extern vgl::DescriptorSetLayout     gDescSetLayout_crossfade;
extern vgl::DescriptorSetLayout     gDescSetLayout_loadingPlaque;
extern vgl::DescriptorSetLayout     gDescSetLayout_gammaAdjustBlit;
extern vgl::DescriptorSetLayout     gDescSetLayout_gammaAdjustPostProcess;
extern vgl::PipelineLayout          gPipelineLayout_draw;
extern vgl::PipelineLayout          gPipelineLayout_msaaResolve;
extern vgl::PipelineLayout          gPipelineLayout_crossfade;
extern vgl::PipelineLayout          gPipelineLayout_loadingPlaque;
extern vgl::PipelineLayout          gPipelineLayout_gammaAdjustBlit;
extern vgl::PipelineLayout          gPipelineLayout_gammaAdjustPostProcess;

extern VPipelineSet<VPipelineType_Main>       gPipelines_Main_NoGammaAdjust;
extern VPipelineSet<VPipelineType_Main>       gPipelines_Main_GammaAdjust;
extern VPipelineSet<VPipelineType_Crossfade>  gPipelines_Crossfade;
extern VPipelineSet<VPipelineType_PSX>        gPipelines_PSX;

void initPipelineComponents(vgl::LogicalDevice& device, const uint32_t numSamples) noexcept;

void initPipelines(
    VRenderPath_Main& mainRPath,
    VRenderPath_Psx& psxRPath,
    VRenderPath_Crossfade& crossfadeRPath,
    const uint32_t numSamples
) noexcept;

void shutdown() noexcept;

const VPipelineSet<VPipelineType_Main>& getMainPipelineSet() noexcept;

END_NAMESPACE(VPipelines)

#endif  // #if PSYDOOM_VULKAN_RENDERER
