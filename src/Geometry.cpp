#include "kk_renderer/Geometry.h"

using namespace kk::renderer;

Geometry Geometry::create(
    RenderingContext& ctx,
    const std::vector<Vertex>& vertices,
    const std::vector<uint32_t> indices
) {
    const size_t vertices_byte = sizeof(Vertex) * vertices.size();
    const size_t indices_byte = sizeof(uint32_t) * indices.size();

    Geometry geometry{};
    // NOTE: performance concern
    geometry.vertices = vertices;
    geometry.indices = indices;
    
    geometry.vertex_buffer = Buffer::create(
        ctx,
        vertices_byte,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    geometry.index_buffer = Buffer::create(
        ctx,
        indices_byte,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    geometry.vertex_buffer.setData(ctx, vertices.data(), vertices_byte);
    geometry.index_buffer.setData(ctx, indices.data(), indices_byte);

    return geometry;
}

void Geometry::destroy(RenderingContext& ctx) {
    vertex_buffer.destroy(ctx);
    index_buffer.destroy(ctx);
}
