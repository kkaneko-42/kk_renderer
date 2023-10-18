#include "kk_renderer/Renderable.h"
#include "kk_renderer/Mat4.h"
#include <cassert>

using namespace kk::renderer;

Renderable::Renderable(
    const std::shared_ptr<Geometry>& geometry,
    const std::shared_ptr<Material>& material
) : geometry(geometry), material(material) {
    static size_t next_id = 0;

    for (auto& desc_set : desc_sets) {
        for (auto& d : desc_set) {
            d = VK_NULL_HANDLE;
        }
    }

    // TODO: Reuse destructed id
    id = next_id++;
}
