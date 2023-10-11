#pragma once

#include <vulkan/vulkan.h>
#include "Window.h"
#include "RenderingContext.h"

namespace kk {
    namespace renderer {
        struct SwapChain {
            static SwapChain create(RenderingContext& ctx, Window& window);
            void destroy(RenderingContext& ctx);

            VkSurfaceKHR surface_;
        };
    }
}
