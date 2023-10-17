#pragma once

#include <memory>
#include <functional>
#include <queue>
#include "RenderingContext.h"
#include "Geometry.h"
#include "Material.h"
#include "Buffer.h"

namespace kk {
    namespace renderer {
        struct Renderable {
            static Renderable create(
                RenderingContext& ctx,
                const std::shared_ptr<Geometry>& geometry,
                const std::shared_ptr<Material>& material
            );

            std::shared_ptr<Geometry> geometry;
            std::shared_ptr<Material> material;
        };
    }
}
