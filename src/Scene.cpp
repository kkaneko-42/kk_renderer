#include "kk_renderer/Scene.h"

using namespace kk::renderer;

void Scene::addObject(const Renderable& renderable) {
    objects_.push_back(renderable);
}

bool Scene::removeObject(const Renderable& renderable) {
    auto it = std::find(objects_.begin(), objects_.end(), renderable);
    if (it == objects_.end()) {
        return false;
    }

    objects_.erase(it);
    return true;
}

void Scene::addLight(const DirectionalLight& light) {
    dir_lights_.push_back(light);
}

bool Scene::removeLight(const DirectionalLight& light) {
    auto it = std::find(dir_lights_.begin(), dir_lights_.end(), light);
    if (it == dir_lights_.end()) {
        return false;
    }

    dir_lights_.erase(it);
    return true;
}
