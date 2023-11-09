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

            inline void setVertexShader(const std::shared_ptr<Shader>& vert) {
                vert_ = vert;
            }

            inline void setFragmentShader(const std::shared_ptr<Shader>& frag) {
                frag_ = frag;
            }

            inline void setTexture(const std::string& name, const std::shared_ptr<Texture>& texture) {
                resource_data_[name] = {std::static_pointer_cast<void>(texture), true};
            }

            // CONCERN: Vulkan abstraction
            inline void setFrontFace(VkFrontFace front_face) {
                rasterizer_.frontFace = front_face;
            }

            inline void setDepthCompareOp(VkCompareOp op) {
                depth_stencil_.depthCompareOp = op;
            }

            void compile(
                RenderingContext& ctx,
                VkRenderPass render_pass,
                VkDescriptorSetLayout per_view_layout,
                VkDescriptorSetLayout per_object_layout
            );

            inline bool isCompiled() const { return is_compiled_; }
            inline VkPipeline getPipeline() const { return pipeline_; }
            inline VkPipelineLayout getPipelineLayout() const { return pipeline_layout_; }
            inline VkDescriptorSetLayout getDescriptorSetLayout() const { return desc_layout_; }
            inline VkDescriptorSet getDescriptorSet() const { return desc_set_; }

        private:
            void setDefault();
            void buildDescLayout(RenderingContext& ctx);
            void buildDescriptorSet(RenderingContext& ctx, const VkDescriptorSetLayout& layout);
            void updateDescriptorSet(RenderingContext& ctx);
            void buildPipelineLayout(RenderingContext& ctx, const std::vector<VkDescriptorSetLayout>& desc_layouts);
            void buildPipeline(RenderingContext& ctx, VkPipelineLayout layout, VkRenderPass render_pass);

            bool is_compiled_;

            // std::shared_ptr<Texture> texture_;
            // name -> {resource, dirty_flag}
            std::unordered_map<std::string, VkDescriptorSetLayoutBinding> resource_layout_;
            std::unordered_map<std::string, std::pair<std::shared_ptr<void>, bool>> resource_data_;

            std::shared_ptr<Shader> vert_, frag_;
            VkPipelineInputAssemblyStateCreateInfo input_asm_;
            VkPipelineViewportStateCreateInfo viewport_;
            VkPipelineRasterizationStateCreateInfo rasterizer_;
            VkPipelineMultisampleStateCreateInfo multisampling_;
            VkPipelineDepthStencilStateCreateInfo depth_stencil_;
            VkPipelineColorBlendStateCreateInfo color_blending_;
            std::vector<VkPipelineColorBlendAttachmentState> blend_attachments_;
            std::vector<VkDynamicState> dynamic_states_;

            VkDescriptorSetLayout desc_layout_;
            VkDescriptorSet desc_set_;

            VkPipelineLayout pipeline_layout_;
            VkPipeline pipeline_;
        };
    }
}
