#include "kk_renderer/Shader.h"
#include <spirv_cross/spirv_glsl.hpp>
#include <cassert>
#include <iostream>
#include <fstream>
#include <cstring>

using namespace kk::renderer;

static std::vector<uint32_t> readBinary(const std::string& path);

Shader Shader::create(RenderingContext& ctx, const std::string& path) {
    std::vector<uint32_t> code = readBinary(path);

    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = code.size();
    info.pCode = code.data();

    Shader shader;
    assert(vkCreateShaderModule(ctx.device, &info, nullptr, &shader.module_) == VK_SUCCESS);

    shader.acquireBindings(std::move(code));

    return shader;
}

void Shader::destroy(RenderingContext& ctx) {
    vkDestroyShaderModule(ctx.device, module_, nullptr);
}

void Shader::acquireBindings(std::vector<uint32_t>&& code) {
    spirv_cross::CompilerGLSL glsl(std::move(code));
    const spirv_cross::ShaderResources resources = glsl.get_shader_resources();
    const std::unordered_map<
        VkDescriptorType,
        const spirv_cross::SmallVector<spirv_cross::Resource>&
    >   desctype_resources = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, resources.uniform_buffers},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, resources.sampled_images}
    };

    // Get shader stage
    VkShaderStageFlags stage;
    switch (glsl.get_execution_model()) {
    case spv::ExecutionModelVertex:
        stage = VK_SHADER_STAGE_VERTEX_BIT; break;
    case spv::ExecutionModelFragment:
        stage = VK_SHADER_STAGE_FRAGMENT_BIT; break;
    default:
        assert(false && "Not supported execution model"); break;
    }

    // Get layout bindings
    for (const auto& kvp : desctype_resources) {
        for (const auto& resource : kvp.second) {
            uint32_t set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
            if (set != 1) {
                // NOTE: Material only use set 1
                continue;
            }

            VkDescriptorSetLayoutBinding binding{};
            binding.binding = glsl.get_decoration(resource.id, spv::DecorationBinding);
            binding.descriptorCount = 1;
            binding.descriptorType = kvp.first;
            binding.stageFlags = stage;
            bindings_.push_back(binding);
        }
    }
}

static std::vector<uint32_t> readBinary(const std::string& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "Failed to open " << path << std::endl;
        assert(false);
    }

    size_t size = (size_t)file.tellg();
    std::vector<char> buffer(size);

    file.seekg(0);
    file.read(buffer.data(), size);

    file.close();

    std::vector<uint32_t> result(size);
    std::memcpy(result.data(), buffer.data(), size);

    return result;
}
