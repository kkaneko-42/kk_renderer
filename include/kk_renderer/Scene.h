#pragma once

#include "Renderable.h"
#include "DirectionalLight.h"

namespace kk {
    namespace renderer {
        class Scene {
        public:
            struct ObjectUniform {
                alignas(16) Mat4 model_to_world;
                alignas(16) Mat4 world_to_model;
            };

            static Scene create(RenderingContext& ctx);
            void destroy();

            void addObject(const Renderable& renderable);
            bool removeObject(const Renderable& renderable);

            // TODO: Support multiple lights
            inline void setLight(const DirectionalLight& light) {
                light_ = light;
            }
            inline constexpr const DirectionalLight& getLight() const {
                return light_;
            }
            // void addLight(const DirectionalLight& light);
            // bool removeLight(const DirectionalLight& light);

            inline void setSkybox(const std::shared_ptr<Texture>& skybox) {
                skybox_ = skybox;
            }

            inline VkDescriptorSetLayout getObjectUniformLayout() const {
                return desc_layout_;
            }

            void each(size_t frame, std::function<void(Renderable&, Buffer&, VkDescriptorSet)> f);

        private:
            std::vector<Renderable> objects_;
            std::vector<std::array<std::pair<Buffer, VkDescriptorSet>, kMaxConcurrentFrames>> uniforms_;
            DirectionalLight light_;
            std::shared_ptr<Texture> skybox_;

            RenderingContext& ctx_;
            VkDescriptorSetLayout desc_layout_;

            Scene(RenderingContext& ctx);
            VkDescriptorSet createUniformDescriptor(const Buffer& buffer);
            void expandUniforms(size_t expand_size);
        };
    }
}
