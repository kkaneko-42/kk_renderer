#pragma once

#include <memory>
#include <functional>
#include <queue>
#include "RenderingContext.h"
#include "Geometry.h"
#include "Material.h"
#include "Buffer.h"
#include "Transform.h"

namespace kk {
    namespace renderer {
        struct Renderable {
            Renderable() = default;
            Renderable(
                const std::shared_ptr<Geometry>& geometry,
                const std::shared_ptr<Material>& material
            );

            Transform transform;
            std::shared_ptr<Geometry> geometry;
            std::shared_ptr<Material> material;

            bool operator==(const Renderable& rhs) const;
        };
    }
}
