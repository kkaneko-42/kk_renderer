#include "kk_renderer/Shader.h"
#include <cassert>
#include <iostream>
#include <fstream>

using namespace kk::renderer;

static std::vector<char> readFile(const std::string& path);

Shader Shader::create(RenderingContext& ctx, const std::string& path) {
    const std::vector<char> code = readFile(path);

    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = code.size();
    info.pCode = reinterpret_cast<const uint32_t*>(code.data());

    Shader shader;
    assert(vkCreateShaderModule(ctx.device, &info, nullptr, &shader.module) == VK_SUCCESS);
    
    // TODO: Get from shader reflection
    if (path.find("triangle.vert.spv") != std::string::npos) {
        shader.sets_bindings[0].resize(1); // How many bindings does descriptor set 0 have ?
        // What kind of resource does binding 0 require ?
        shader.sets_bindings[0][0].binding = 0;
        shader.sets_bindings[0][0].descriptorCount = 1;
        shader.sets_bindings[0][0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        shader.sets_bindings[0][0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    }
    else if (path.find("triangle.frag.spv") != std::string::npos) {
        // Do nothing
    }
    else if (path.find("texture.vert.spv") != std::string::npos) {
        shader.sets_bindings[1].resize(1);
        shader.sets_bindings[1][0].binding = 0;
        shader.sets_bindings[1][0].descriptorCount = 1;
        shader.sets_bindings[1][0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        shader.sets_bindings[1][0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    }
    else if (path.find("texture.frag.spv") != std::string::npos) {
        shader.sets_bindings[0].resize(1);
        shader.sets_bindings[0][0].binding = 0;
        shader.sets_bindings[0][0].descriptorCount = 1;
        shader.sets_bindings[0][0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        shader.sets_bindings[0][0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    else {
        std::cerr << "Shader::create(): Error: Failed to get reflection of " << path << std::endl;
        assert(false);
    }
    
    return shader;
}

void Shader::destroy(RenderingContext& ctx) {
    vkDestroyShaderModule(ctx.device, module, nullptr);
}

static std::vector<char> readFile(const std::string& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "Failed to open " << path << std::endl;
        assert(false);
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

