#pragma once

#include "Vec3.h"
#include "Vec4.h"
#include <vulkan/vulkan.h>
#include <array>

namespace kk {
    namespace renderer {
        struct Vertex {
            Vec3 position;
            Vec4 color;

            static VkVertexInputBindingDescription getBindingDescription();
            static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
        };
    }
}
