#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include "RenderingContext.h"

namespace kk {
    namespace renderer {
        struct Texture {
            static Texture create(RenderingContext& ctx, const std::string& path);

            static Texture create(
                RenderingContext& ctx,
                const void* texels,
                size_t texel_byte,
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
            VkSampler sampler;

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
