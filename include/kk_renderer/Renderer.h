#pragma once

#include <vulkan/vulkan.h>
#include "RenderingContext.h"
#include "Swapchain.h"
#include "Geometry.h"
#include "Transform.h"
#include "Renderable.h"
#include "Camera.h"

namespace kk {
    namespace renderer {
        class Renderer {
        public:
            static Renderer create(RenderingContext& ctx, Swapchain& swapchain);
            void destroy(RenderingContext& ctx);

            bool beginFrame(RenderingContext& ctx, Swapchain& swapchain);
            void endFrame(RenderingContext& ctx, Swapchain& swapchain);
            void render(RenderingContext& ctx, Renderable& renderable, const Transform& transform, const Camera& camera);

        private:
            VkRenderPass render_pass_;

            std::array<VkFramebuffer, kMaxConcurrentFrames> framebuffers_; // CONCERN
            std::array<VkCommandBuffer, kMaxConcurrentFrames> cmd_bufs_;
            size_t current_frame_;
            uint32_t img_idx_;
        };
    }
}
