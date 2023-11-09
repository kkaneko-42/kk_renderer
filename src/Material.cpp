#include "kk_renderer/Material.h"
#include "kk_renderer/Vertex.h"
#include <cassert>
#include <iostream>
#include <algorithm>
#include <map>

using namespace kk::renderer;

Material::Material() :
    is_compiled_(false),
    pipeline_layout_(VK_NULL_HANDLE),
    pipeline_(VK_NULL_HANDLE) {
    setDefault();
}

void Material::destroy(RenderingContext& ctx) {
    vkDestroyPipeline(ctx.device, pipeline_, nullptr);
    vkDestroyPipelineLayout(ctx.device, pipeline_layout_, nullptr);
    vkDestroyDescriptorSetLayout(ctx.device, desc_layout_, nullptr);
}

void Material::compile(
    RenderingContext& ctx,
    VkRenderPass render_pass,
    VkDescriptorSetLayout per_view_layout,
    VkDescriptorSetLayout per_object_layout
) {
    // TODO: Destroy resources existing already
    
    buildDescLayout(ctx);
    buildDescriptorSet(ctx, desc_layout_);
    buildPipelineLayout(ctx, {per_view_layout, desc_layout_, per_object_layout});
    buildPipeline(ctx, pipeline_layout_, render_pass);

    is_compiled_ = true;
}

void Material::buildDescLayout(RenderingContext& ctx) {
    // Gather bindings
    const auto& vert_bindings = vert_->getResourceLayout();
    resource_layout_ = vert_bindings;

    const auto& frag_bindings = frag_->getResourceLayout();
    for (const auto& kvp : frag_bindings) {
        const auto& name = kvp.first;
        if (resource_layout_.count(name) == 0) {
            resource_layout_[name] = kvp.second;
        } else {
            resource_layout_[name].stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
        }
    }

    std::vector<VkDescriptorSetLayoutBinding> bindings;
    bindings.reserve(resource_layout_.size());
    for (const auto& kvp : resource_layout_) {
        bindings.push_back(kvp.second);
    }

    // Create descriptor set layouts
    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = static_cast<uint32_t>(bindings.size());
    info.pBindings = bindings.data();

    assert(vkCreateDescriptorSetLayout(ctx.device, &info, nullptr, &desc_layout_) == VK_SUCCESS);
}

void Material::buildDescriptorSet(RenderingContext& ctx, const VkDescriptorSetLayout& layout) {
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = ctx.desc_pool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &layout;
    assert(vkAllocateDescriptorSets(ctx.device, &alloc_info, &desc_set_) == VK_SUCCESS);

    updateDescriptorSet(ctx);
}

void Material::updateDescriptorSet(RenderingContext& ctx) {
    std::vector<VkWriteDescriptorSet> writes;
    for (const auto& kvp : resource_layout_) {
        const auto& name = kvp.first;
        if (resource_data_.count(name) == 0) {
            std::cerr << "WARNING: Material param \"" << name << "\" is not set" << std::endl;
            continue;
        } else if (!resource_data_[name].second) {
            // Resource is not changed
            continue;
        }

        const auto& type = resource_layout_[name].descriptorType;
        VkWriteDescriptorSet desc_write{};
        desc_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc_write.dstSet = desc_set_;
        desc_write.dstBinding = resource_layout_[name].binding;
        desc_write.descriptorCount = 1;
        desc_write.descriptorType = type;

        const auto& data = resource_data_[name].first;
        if (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
            const auto buf_data = std::static_pointer_cast<Buffer>(data);
            // TODO: Validate resource type
            if (buf_data == nullptr) {
                std::cerr << "WARNING: Material param \"" << name << "\" have wrong resource type (Buffer required)" << std::endl;
            } else {
                auto* buf_info = new VkDescriptorBufferInfo();
                buf_info->buffer = buf_data->buffer;
                buf_info->offset = 0;
                buf_info->range  = buf_data->size;
                desc_write.pBufferInfo = buf_info;
            }
        } else if (type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
            const auto img_data = std::static_pointer_cast<Texture>(data);
            // TODO: Validate resource type
            if (img_data == nullptr) {
                std::cerr << "WARNING: Material param \"" << name << "\" have wrong resource type (Texture required)" << std::endl;
            } else {
                auto* img_info = new VkDescriptorImageInfo();
                img_info->imageLayout   = img_data->layout;
                img_info->imageView     = img_data->view;
                img_info->sampler       = img_data->sampler;
                desc_write.pImageInfo = img_info;
            }
        } else {
            std::cerr << "UNSUPPORTED: Resource type: " << type << std::endl;
        }
        writes.push_back(desc_write);
    }
    vkUpdateDescriptorSets(ctx.device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

    // Cleanup
    for (const auto& w : writes) {
        delete w.pBufferInfo;
        delete w.pImageInfo;
    }
}

void Material::buildPipelineLayout(RenderingContext& ctx, const std::vector<VkDescriptorSetLayout>& desc_layouts) {
    VkPipelineLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.setLayoutCount = static_cast<uint32_t>(desc_layouts.size());
    info.pSetLayouts = desc_layouts.data();

    assert(vkCreatePipelineLayout(ctx.device, &info, nullptr, &pipeline_layout_) == VK_SUCCESS);
}

void Material::buildPipeline(RenderingContext& ctx, VkPipelineLayout layout, VkRenderPass render_pass) {
    std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};
    // Set vertex shader info
    shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stages[0].module = vert_->get();
    shader_stages[0].pName = "main";
    // Set fragment shader info
    shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stages[1].module = frag_->get();
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

    assert(vkCreateGraphicsPipelines(ctx.device, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline_) == VK_SUCCESS);
}

void Material::setDefault() {
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
    rasterizer_.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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
}
