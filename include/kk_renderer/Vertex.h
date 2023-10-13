#pragma once

#include <glm/glm.hpp>

namespace kk {
    // Math lib mock
    using Vec3 = glm::vec3;
    using Vec4 = glm::vec4;

    namespace renderer {
        struct Vertex {
            Vec3 position;
            Vec4 color;
        };
    }
}
