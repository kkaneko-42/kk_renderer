#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include "RenderingContext.h"

namespace kk {
    namespace renderer {
        // TODO: Use kk::renderer::Image
        struct Texture {
            static Texture create(RenderingContext& ctx, const std::string& path);
            static Texture create(
                RenderingContext& ctx,
                const std::string& neg_x_path,
                const std::string& pos_x_path,
                const std::string& neg_y_path,
                const std::string& pos_y_path,
                const std::string& neg_z_path,
                const std::string& pos_z_path
            );

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
            void transitionLayout(RenderingContext& ctx, VkImageLayout new_layout);

            VkImage image;
            VkDeviceMemory memory;
            VkImageView view;
            VkSampler sampler;

            uint32_t width;
            uint32_t height;
            VkFormat format;
            uint32_t array_layers;
            VkImageTiling tiling;
            VkImageLayout layout;
            VkImageUsageFlags usage;
            VkMemoryPropertyFlags props;
            VkImageAspectFlags aspect;
        };
    }
}
