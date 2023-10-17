#include "kk_renderer/Renderable.h"
#include "kk_renderer/Mat4.h"
#include <cassert>

using namespace kk::renderer;

Buffer Renderable::uniform_;

Renderable::Renderable(const std::shared_ptr<Geometry>& geometry, const std::shared_ptr<Material>& material)
    : geometry(geometry), material(material)
{
    static size_t next_id = 0;
    // TODO: Reused destructed id
    id = next_id++;

    material->setBuffer(
        0,
        std::make_shared<Buffer>(uniform_), // TODO
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        VK_SHADER_STAGE_VERTEX_BIT,
        id * sizeof(Mat4),
        sizeof(Mat4)
    );
}

Renderable::~Renderable() {

}

void Renderable::createUniformBuffers(RenderingContext& ctx, size_t max_objs) {
    uniform_ = Buffer::create(
        ctx,
        sizeof(Mat4) * max_objs,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    vkMapMemory(ctx.device, uniform_.memory, 0, VK_WHOLE_SIZE, 0, &uniform_.mapped);
}

void Renderable::destroyUniformBuffers(RenderingContext& ctx) {
    uniform_.destroy(ctx);
}
