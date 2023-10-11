#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace kk {
    namespace renderer {
        struct RenderingContext {
            VkInstance instance;
            VkDebugUtilsMessengerEXT debug_messenger;
            VkPhysicalDevice gpu;
            uint32_t graphics_family, present_family;
            VkDevice device;
            VkQueue graphics_queue, present_queue;
            VkCommandPool cmd_pool;

            static RenderingContext create();
            void destroy();
        };
    }
}
