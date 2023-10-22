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

            inline void setTexture(const std::shared_ptr<Texture>& texture) {
                texture_ = texture;
                // TODO: set dirty flag true
            }

            // CONCERN: Vulkan abstraction
            inline void setFrontFace(VkFrontFace front_face) {
                rasterizer_.frontFace = front_face;
            }

            inline std::shared_ptr<Texture> getTexture() const {
                return texture_;
            }

            void compile(RenderingContext& ctx, VkRenderPass render_pass);

            inline bool isCompiled() const { return is_compiled_; }
            inline VkPipeline getPipeline() const { return pipeline_; }
            inline VkPipelineLayout getPipelineLayout() const { return pipeline_layout_; }
            inline const std::vector<VkDescriptorSetLayout>& getDescriptorSetLayouts() const { return desc_layouts_; }
            inline const VkDescriptorSet& getDescriptorSet() const { return desc_set_; }

        private:
            void setDefault();
            void buildDescLayout(RenderingContext& ctx);
            void buildDescriptorSets(RenderingContext& ctx, const VkDescriptorSetLayout& layouts);
            void buildPipelineLayout(RenderingContext& ctx, const std::vector<VkDescriptorSetLayout>& desc_layouts);
            void buildPipeline(RenderingContext& ctx, VkPipelineLayout layout, VkRenderPass render_pass);

            bool is_compiled_;

            std::shared_ptr<Texture> texture_;

            std::shared_ptr<Shader> vert_, frag_;
            VkPipelineInputAssemblyStateCreateInfo input_asm_;
            VkPipelineViewportStateCreateInfo viewport_;
            VkPipelineRasterizationStateCreateInfo rasterizer_;
            VkPipelineMultisampleStateCreateInfo multisampling_;
            VkPipelineDepthStencilStateCreateInfo depth_stencil_;
            VkPipelineColorBlendStateCreateInfo color_blending_;
            std::vector<VkPipelineColorBlendAttachmentState> blend_attachments_;
            std::vector<VkDynamicState> dynamic_states_;

            std::vector<VkDescriptorSetLayout> desc_layouts_;

            VkPipelineLayout pipeline_layout_;
            VkPipeline pipeline_;

            VkDescriptorSet desc_set_;
        };
    }
}
