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

            Transform() : position(), scale({ 1, 1, 1 }) {
                rotation.w = 1;
                rotation.x = rotation.y = rotation.z = 0;
            }

            inline Vec3 getForward() const {
                return rotation * Vec3(0, 0, 1);
            }

            inline Vec3 getUp() const {
                return rotation * Vec3(0, -1, 0);
            }

            inline Vec3 getRight() const {
                return rotation * Vec3(1, 0, 0);
            }

            inline bool operator==(const Transform& rhs) const {
                return (
                    position == rhs.position &&
                    rotation == rhs.rotation &&
                    scale == rhs.scale
                );
            }
        };
    }
}
