#pragma once

#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"
#include <vulkan/vulkan.h>
#include <array>

namespace kk {
    namespace renderer {
        // FIXME: std::hash<Vertex> is implemented in Geometry.cpp
        struct Vertex {
            Vec3 position;
            Vec3 normal;
            Vec2 uv;
            Vec4 color;

            static VkVertexInputBindingDescription getBindingDescription();
            static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions();
        };

        bool operator==(const Vertex& lhs, const Vertex& rhs);
    }
}
