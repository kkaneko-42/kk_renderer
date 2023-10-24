#pragma once

#include "Vec3.h"

namespace kk {
    namespace renderer {
        struct DirectionalLight {
            DirectionalLight()
                : dir(0.0f, 0.0f, 1.0f), color(1.0f, 1.0f, 1.0f), intensity(1.0f)
            {}

            alignas(16) Vec3 pos;
            alignas(16) Vec3 dir;
            alignas(16) Vec3 color;
            float intensity;
        };
    }
}
