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
#include "Image.h"
#include "DirectionalLight.h"

namespace kk {
    namespace renderer {
        class Renderer {
        public:
            static Renderer create(RenderingContext& ctx);
            void destroy(RenderingContext& ctx);

            bool beginFrame(RenderingContext& ctx);
            void endFrame(RenderingContext& ctx);
            void render(RenderingContext& ctx, Renderable& renderable, const Transform& transform);
            void render(RenderingContext& ctx, std::vector<Renderable>& scene, const DirectionalLight& light, const Camera& camera);

            void compileMaterial(RenderingContext& ctx, const std::shared_ptr<Material>& material);

            // FIXME: For editor initialization, framebuffer should be public.
            inline VkFramebuffer getFramebuf() const {
                return framebuffers_[img_idx_];
            }

            // FIXME: For editor rendering, cmd buf should be public.
            inline VkCommandBuffer getCmdBuf() const {
                return cmd_bufs_[current_frame_];
            }

        private:
            struct GlobalUniform {
                alignas(16) Mat4 view;
                alignas(16) Mat4 proj;
                alignas(16) Mat4 light_view;
                alignas(16) Mat4 light_proj;
                alignas(16) Vec3 light_pos;
                alignas(16) Vec3 light_dir;
                alignas(16) Vec3 light_color;
                float light_intensity;
            };

            struct ObjectUniform {
                alignas(16) Mat4 model_to_world;
                alignas(16) Mat4 world_to_model;
            };

            void renderShadowMap(RenderingContext& ctx, std::vector<Renderable>& scene, const DirectionalLight& light);
            void renderColor(RenderingContext& ctx, std::vector<Renderable>& scene, const DirectionalLight& light, const Camera& camera, Swapchain& swapchain);
            void createDescriptors(RenderingContext& ctx);
            void createFramebuffers(RenderingContext& ctx, const Swapchain& swapchain);
            void createShadowResources(RenderingContext& ctx);
            void prepareRendering(RenderingContext& ctx, Renderable& renderable);

            VkRenderPass render_pass_;
            Image depth_;

            Texture shadow_map_;
            VkRenderPass shadow_pass_;
            VkPipelineLayout shadow_layout_;
            VkPipeline shadow_pipeline_;
            VkFramebuffer shadow_framebuffer_;
            std::array<std::pair<Buffer, VkDescriptorSet>, kMaxConcurrentFrames> shadow_global_uniforms_;

            std::vector<VkFramebuffer> framebuffers_;
            std::array<VkCommandBuffer, kMaxConcurrentFrames> cmd_bufs_;
            size_t current_frame_;
            uint32_t img_idx_;

            VkDescriptorSetLayout global_uniform_layout_;
            std::array<std::pair<Buffer, VkDescriptorSet>, kMaxConcurrentFrames> global_uniforms_;
            VkDescriptorSetLayout object_uniform_layout_;
            std::unordered_map<size_t, std::array<std::pair<Buffer, VkDescriptorSet>, kMaxConcurrentFrames>> object_uniforms_;

            bool is_camera_binded_;
        };
    }
}
