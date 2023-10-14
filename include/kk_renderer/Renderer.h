#pragma once

#include "RenderingContext.h"
#include "Swapchain.h"
#include "Geometry.h"
#include "Transform.h"
#include "ResourceDescriptor.h"

namespace kk {
    namespace renderer {
        class Renderer {
        public:
            static Renderer create(RenderingContext& ctx, Swapchain& swapchain);
            void destroy(RenderingContext& ctx);

            bool beginFrame(RenderingContext& ctx, Swapchain& swapchain);
            void endFrame(RenderingContext& ctx, Swapchain& swapchain);
            void recordCommands();
            void render(const Geometry& geometry);
            void render(const Geometry& geometry, const Transform& transform);

        private:
            VkRenderPass render_pass_;
            VkPipelineLayout pipeline_layout_;
            VkPipeline pipeline_;
            VkDescriptorSetLayout desc_layout_;
            std::array<VkDescriptorSet, kMaxConcurrentFrames> desc_sets_;
            Buffer uniform_; // CONCERN: This shouldn't be placed here ?
            void* mapped_uniform_;

            std::array<VkFramebuffer, kMaxConcurrentFrames> framebuffers_; // CONCERN
            std::array<VkCommandBuffer, kMaxConcurrentFrames> cmd_bufs_;
            size_t current_frame_;
            uint32_t img_idx_;
        };
    }
}
