#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <functional>
#include "Window.h"

namespace kk {
    namespace renderer {
        constexpr size_t kMaxConcurrentFrames = 2;

        struct Swapchain {
            struct SupportInfo {
                VkSurfaceCapabilitiesKHR caps;
                std::vector<VkSurfaceFormatKHR> formats;
                std::vector<VkPresentModeKHR> present_modes;

                static SupportInfo query(VkPhysicalDevice gpu, VkSurfaceKHR);
            }   availables;

            VkSurfaceFormatKHR format;
            VkPresentModeKHR present_mode;
            VkSurfaceTransformFlagBitsKHR pre_transform;
            VkExtent2D extent;
            std::vector<VkImage> images;
            std::vector<VkImageView> views;
            VkSwapchainKHR swapchain;
        };

        struct RenderingContext {
            VkInstance instance;
            VkDebugUtilsMessengerEXT debug_messenger;
            VkSurfaceKHR surface;
            VkPhysicalDevice gpu;
            uint32_t graphics_family, present_family;
            VkDevice device;
            VkQueue graphics_queue, present_queue;
            Swapchain swapchain;
            VkCommandPool cmd_pool;
            VkDescriptorPool desc_pool;
            std::array<VkFence, kMaxConcurrentFrames> fences;
            std::array<VkSemaphore, kMaxConcurrentFrames> render_complete;
            std::array<VkSemaphore, kMaxConcurrentFrames> present_complete;

            static RenderingContext create(const WindowPtr& window);
            void destroy();
            VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
            uint32_t findMemoryType(uint32_t type_filter, VkMemoryPropertyFlags props);
            bool findFormat(const std::vector<VkFormat>& cands, VkImageTiling tiling, VkFormatFeatureFlags features, VkFormat* result);
            // VkCommandBuffer beginSingleTimeCommandBuffer();
            // void endSingleTimeCommandBuffer(VkCommandBuffer cmd);
            void submitCmdsImmediate(std::function<void(VkCommandBuffer)> cmds_recorder);
            void transitionImageLayout(
                VkImage image,
                VkFormat format,
                VkImageLayout old_layout,
                VkImageLayout new_layout
            );
        };
    }
}
