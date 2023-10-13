#include "kk_renderer/Buffer.h"
#include <cassert>

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
    vkFreeMemory(ctx.device, memory, nullptr);
    vkDestroyBuffer(ctx.device, buffer, nullptr);
}
