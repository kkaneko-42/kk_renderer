#pragma once

#include <vulkan/vulkan.h>
#include <unordered_map>
#include "RenderingContext.h"
#include "Swapchain.h"
#include "Geometry.h"
#include "Transform.h"
#include "Renderable.h"
#include "Camera.h"
#include "ResourceDescriptor.h"

namespace kk {
    namespace renderer {
        class Renderer {
        public:
            static Renderer create(RenderingContext& ctx, Swapchain& swapchain);
            void destroy(RenderingContext& ctx);

            bool beginFrame(RenderingContext& ctx, Swapchain& swapchain);
            void endFrame(RenderingContext& ctx, Swapchain& swapchain);
            void render(RenderingContext& ctx, Renderable& renderable, const Transform& transform, const Camera& camera);

            void compileMaterial(RenderingContext& ctx, const std::shared_ptr<Material>& material);

        private:
            void prepareRendering(RenderingContext& ctx, Renderable& renderable);

            VkRenderPass render_pass_;

            std::vector<VkFramebuffer> framebuffers_;
            std::array<VkCommandBuffer, kMaxConcurrentFrames> cmd_bufs_;
            size_t current_frame_;
            uint32_t img_idx_;

            std::unordered_map<size_t, std::array<Buffer, kMaxConcurrentFrames>> uniforms_;
        };
    }
}
