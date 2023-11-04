#pragma once

#include <vulkan/vulkan.h>
#include "Window.h"
#include "RenderingContext.h"

namespace kk {
    namespace renderer {
        /*
        struct Swapchain {
            static Swapchain create(RenderingContext& ctx, Window& window);
            void destroy(RenderingContext& ctx);

            VkSurfaceKHR surface;

            VkSurfaceFormatKHR surface_format;
            VkPresentModeKHR present_mode;
            VkExtent2D extent;
            VkSurfaceTransformFlagBitsKHR pre_transform;
            std::vector<VkImage> images;
            std::vector<VkImageView> views;
            VkSwapchainKHR swapchain;
        };
        */
    }
}
