#pragma once

#include "Renderable.h"
#include "DirectionalLight.h"

namespace kk {
    namespace renderer {
        class Scene {
        public:
            void addObject(const Renderable& renderable);
            bool removeObject(const Renderable& renderable);

            void addLight(const DirectionalLight& light);
            bool removeLight(const DirectionalLight& light);

            inline void setSkybox(const std::shared_ptr<Texture>& skybox) {
                skybox_ = skybox;
            }

            inline constexpr const std::vector<std::pair<Buffer, VkDescriptorSet>>& getUniforms(size_t frame) const {
                return uniforms_[frame];
            }

            inline constexpr const std::vector<Renderable>& getObjects() const {
                return objects_;
            }

        private:
            std::vector<Renderable> objects_;
            std::array<std::vector<std::pair<Buffer, VkDescriptorSet>>, kMaxConcurrentFrames> uniforms_;
            std::vector<DirectionalLight> dir_lights_;
            std::shared_ptr<Texture> skybox_;
        };
    }
}
