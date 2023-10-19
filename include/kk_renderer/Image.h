#pragma once

#include <vulkan/vulkan.h>
#include "RenderingContext.h"

namespace kk {
    namespace renderer {
        struct Image {
            static Image create(
                RenderingContext& ctx,
                uint32_t width,
                uint32_t height,
                VkFormat format,
                VkImageTiling tiling,
                VkImageUsageFlags usage,
                VkMemoryPropertyFlags props,
                VkImageAspectFlags aspect
            );
            void destroy(RenderingContext& ctx);

            VkImage image;
            VkDeviceMemory memory;
            VkImageView view;

            uint32_t width;
            uint32_t height;
            VkFormat format;
            VkImageTiling tiling;
            VkImageUsageFlags usage;
            VkMemoryPropertyFlags props;
            VkImageAspectFlags aspect;
        };
    }
}
