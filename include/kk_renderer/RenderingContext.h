#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <functional>

namespace kk {
    namespace renderer {
        constexpr size_t kMaxConcurrentFrames = 2;

        struct RenderingContext {
            VkInstance instance;
            VkDebugUtilsMessengerEXT debug_messenger;
            VkPhysicalDevice gpu;
            uint32_t graphics_family, present_family;
            VkDevice device;
            VkQueue graphics_queue, present_queue;
            VkCommandPool cmd_pool;
            VkDescriptorPool desc_pool;
            std::array<VkFence, kMaxConcurrentFrames> fences;
            std::array<VkSemaphore, kMaxConcurrentFrames> render_complete;
            std::array<VkSemaphore, kMaxConcurrentFrames> present_complete;

            static RenderingContext create();
            void destroy();
            VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
            uint32_t findMemoryType(uint32_t type_filter, VkMemoryPropertyFlags props);
            // VkCommandBuffer beginSingleTimeCommandBuffer();
            // void endSingleTimeCommandBuffer(VkCommandBuffer cmd);
            void submitCmdsImmediate(std::function<void(VkCommandBuffer)> cmds_recorder);
            void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout);
        };
    }
}
