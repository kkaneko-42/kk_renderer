#include "kk_renderer/GraphicsPipeline.h"

using namespace kk::renderer;

GraphicsPipeline::GraphicsPipeline() : layout_(VK_NULL_HANDLE), pipeline_(VK_NULL_HANDLE) {
    setDefault();
}

void GraphicsPipeline::setVertexShader(const std::shared_ptr<Shader>& vert) {
    shaders_[VK_SHADER_STAGE_VERTEX_BIT] = vert;
    is_warmed_up_ = false;
}

void GraphicsPipeline::setFragmentShader(const std::shared_ptr<Shader>& frag) {
    shaders_[VK_SHADER_STAGE_FRAGMENT_BIT] = frag;
    is_warmed_up_ = false;
}

void GraphicsPipeline::warmUp(RenderingContext& ctx, VkRenderPass render_pass) {
    vkDeviceWaitIdle(ctx.device);
    vkDestroyPipelineLayout(ctx.device, layout_, nullptr);
    vkDestroyPipeline(ctx.device, pipeline_, nullptr);
    layout_ = VK_NULL_HANDLE;
    pipeline_ = VK_NULL_HANDLE;

    VkDescriptorSetLayout desc_layout = createDescLayout(ctx);
    layout_ = createLayout(ctx, desc_layout);

    VkPipelineShaderStageCreateInfo vert_info{};
    vert_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_info.module = shaders_[VK_SHADER_STAGE_VERTEX_BIT]->module;
    vert_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_info{};
    frag_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_info.module = shaders_[VK_SHADER_STAGE_FRAGMENT_BIT]->module;
    frag_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = { vert_info, frag_info };

    const auto binding_desc = Vertex::getBindingDescription();
    const auto attr_desc = Vertex::getAttributeDescriptions();
    VkPipelineVertexInputStateCreateInfo vert_input{};
    vert_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vert_input.vertexBindingDescriptionCount = 1;
    vert_input.pVertexBindingDescriptions = &binding_desc;
    vert_input.vertexAttributeDescriptionCount = static_cast<uint32_t>(attr_desc.size());
    vert_input.pVertexAttributeDescriptions = attr_desc.data();

    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states_.size());
    dynamic_state.pDynamicStates = dynamic_states_.data();

    VkGraphicsPipelineCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.stageCount = 2;
    info.pStages = shader_stages;
    info.pVertexInputState = &vert_input;
    info.pInputAssemblyState = &input_asm_;
    info.pViewportState = &viewport_;
    info.pRasterizationState = &rasterizer_;
    info.pMultisampleState = &multisampling_;
    info.pColorBlendState = &color_blending_;
    info.pDynamicState = &dynamic_state;
    info.layout = layout_;
    info.renderPass = render_pass;
    info.subpass = 0; // TODO

    assert(vkCreateGraphicsPipelines(ctx.device, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline_) == VK_SUCCESS);

    is_warmed_up_ = true;
}

VkDescriptorSetLayout GraphicsPipeline::createDescLayout(RenderingContext& ctx) {
    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    for (const auto& shader : shaders_) {
        for (const auto& binding : shader.second->bindings) {
            bindings.push_back(binding.second);
        }
    }

    info.bindingCount = static_cast<uint32_t>(bindings.size());
    info.pBindings = bindings.data();
    
    VkDescriptorSetLayout desc_layout;
    assert(vkCreateDescriptorSetLayout(ctx.device, &info, nullptr, &desc_layout) == VK_SUCCESS);

    return desc_layout;
}

VkPipelineLayout GraphicsPipeline::createLayout(RenderingContext& ctx, VkDescriptorSetLayout desc_layout) {
    VkPipelineLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.setLayoutCount = 1;
    info.pSetLayouts = &desc_layout;
    
    VkPipelineLayout layout;
    assert(vkCreatePipelineLayout(ctx.device, &info, nullptr, &layout) == VK_SUCCESS);

    return layout;
}

void GraphicsPipeline::setDefault() {
    input_asm_.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_asm_.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_asm_.primitiveRestartEnable = VK_FALSE;

    viewport_.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_.viewportCount = 1;
    viewport_.scissorCount = 1;

    rasterizer_.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_.depthClampEnable = VK_FALSE;
    rasterizer_.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer_.lineWidth = 1.0f;
    rasterizer_.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer_.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer_.depthBiasEnable = VK_FALSE;

    multisampling_.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_.sampleShadingEnable = VK_FALSE;
    multisampling_.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    blend_attachments_.resize(1);
    blend_attachments_[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blend_attachments_[0].blendEnable = VK_FALSE;

    color_blending_.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending_.logicOpEnable = VK_FALSE;
    color_blending_.logicOp = VK_LOGIC_OP_COPY;
    color_blending_.attachmentCount = static_cast<uint32_t>(blend_attachments_.size());
    color_blending_.pAttachments = blend_attachments_.data();
    color_blending_.blendConstants[0] = 0.0f;
    color_blending_.blendConstants[1] = 0.0f;
    color_blending_.blendConstants[2] = 0.0f;
    color_blending_.blendConstants[3] = 0.0f;

    dynamic_states_ = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
}
