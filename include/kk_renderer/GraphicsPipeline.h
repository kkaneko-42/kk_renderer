#pragma once

#include <vulkan/vulkan.h>
#include <unordered_map>
#include <memory>
#include <vector>
#include "RenderingContext.h"
#include "Renderer.h"
#include "Shader.h"

namespace kk {
    namespace renderer {
        class GraphicsPipeline {
        public:
            GraphicsPipeline();

            void setVertexShader(const std::shared_ptr<Shader>& vert);
            void setFragmentShader(const std::shared_ptr<Shader>& frag);

            void warmUp(RenderingContext& ctx, VkRenderPass render_pass);

            inline bool isWarmedUp() const { return is_warmed_up_; }
            inline VkPipeline get() const { return pipeline_; }
            inline VkPipelineLayout getLayout() const { return layout_; }

        private:
            void setDefault();
            VkDescriptorSetLayout createDescLayout(RenderingContext& ctx);
            VkPipelineLayout createLayout(RenderingContext& ctx, VkDescriptorSetLayout desc_layout);

            VkPipeline pipeline_;
            VkPipelineLayout layout_;
            bool is_warmed_up_;

            std::unordered_map<VkShaderStageFlags, std::shared_ptr<Shader>> shaders_;

            VkPipelineInputAssemblyStateCreateInfo input_asm_;
            VkPipelineViewportStateCreateInfo viewport_;
            VkPipelineRasterizationStateCreateInfo rasterizer_;
            VkPipelineMultisampleStateCreateInfo multisampling_;
            VkPipelineColorBlendStateCreateInfo color_blending_;
            std::vector<VkPipelineColorBlendAttachmentState> blend_attachments_;
            std::vector<VkDynamicState> dynamic_states_;
        };
    }
}
