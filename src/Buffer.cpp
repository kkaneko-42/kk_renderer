#include "kk_renderer/Buffer.h"
#include <iostream>
#include <cassert>
#include <cstring>

using namespace kk::renderer;

Buffer Buffer::create(RenderingContext& ctx, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags mem_props) {
    Buffer buffer{};

    VkBufferCreateInfo buf_info{};
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.size = size;
    buf_info.usage = usage;
    buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    assert(vkCreateBuffer(ctx.device, &buf_info, nullptr, &buffer.buffer) == VK_SUCCESS);

    VkMemoryRequirements mem_reqs{};
    vkGetBufferMemoryRequirements(ctx.device, buffer.buffer, &mem_reqs);
    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_reqs.size;
    alloc_info.memoryTypeIndex = ctx.findMemoryType(mem_reqs.memoryTypeBits, mem_props);
    assert(vkAllocateMemory(ctx.device, &alloc_info, nullptr, &buffer.memory) == VK_SUCCESS);

    assert(vkBindBufferMemory(ctx.device, buffer.buffer, buffer.memory, 0) == VK_SUCCESS);

    buffer.size = size;
    buffer.usage = usage;
    buffer.memory_props = mem_props;

    return buffer;
}

void Buffer::destroy(RenderingContext& ctx) {
    vkDeviceWaitIdle(ctx.device); // CONCERN
    vkFreeMemory(ctx.device, memory, nullptr);
    vkDestroyBuffer(ctx.device, buffer, nullptr);
}

void Buffer::setData(RenderingContext& ctx, const void* data, size_t src_size) {
    const size_t set_size = std::min(size, src_size);
    if (src_size > size) {
        std::cerr << "Buffer::setData(): WARNING: src size (" << src_size << ") is larger than buffer size (" << size << "). ";
        std::cerr << "Set up to " << size << " byte." << std::endl;
    }

    if (memory_props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        void* mapped;
        vkMapMemory(ctx.device, memory, 0, set_size, 0, &mapped);
        std::memcpy(mapped, data, set_size);
        vkUnmapMemory(ctx.device, memory);
    }
    else {
        Buffer staging = Buffer::create(
            ctx,
            set_size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        staging.setData(ctx, data, set_size);
        staging.copyTo(ctx, *this, set_size);
        staging.destroy(ctx);
    }
}

void Buffer::copyTo(RenderingContext& ctx, Buffer& dst, VkDeviceSize copy_size) const {
    VkCommandBuffer cmd_buf = ctx.beginSingleTimeCommandBuffer();

    VkBufferCopy copy_region{};
    copy_region.size = copy_size;
    vkCmdCopyBuffer(cmd_buf, buffer, dst.buffer, 1, &copy_region);

    ctx.endSingleTimeCommandBuffer(cmd_buf);
}
