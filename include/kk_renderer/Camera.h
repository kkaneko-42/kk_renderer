#pragma once

#include "Transform.h"
#include "Mat4.h"

namespace kk {
    namespace renderer {
        struct Camera {
            virtual ~Camera() {}

            Transform transform;
            virtual Mat4 getProjection() const = 0;
        };
    }
}
