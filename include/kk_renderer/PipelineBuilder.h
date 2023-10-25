#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include "RenderingContext.h"
#include "Shader.h"

namespace kk {
    namespace renderer {
        // NOTE: Should be internal ?
        class PipelineBuilder {
        public:
            VkPipeline build(RenderingContext& ctx, uint32_t subpass, VkPipelineLayout layout, VkRenderPass render_pass);
            
            PipelineBuilder& setDefault();
            
            inline PipelineBuilder& setVertexShader(const std::shared_ptr<Shader>& vert) {
                vert_ = vert;
                return *this;
            }

            inline PipelineBuilder& setFragmentShader(const std::shared_ptr<Shader>& frag) {
                frag_ = frag;
                return *this;
            }

            inline PipelineBuilder& setFrontFace(VkFrontFace front_face) {
                rasterizer_.frontFace = front_face;
                return *this;
            }

        private:
            std::shared_ptr<Shader> vert_, frag_;
            VkPipelineInputAssemblyStateCreateInfo input_asm_;
            VkPipelineViewportStateCreateInfo viewport_;
            VkPipelineRasterizationStateCreateInfo rasterizer_;
            VkPipelineMultisampleStateCreateInfo multisampling_;
            VkPipelineDepthStencilStateCreateInfo depth_stencil_;
            VkPipelineColorBlendStateCreateInfo color_blending_;
            std::vector<VkPipelineColorBlendAttachmentState> blend_attachments_;
            std::vector<VkDynamicState> dynamic_states_;
        };
    }
}
