#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <array>

namespace kk {
    namespace renderer {
        constexpr size_t kAsyncRenderingCount = 3;

        struct RenderingContext {
            VkInstance instance;
            VkDebugUtilsMessengerEXT debug_messenger;
            VkPhysicalDevice gpu;
            uint32_t graphics_family, present_family;
            VkDevice device;
            VkQueue graphics_queue, present_queue;
            VkCommandPool cmd_pool;
            std::array<VkFence, kAsyncRenderingCount> fences;

            static RenderingContext create();
            void destroy();
        };
    }
}
