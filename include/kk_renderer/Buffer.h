#pragma once

#include <vulkan/vulkan.h>
#include "RenderingContext.h"

namespace kk {
    namespace renderer {
        struct Buffer {
            static Buffer create(RenderingContext& ctx, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags mem_props);
            void destroy(RenderingContext& ctx);
            void setData(RenderingContext& ctx, const void* src, size_t src_size);
            void copyTo(RenderingContext& ctx, Buffer& dst, VkDeviceSize copy_size) const;

            VkBuffer buffer;
            VkDeviceMemory memory;
            VkDeviceSize size;
            VkBufferUsageFlags usage;
            VkMemoryPropertyFlags memory_props;
        };
    }
}
