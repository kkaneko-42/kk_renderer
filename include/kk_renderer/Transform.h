#pragma once

#include "Vec3.h"
#include "Quat.h"
#include "Buffer.h"
#include <array>

namespace kk {
    namespace renderer {
        struct Transform {
            Vec3 position;
            Quat rotation;
            Vec3 scale;

            Transform() : position(), rotation({0, 0, 0, 1}), scale({ 1, 1, 1 }) {}

            inline Vec3 getForward() const {
                return rotation * Vec3(0, 0, 1);
            }

            inline Vec3 getUp() const {
                return rotation * Vec3(0, -1, 0);
            }
        };
    }
}
