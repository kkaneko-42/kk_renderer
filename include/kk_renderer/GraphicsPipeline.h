#pragma once

#include <vulkan/vulkan.h>
#include <unordered_map>
#include <memory>
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

            void warmup(RenderingContext& ctx, const Renderer& renderer);

        private:
            void setDefault();

            VkPipeline pipeline_;
            VkPipelineLayout layout_;
            bool is_warmup_;

            std::shared_ptr<Shader> vert_, frag_;
            std::vector<>
            VkPipelineVertexInputStateCreateInfo vert_input_;
            VkPipelineInputAssemblyStateCreateInfo input_asm_;
            VkPipelineViewportStateCreateInfo viewport_;
            VkPipelineRasterizationStateCreateInfo rasterizer_;
            VkPipelineMultisampleStateCreateInfo multisampling_;
            VkPipelineColorBlendStateCreateInfo color_blending_;
            VkPipelineDynamicStateCreateInfo dynamic_state_;
        };
    }
}
