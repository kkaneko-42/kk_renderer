#pragma once

#include <vulkan/vulkan.h>
#include "RenderingContext.h"

namespace kk {
    namespace renderer {
        struct Texture {
            static Texture create(
                RenderingContext& ctx,
                uint32_t width,
                uint32_t height,
                VkFormat format,
                VkImageTiling tiling,
                VkImageUsageFlags usage,
                VkMemoryPropertyFlags props,
                VkImageAspectFlags aspect
            );

            VkImage image;
            VkDeviceMemory memory;
            VkImageView view;
        };
    }
}
