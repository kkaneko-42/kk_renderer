#pragma once

#include "Vec3.h"

namespace kk {
    namespace renderer {
        struct DirectionalLight {
            DirectionalLight()
                : transform(), color(1.0f, 1.0f, 1.0f), intensity(1.0f)
            {}

            Transform transform;
            alignas(16) Vec3 color;
            float intensity;
        };
    }
}
