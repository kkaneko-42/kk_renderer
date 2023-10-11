#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace kk {
    namespace renderer {
        struct RenderingContext {
            VkInstance instance;
            VkPhysicalDevice gpu;

            static RenderingContext create();
        };
    }
}
