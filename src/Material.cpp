#include "kk_renderer/Material.h"
#include "kk_renderer/Vertex.h"
#include <cassert>
#include <iostream>

using namespace kk::renderer;

Material::Material() :
    is_compiled_(false),
    pipeline_layout_(VK_NULL_HANDLE),
    pipeline_(VK_NULL_HANDLE),
    desc_layout_(VK_NULL_HANDLE) {
    setDefault();
}

void Material::destroy(RenderingContext& ctx) {
    vkFreeDescriptorSets(ctx.device, ctx.desc_pool, static_cast<uint32_t>(desc_sets_.size()), desc_sets_.data());
    vkDestroyPipeline(ctx.device, pipeline_, nullptr);
    vkDestroyPipelineLayout(ctx.device, pipeline_layout_, nullptr);
    vkDestroyDescriptorSetLayout(ctx.device, desc_layout_, nullptr);
}

void Material::setVertexShader(const std::shared_ptr<Shader>& vert) {
    shaders_[VK_SHADER_STAGE_VERTEX_BIT] = vert;
    is_compiled_ = false;
}

void Material::setFragmentShader(const std::shared_ptr<Shader>& frag) {
    shaders_[VK_SHADER_STAGE_FRAGMENT_BIT] = frag;
    is_compiled_ = false;
}

void Material::setBuffer(
    uint32_t binding,
    const std::shared_ptr<Buffer>& buffer,
    VkDescriptorType type,
    VkShaderStageFlags stage,
    VkDeviceSize offset,
    VkDeviceSize range
) {
    VkDescriptorSetLayoutBinding layout_binding{};
    layout_binding.binding = binding;
    layout_binding.descriptorCount = 1;
    layout_binding.descriptorType = type;
    layout_binding.stageFlags = stage;

    std::shared_ptr<VkDescriptorBufferInfo> buf_info(new VkDescriptorBufferInfo());
    buf_info->buffer = buffer->buffer;
    buf_info->offset = offset;
    buf_info->range = range;

    addBindingInfo(layout_binding, std::move(buf_info), buffer);

    is_compiled_ = false;
}

void Material::setTexture(
    uint32_t binding,
    const std::shared_ptr<Texture>& texture,
    VkDescriptorType type,
    VkShaderStageFlags stage
) {
    VkDescriptorSetLayoutBinding layout_binding{};
    layout_binding.binding = binding;
    layout_binding.descriptorCount = 1;
    layout_binding.descriptorType = type;
    layout_binding.stageFlags = stage;

    std::shared_ptr<VkDescriptorImageInfo> img_info(new VkDescriptorImageInfo());
    // img_info->imageLayout = texture->layout;
    img_info->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // TODO: support other layout
    img_info->imageView = texture->view;
    img_info->sampler = texture->sampler;

    addBindingInfo(layout_binding, std::move(img_info), texture);

    is_compiled_ = false;
}

std::shared_ptr<Buffer> Material::getBuffer(uint32_t binding) {
    // TODO: Validation
    return std::static_pointer_cast<Buffer>(bindings_[binding].resource);
}

void Material::compile(RenderingContext& ctx, VkRenderPass render_pass) {
    // TODO: Destroy resources existing already
    
    buildDescLayout(ctx);
    buildDescSets(ctx, desc_layout_);
    buildPipelineLayout(ctx, desc_layout_);
    buildPipeline(ctx, pipeline_layout_, render_pass);

    is_compiled_ = true;
}

void Material::buildDescLayout(RenderingContext& ctx) {
    std::vector<VkDescriptorSetLayoutBinding> bindings(bindings_.size());
    for (uint32_t i = 0; i < bindings_.size(); ++i) {
        bindings[i] = bindings_[i].layout;
    }

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = static_cast<uint32_t>(bindings.size());
    info.pBindings = bindings.data();

    assert(vkCreateDescriptorSetLayout(ctx.device, &info, nullptr, &desc_layout_) == VK_SUCCESS);
}

void Material::buildDescSets(RenderingContext& ctx, VkDescriptorSetLayout layout) {
    for (size_t i = 0; i < kMaxConcurrentFrames; ++i) {
        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = ctx.desc_pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &layout;
        assert(vkAllocateDescriptorSets(ctx.device, &alloc_info, &desc_sets_[i]) == VK_SUCCESS);

        for (const auto& bind_info : bindings_) {
            const uint32_t binding = bind_info.layout.binding;

            VkWriteDescriptorSet desc_write{};
            desc_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            desc_write.dstSet = desc_sets_[i];
            desc_write.dstBinding = binding;
            desc_write.descriptorType = bind_info.layout.descriptorType;
            desc_write.descriptorCount = 1;

            switch (bind_info.layout.descriptorType) {
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                desc_write.pBufferInfo = static_cast<VkDescriptorBufferInfo*>(bind_info.bind_info.get());
                break;

            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
                desc_write.pImageInfo = static_cast<VkDescriptorImageInfo*>(bind_info.bind_info.get());
                break;
            }

            default:
                std::cerr << "ResourceDescriptor::buildSets(): Error: Unsupported descriptor type: " << bind_info.layout.descriptorType << std::endl;
                assert(false);
                break;
            }

            vkUpdateDescriptorSets(ctx.device, 1, &desc_write, 0, nullptr);
        }
    }
}

void Material::buildPipelineLayout(RenderingContext& ctx, VkDescriptorSetLayout desc_layout) {
    VkPipelineLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.setLayoutCount = 1;
    info.pSetLayouts = &desc_layout;

    assert(vkCreatePipelineLayout(ctx.device, &info, nullptr, &pipeline_layout_) == VK_SUCCESS);
}

void Material::buildPipeline(RenderingContext& ctx, VkPipelineLayout layout, VkRenderPass render_pass) {
    std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
    shader_stages.reserve(shaders_.size());
    for (const auto& kvp : shaders_) {
        VkPipelineShaderStageCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        info.stage = kvp.first;
        info.module = kvp.second->module;
        info.pName = "main";
        shader_stages.push_back(info);
    }

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
    rasterizer_.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer_.depthBiasEnable = VK_FALSE;

    multisampling_ = {};
    multisampling_.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_.sampleShadingEnable = VK_FALSE;
    multisampling_.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

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

void Material::addBindingInfo(
    const VkDescriptorSetLayoutBinding& layout,
    const std::shared_ptr<void>& bind_info,
    const std::shared_ptr<void>& resource
) {
    const uint32_t binding = layout.binding;
    if (binding >= bindings_.size()) {
        // TODO: Avoid frequent allocation
        bindings_.resize(static_cast<size_t>(layout.binding) + 1);
    }

    bindings_[binding].layout = layout;
    bindings_[binding].bind_info = bind_info;
    bindings_[binding].resource = resource;
}
