#pragma once

#if PSYDOOM_VULKAN_RENDERER

#include "Defines.h"
#include "DescriptorPool.h"
#include "Framebuffer.h"
#include "IVRenderPath.h"
#include "RenderPass.h"
#include "Texture.h"
#include "VVertexBufferSet.h"

#include <vector>

//------------------------------------------------------------------------------------------------------------------------------------------
// A Vulkan renderer path which takes the output from the emulated PSX GPU and blits it to the current swapchain image for display.
// Allows the classic PlayStation renderer to be passed through the Vulkan renderer and output that way.
// This architecture is key to allowing rapid toggling between the old PSX renderer and the new Vulkan renderer.
//------------------------------------------------------------------------------------------------------------------------------------------
class VRenderPath_Psx : public IVRendererPath {
public:
    VRenderPath_Psx() noexcept;
    ~VRenderPath_Psx() noexcept;

    void init(
        vgl::LogicalDevice& device,
        vgl::Swapchain& swapchain,
        const VkFormat presentSurfaceFormat,
        const VkFormat psxFramebufferFormat
    ) noexcept;

    void destroy() noexcept;

    virtual bool ensureValidFramebuffers(const uint32_t fbWidth, const uint32_t fbHeight) noexcept override;
    virtual void beginFrame(vgl::Swapchain& swapchain, vgl::CmdBufferRecorder& cmdRec) noexcept override;
    virtual void endFrame(vgl::Swapchain& swapchain, vgl::CmdBufferRecorder& cmdRec) noexcept override;

    inline bool isValid() const noexcept { return mbIsValid; }
    inline const vgl::RenderPass& getGammaAdjustRenderPass() const { return mGammaAdjustRenderPass; }

private:
    bool initGammaAdjustRenderPass(const VkFormat presentSurfaceFormat) noexcept;
    bool initGammaAdjustDescriptorPoolAndSets() noexcept;
    bool doGammaAdjustFramebuffersNeedRecreate() noexcept;

    void copyPsxFramebufferToFbTexture_A1R5G5B5(const uint16_t* const pSrcPixels, uint16_t* pDstPixels) noexcept;
    void copyPsxFramebufferToFbTexture_B8G8R8A8(const uint16_t* const pSrcPixels, uint32_t* pDstPixels) noexcept;

    bool                    mbIsValid;                  // True if the render path has been initialized
    vgl::LogicalDevice*     mpDevice;                   // The Vulkan device used
    vgl::Swapchain*         mpSwapchain;                // The swapchain used
    vgl::RenderPass         mGammaAdjustRenderPass;     // A Vulkan renderpass used for adjusting gamma when using this render path

    // PSX renderer framebuffer textures, as copied from the PSX GPU.
    // When not doing any gamma adjust these are blitted directly onto the current swapchain image.
    // One for each ringbuffer slot, so we can update while a previous frame's image is still blitting to the screen.
    vgl::Texture mPsxFramebufferTextures[vgl::Defines::RINGBUFFER_SIZE];
    
    // Vulkan framebuffers for when gamma adjust is being used.
    // Each framebuffer uses a single swapchain image.
    std::vector<vgl::Framebuffer> mGammaAdjustFramebuffers;
    
    // Used for gamma adjustment: a descriptor pool and the descriptor sets allocated from it.
    // The descriptor sets just contain a binding for an input multi-sampled attachment to resolve - one for each ringbuffer index so we don't have to update constantly.
    vgl::DescriptorPool mGammaAdjustDescriptorPool;
    vgl::DescriptorSet* mpGammaAdjustDescriptorSets[vgl::Defines::RINGBUFFER_SIZE];
    
    // Vertex buffers used for doing gamma adjust
    VVertexBufferSet mGammaAdjustVerts;
};

#endif  // #if PSYDOOM_VULKAN_RENDERER
