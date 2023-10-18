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
            Renderable(
                const std::shared_ptr<Geometry>& geometry,
                const std::shared_ptr<Material>& material
            );

            size_t id;
            std::shared_ptr<Geometry> geometry;
            std::shared_ptr<Material> material;
            std::array<std::vector<VkDescriptorSet>, kMaxConcurrentFrames> desc_sets;
        };
    }
}
