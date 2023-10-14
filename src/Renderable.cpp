#include "kk_renderer/Renderable.h"
#include <cassert>

using namespace kk::renderer;

Renderable Renderable::create(RenderingContext& ctx, const Geometry& geometry_value) {
    Renderable renderable{};
    renderable.geometry = geometry_value;

    for (auto& desc : renderable.resources) {
        std::shared_ptr<Buffer> uniform = std::make_shared<Buffer>(
            Buffer::create(
                ctx,
                sizeof(glm::mat4),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            )
        );
        assert(vkMapMemory(ctx.device, uniform->memory, 0, uniform->size, 0, &uniform->mapped) == VK_SUCCESS);

        desc.bindBuffer(
            0,
            uniform,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            VK_SHADER_STAGE_VERTEX_BIT
        );
        desc.buildLayout(ctx);
        desc.buildSet(ctx);
    }

    return renderable;
}

void Renderable::destroy(RenderingContext& ctx) {
    vkDeviceWaitIdle(ctx.device);
    for (auto& res : resources) {
        res.destroy(ctx);
    }
}
