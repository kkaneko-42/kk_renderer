#include "kk_renderer/PipelineBuilder.h"
#include "kk_renderer/Vertex.h"

using namespace kk::renderer;

VkPipeline PipelineBuilder::build(
    RenderingContext& ctx,
    uint32_t subpass,
    VkPipelineLayout layout,
    VkRenderPass render_pass
) {
    std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};
    // Set vertex shader info
    shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stages[0].module = vert_->module;
    shader_stages[0].pName = "main";
    // Set fragment shader info
    shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stages[1].module = frag_->module;
    shader_stages[1].pName = "main";

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

    color_blending_.attachmentCount = static_cast<uint32_t>(blend_attachments_.size());
    color_blending_.pAttachments = blend_attachments_.data();

    VkGraphicsPipelineCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.stageCount = static_cast<uint32_t>(shader_stages.size());
    info.pStages = shader_stages.data();
    info.pVertexInputState = &vert_input;
    info.pInputAssemblyState = &input_asm_;
    info.pViewportState = &viewport_;
    info.pRasterizationState = &rasterizer_;
    info.pMultisampleState = &multisampling_;
    info.pDepthStencilState = &depth_stencil_;
    info.pColorBlendState = &color_blending_;
    info.pDynamicState = &dynamic_state;
    info.layout = layout;
    info.renderPass = render_pass;
    info.subpass = 0; // TODO

    VkPipeline pipeline;
    assert(vkCreateGraphicsPipelines(ctx.device, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline) == VK_SUCCESS);
    return pipeline;
}

PipelineBuilder& PipelineBuilder::setDefault() {
    input_asm_ = {};
    input_asm_.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_asm_.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_asm_.primitiveRestartEnable = VK_FALSE;

    viewport_ = {};
    viewport_.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_.viewportCount = 1;
    viewport_.scissorCount = 1;

    rasterizer_ = {};
    rasterizer_.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_.depthClampEnable = VK_FALSE;
    rasterizer_.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer_.lineWidth = 1.0f;
    rasterizer_.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer_.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer_.depthBiasEnable = VK_FALSE;

    multisampling_ = {};
    multisampling_.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_.sampleShadingEnable = VK_FALSE;
    multisampling_.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    depth_stencil_ = {};
    depth_stencil_.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_.depthTestEnable = VK_TRUE;
    depth_stencil_.depthWriteEnable = VK_TRUE;
    depth_stencil_.depthCompareOp = VK_COMPARE_OP_LESS;

    blend_attachments_.resize(1);
    blend_attachments_[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blend_attachments_[0].blendEnable = VK_FALSE;

    color_blending_ = {};
    color_blending_.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending_.logicOpEnable = VK_FALSE;
    color_blending_.logicOp = VK_LOGIC_OP_COPY;
    color_blending_.blendConstants[0] = 0.0f;
    color_blending_.blendConstants[1] = 0.0f;
    color_blending_.blendConstants[2] = 0.0f;
    color_blending_.blendConstants[3] = 0.0f;

    dynamic_states_ = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    return *this;
}
