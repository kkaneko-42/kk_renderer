#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace kk {
    namespace renderer {
        struct RenderingContext {
            VkInstance instance;
            VkDebugUtilsMessengerEXT debug_messenger;
            VkPhysicalDevice gpu;
            uint32_t queue_family;
            VkDevice device;
            VkQueue queue;
            VkCommandPool cmd_pool;

            static RenderingContext create();
            static void destory(RenderingContext& ctx);
        };
    }
}
