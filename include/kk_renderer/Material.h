#pragma once

#include <memory>
#include "RenderingContext.h"
#include "Texture.h"
#include "Buffer.h"
#include "Shader.h"

namespace kk {
    namespace renderer{
        class Material {
        public:
            Material();

            void destroy(RenderingContext& ctx);

            void setVertexShader(const std::shared_ptr<Shader>& vert);
            void setFragmentShader(const std::shared_ptr<Shader>& frag);
            void setBuffer(
                uint32_t binding,
                const std::shared_ptr<Buffer>& buffer,
                VkDescriptorType type,
                VkShaderStageFlags stage
            );
            void setTexture(
                uint32_t binding,
                const std::shared_ptr<Texture>& texture,
                VkDescriptorType type,
                VkShaderStageFlags stage
            );
            std::shared_ptr<Buffer> getBuffer(uint32_t binding);

            void compile(RenderingContext& ctx, VkRenderPass render_pass);

            inline bool isCompiled() const { return is_compiled_; }
            inline VkPipeline getPipeline() const { return pipeline_; }
            inline VkPipelineLayout getPipelineLayout() const { return pipeline_layout_; }
            inline VkDescriptorSetLayout getDescriptorSetLayout() const { return desc_layout_; }
            inline const std::array<VkDescriptorSet, kMaxConcurrentFrames>& getDescriptorSets() const {
                return desc_sets_;
            }

        private:
            struct ResourceBindingInfo {
                VkDescriptorSetLayoutBinding layout;
                std::shared_ptr<void> bind_info;
                std::shared_ptr<void> resource;
            };

            void setDefault();
            void buildDescLayout(RenderingContext& ctx);
            void buildDescSets(RenderingContext& ctx, VkDescriptorSetLayout layout);
            void buildPipelineLayout(RenderingContext& ctx, VkDescriptorSetLayout desc_layout);
            void buildPipeline(RenderingContext& ctx, VkPipelineLayout layout, VkRenderPass render_pass);

            void addBindingInfo(
                const VkDescriptorSetLayoutBinding& layout,
                const std::shared_ptr<void>& bind_info,
                const std::shared_ptr<void>& resource
            );

            bool is_compiled_;

            std::unordered_map<VkShaderStageFlagBits, std::shared_ptr<Shader>> shaders_;
            VkPipelineInputAssemblyStateCreateInfo input_asm_;
            VkPipelineViewportStateCreateInfo viewport_;
            VkPipelineRasterizationStateCreateInfo rasterizer_;
            VkPipelineMultisampleStateCreateInfo multisampling_;
            VkPipelineColorBlendStateCreateInfo color_blending_;
            std::vector<VkPipelineColorBlendAttachmentState> blend_attachments_;
            std::vector<VkDynamicState> dynamic_states_;

            VkPipelineLayout pipeline_layout_;
            VkPipeline pipeline_;

            // NOTE: LayoutBinding |-> (bind info, bind resource)
            std::vector<ResourceBindingInfo> bindings_;
            VkDescriptorSetLayout desc_layout_;
            std::array<VkDescriptorSet, kMaxConcurrentFrames> desc_sets_;
        };
    }
}
