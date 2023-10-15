#pragma once

#include "RenderingContext.h"
#include "Transform.h"
#include "Geometry.h"
#include "ResourceDescriptor.h"
#include "Buffer.h"
#include "Texture.h"

namespace kk {
    namespace renderer {
        struct Renderable {
            static Renderable create(RenderingContext& ctx, const Geometry& geometry_value, std::shared_ptr<Texture> texture = nullptr);
            void destroy(RenderingContext& ctx);

            Transform transform;
            Geometry geometry;

            std::array<ResourceDescriptor, kMaxConcurrentFrames> resources;
        };
    }
}
