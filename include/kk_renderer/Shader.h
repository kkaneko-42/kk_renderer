#pragma once

#include "RenderingContext.h"
#include <unordered_map>
#include <string>

namespace kk {
    namespace renderer {
        struct Shader {
            static Shader create(RenderingContext& ctx, const std::string& path);
            void destroy(RenderingContext& ctx);

            std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;
            VkShaderModule module;
        };
    }
}
