#pragma once

#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"
#include <vulkan/vulkan.h>
#include <array>

namespace kk {
    namespace renderer {
        struct Vertex {
            Vec3 position;
            Vec2 uv;
            Vec4 color;

            static VkVertexInputBindingDescription getBindingDescription();
            static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
        };

        bool operator==(const Vertex& lhs, const Vertex& rhs);
    }
}
