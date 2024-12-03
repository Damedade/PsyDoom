#pragma once

#if PSYDOOM_VULKAN_RENDERER

#include "Asserts.h"
#include "Framebuffer.h"
#include "IVRenderPath.h"
#include "RenderPass.h"
#include "VMsaaResolver.h"

#include <functional>
#include <vector>

class VRenderPath_Main;

namespace vgl {
    class Framebuffer;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// A render path used for drawing loading plaques
//------------------------------------------------------------------------------------------------------------------------------------------
class VRenderPath_LoadingPlaque : public IVRendererPath {
public:
    VRenderPath_LoadingPlaque() noexcept;
    ~VRenderPath_LoadingPlaque() noexcept;

    void init(
        vgl::LogicalDevice& device,
        vgl::Swapchain& swapchain,
        const VkFormat presentSurfaceFormat
    ) noexcept;

    void destroy() noexcept;
    
    virtual bool ensureValidFramebuffers(
        const uint32_t fbWidth,
        const uint32_t fbHeight,
        const bool bGpuIsIdle
    ) noexcept override;
    
    virtual void beginFrame(vgl::Swapchain& swapchain, vgl::CmdBufferRecorder& cmdRec) noexcept override;
    virtual void endFrame(vgl::Swapchain& swapchain, vgl::CmdBufferRecorder& cmdRec) noexcept override;

    inline bool isValid() const noexcept { return mbIsValid; }
    inline const vgl::RenderPass& getRenderPass() const { return mRenderPass; }
    
    inline void setPreRenderPassAction(std::function<void()>&& action) noexcept {
        mPreRenderPassAction = std::move(action);
    }

private:
    bool initRenderPass() noexcept;
    bool doFramebuffersNeedRecreate() noexcept;

    bool                            mbIsValid;                  // True if the render path has been initialized
    vgl::LogicalDevice*             mpDevice;                   // The vulkan device used
    vgl::Swapchain*                 mpSwapchain;                // The swapchain used
    VkFormat                        mPresentSurfaceFormat;      // The format for the swapchain image we present to (the output destination for this render path)
    vgl::RenderPass                 mRenderPass;                // The Vulkan renderpass for this render path
    std::vector<vgl::Framebuffer>   mFramebuffers;              // Framebuffers for each swapchain image
    std::function<void()>           mPreRenderPassAction;       // An action to be performed before the loading plaque render pass is started (note: executed only once!)
};

#endif  // #if PSYDOOM_VULKAN_RENDERER
