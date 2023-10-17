#include "kk_renderer/Renderable.h"
#include "kk_renderer/Mat4.h"
#include <cassert>

using namespace kk::renderer;

Renderable Renderable::create(
	RenderingContext& ctx,
	const std::shared_ptr<Geometry>& geometry,
	const std::shared_ptr<Material>& material
) {
	std::shared_ptr<Buffer> uniform(new Buffer(), [&ctx](Buffer* buf) { buf->destroy(ctx); delete buf; });
	*uniform = Buffer::create(
		ctx,
		sizeof(Mat4),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);
	vkMapMemory(ctx.device, uniform->memory, 0, VK_WHOLE_SIZE, 0, &uniform->mapped);

	Renderable renderable{};
	renderable.geometry = geometry;
	renderable.material = material;
	renderable.material->setBuffer(0, uniform, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);

	return renderable;
}
