#include "kk_renderer/Geometry.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>
#include <functional>

using namespace kk;
using namespace kk::renderer;

template <class T>
static void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            size_t hash = 0;
            hash_combine(hash, vertex.position);
            hash_combine(hash, vertex.normal);
            hash_combine(hash, vertex.uv);
            hash_combine(hash, vertex.color);

            return hash;
        }
    };
}

static void loadModel(const std::string& path, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

Geometry Geometry::create(RenderingContext& ctx, const std::string& path) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    loadModel(path, vertices, indices);

    return Geometry::create(
        ctx,
        vertices,
        indices
    );
}

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

static void loadModel(const std::string& path, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
    tinyobj::attrib_t attr;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attr, &shapes, &materials, &warn, &err, path.c_str())) {
        throw std::runtime_error(err);
    }

    std::unordered_map<Vertex, uint32_t> unique_vertices{};

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.position = {
                attr.vertices[3 * index.vertex_index + 0],
                attr.vertices[3 * index.vertex_index + 1],
                attr.vertices[3 * index.vertex_index + 2]
            };

            vertex.normal = {
                attr.normals[3 * index.normal_index + 0],
                attr.normals[3 * index.normal_index + 1],
                attr.normals[3 * index.normal_index + 2]
            };

            vertex.uv = {
                attr.texcoords[2 * index.texcoord_index + 0],
                1.0f - attr.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.color = { 1.0f, 1.0f, 1.0f, 1.0f };

            if (unique_vertices.count(vertex) == 0) {
                unique_vertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(unique_vertices[vertex]);
        }
    }
}
