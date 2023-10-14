#include "kk_renderer/ResourceDescriptor.h"
#include <iostream>
#include <cassert>
#include <algorithm>
#include <memory>

using namespace kk::renderer;

ResourceDescriptor::ResourceDescriptor() : layout_(VK_NULL_HANDLE), set_(VK_NULL_HANDLE) {}

void ResourceDescriptor::destroy(RenderingContext& ctx) {
    if (set_ != VK_NULL_HANDLE) {
        vkFreeDescriptorSets(ctx.device, ctx.desc_pool, 1, &set_);
    }
    if (layout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(ctx.device, layout_, nullptr);
    }
}

void ResourceDescriptor::bindBuffer(
    uint32_t binding,
    const std::shared_ptr<Buffer>& buffer,
    VkDescriptorType type,
    VkShaderStageFlags stage
) {
    VkDescriptorSetLayoutBinding layout_binding{};
    layout_binding.binding = binding;
    layout_binding.descriptorCount = 1;
    layout_binding.descriptorType = type;
    layout_binding.stageFlags = stage;

    resources_[binding].first = layout_binding;
    resources_[binding].second = std::static_pointer_cast<void>(buffer);
}

std::shared_ptr<Buffer> ResourceDescriptor::getBuffer(uint32_t binding) {
    // TODO: Validation
    return std::static_pointer_cast<Buffer>(resources_[binding].second);
}

// NOTE: Once called, return same object
void ResourceDescriptor::buildLayout(RenderingContext& ctx) {
    std::vector<VkDescriptorSetLayoutBinding> bindings(resources_.size());
    for (uint32_t i = 0; i < resources_.size(); ++i) {
        bindings[i] = resources_[i].first;
    }

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = static_cast<uint32_t>(bindings.size());
    info.pBindings = bindings.data();

    assert(vkCreateDescriptorSetLayout(ctx.device, &info, nullptr, &layout_) == VK_SUCCESS);
}

// NOTE: Once called, return same object
void ResourceDescriptor::buildSet(RenderingContext& ctx) {
    if (layout_ == VK_NULL_HANDLE) {
        // Build set layout
        buildLayout(ctx);
    }

    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = ctx.desc_pool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &layout_;

    assert(vkAllocateDescriptorSets(ctx.device, &alloc_info, &set_) == VK_SUCCESS);

    for (const auto& kvp : resources_) {
        const uint32_t binding = kvp.first;
        const VkDescriptorSetLayoutBinding layout_binding = kvp.second.first;
        const void* resource = kvp.second.second.get();

        VkWriteDescriptorSet desc_write{};
        desc_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc_write.dstSet = set_;
        desc_write.dstBinding = binding;
        desc_write.descriptorType = layout_binding.descriptorType;
        desc_write.descriptorCount = 1;

        // Create descriptor resource info
        VkDescriptorBufferInfo buf_info{};
        switch (layout_binding.descriptorType) {
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: { // NOTE: buf_resource should be scoped
            const Buffer* buf_resource = static_cast<const Buffer*>(resource);
            buf_info.buffer = buf_resource->buffer;
            buf_info.offset = 0;
            buf_info.range = buf_resource->size;
            desc_write.pBufferInfo = &buf_info;
            break;
        }

        default:
            std::cerr << "ResourceDescriptor::buildSets(): Error: Unsupported descriptor type: " << layout_binding.descriptorType << std::endl;
            assert(false);
            break;
        }

        vkUpdateDescriptorSets(ctx.device, 1, &desc_write, 0, nullptr);
    }
}
