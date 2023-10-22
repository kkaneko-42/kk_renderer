#pragma once

#include "RenderingContext.h"
#include "Mat4.h"
#include "Texture.h"
#include <unordered_map>
#include <string>

namespace kk {
    namespace renderer {
        struct Shader {
            static Shader create(RenderingContext& ctx, const std::string& path);
            void destroy(RenderingContext& ctx);

            // Descriptor Set Index -> Layout Bindings
            std::unordered_map<size_t, std::vector<VkDescriptorSetLayoutBinding>> sets_bindings;
            VkShaderModule module;
        };
    }
}
