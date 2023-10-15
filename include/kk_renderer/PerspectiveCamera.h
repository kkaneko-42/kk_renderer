#pragma once

#include "Camera.h"

namespace kk {
    namespace renderer {
        class PerspectiveCamera : public Camera {
        public:
            PerspectiveCamera(float fov_deg, float aspect, float near, float far);

            inline Mat4 getProjection() const override { return proj_; };

        private:
            Mat4 proj_;
        };
    }
}
