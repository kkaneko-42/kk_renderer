#pragma once

#include "RenderingContext.h"
#include "Swapchain.h"

namespace kk {
    namespace renderer {
        class Renderer {
        public:
            static Renderer create(RenderingContext& ctx, VkFormat swapchain_format /* TODO: remove swapchain_format */);
            void destroy(RenderingContext& ctx);

            bool beginFrame(RenderingContext& ctx, Swapchain& swapchain);
            void endFrame(RenderingContext& ctx, Swapchain& swapchain);

        private:
            VkRenderPass render_pass_;
            VkPipelineLayout pipeline_layout_;
            VkPipeline pipeline_;
            std::array<VkCommandBuffer, kMaxConcurrentFrames> cmd_bufs_;
            size_t current_frame_;
            uint32_t img_idx_;
        };
    }
}
