#pragma once

#include <memory>
#include "Geometry.h"
#include "Material.h"

namespace kk {
    namespace renderer {
        struct Renderable {
            std::shared_ptr<Geometry> geometry;
            std::shared_ptr<Material> material;
        };
    }
}
