#include "kk_renderer/PerspectiveCamera.h"

using namespace kk::renderer;

PerspectiveCamera::PerspectiveCamera(float fov_deg, float aspect, float near, float far)
    : proj_(glm::perspective(glm::radians(fov_deg), aspect, near, far)) {
    proj_[1][1] *= -1;
}
