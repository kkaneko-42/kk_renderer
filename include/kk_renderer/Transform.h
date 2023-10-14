#pragma once

#include "Vec3.h"
#include "Quat.h"

namespace kk {
    namespace renderer {
        struct Transform {
            Vec3 position;
            Quat rotation;
            Vec3 scale;

            Transform() : position(), rotation(), scale({ 1.0f, 1.0f, 1.0f }) {}
        };
    }
}
