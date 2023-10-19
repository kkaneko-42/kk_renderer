#pragma once

#include "RenderingContext.h"
#include "Vertex.h"
#include "Buffer.h"
#include <vector>
#include <string>

namespace kk {
    namespace renderer {
        struct Geometry {
            std::vector<Vertex> vertices;
            Buffer vertex_buffer;
            std::vector<uint32_t> indices;
            Buffer index_buffer;

            static Geometry create(RenderingContext& ctx, const std::string& path);

            static Geometry create(
                RenderingContext& ctx,
                const std::vector<Vertex>& vertices,
                const std::vector<uint32_t> indices
            );
            void destroy(RenderingContext& ctx);
        };
    }
}
