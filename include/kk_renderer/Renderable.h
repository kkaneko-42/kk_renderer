#pragma once

#include "RenderingContext.h"
#include "Transform.h"
#include "Geometry.h"
#include "ResourceDescriptor.h"
#include "Buffer.h"

namespace kk {
    namespace renderer {
        struct Renderable {
            static Renderable create(RenderingContext& ctx, const Geometry& geometry_value);
            void destroy(RenderingContext& ctx);

            Transform transform;
            Geometry geometry;

            std::array<ResourceDescriptor, kMaxConcurrentFrames> resources;
        };
    }
}
