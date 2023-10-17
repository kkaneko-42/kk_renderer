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
            Renderable(const std::shared_ptr<Geometry>& geometry, const std::shared_ptr<Material>& material);
            ~Renderable();

            // TODO: These shouldn't be placed here
            static void createUniformBuffers(RenderingContext& ctx, size_t max_objs);
            static void destroyUniformBuffers(RenderingContext& ctx);

            size_t id;
            std::shared_ptr<Geometry> geometry;
            std::shared_ptr<Material> material;

        private:
            // static std::array<std::unique_ptr<Buffer, std::function<void(Buffer*)>>, kMaxConcurrentFrames> uniforms_;
            static Buffer uniform_;
        };
    }
}
