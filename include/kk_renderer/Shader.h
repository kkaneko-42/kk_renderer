#pragma once

#include "RenderingContext.h"
#include "Mat4.h"
#include "Texture.h"

#include <unordered_map>
#include <string>

namespace kk {
    namespace renderer {
        class Shader {
        public:
            static Shader create(RenderingContext& ctx, const std::string& path);
            void destroy(RenderingContext& ctx);

            inline VkShaderModule get() const { return module_; }
            inline constexpr const std::vector<VkDescriptorSetLayoutBinding>& getResourceLayout() const {
                return bindings_;
            }

        private:
            void acquireBindings(std::vector<uint32_t>&& code);

            std::vector<VkDescriptorSetLayoutBinding> bindings_;
            VkShaderModule module_;
        };
    }
}
