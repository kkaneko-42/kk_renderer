#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace kk {
    namespace renderer {
        struct RenderingContext {
            VkInstance instance;

            static RenderingContext create();

        private:
            static VkInstance createInstance(
                const std::vector<const char*>& exts,
                const std::vector<const char*>& layers
            );
            static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
            static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
                VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                void* pUserData
            );
        };
    }
}
