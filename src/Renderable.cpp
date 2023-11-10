#include "kk_renderer/Renderable.h"
#include "kk_renderer/Mat4.h"
#include <cassert>

using namespace kk::renderer;

Renderable::Renderable(
    const std::shared_ptr<Geometry>& geometry,
    const std::shared_ptr<Material>& material
) : geometry(geometry), material(material) {

}

bool Renderable::operator==(const Renderable& rhs) const {
    return (
        (geometry == rhs.geometry) &&
        (material == rhs.material) &&
        (transform == rhs.transform)
    );
}
