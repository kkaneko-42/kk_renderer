#include "kk_renderer/Scene.h"
#include <algorithm>

using namespace kk::renderer;

static Buffer createUniformBuffer(RenderingContext& ctx);

Scene::Scene(RenderingContext& ctx) : ctx_(ctx) {

}

Scene Scene::create(RenderingContext& ctx) {
    Scene scene(ctx);

    // Create descriptor set layout
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorCount = 1;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = 1;
    info.pBindings = &binding;
    assert(vkCreateDescriptorSetLayout(ctx.device, &info, nullptr, &scene.desc_layout_) == VK_SUCCESS);

    // Create initial uniforms
    // FIXME: Magic number
    scene.expandUniforms(256);

    return scene;
}

void Scene::destroy() {
    for (auto& uniform : uniforms_) {
        for (auto& kvp : uniform) {
            kvp.first.destroy(ctx_);
            vkFreeDescriptorSets(ctx_.device, ctx_.desc_pool, 1, &kvp.second);
        }
    }
    vkDestroyDescriptorSetLayout(ctx_.device, desc_layout_, nullptr);
}

void Scene::addObject(const Renderable& renderable) {
    objects_.push_back(renderable);

    if (objects_.size() > uniforms_.size()) {
        const size_t expand_size = (uniforms_.size() == 0) ? 1 : uniforms_.size();
        expandUniforms(expand_size);
    }
}

bool Scene::removeObject(const Renderable& renderable) {
    auto it = std::find(objects_.begin(), objects_.end(), renderable);
    if (it == objects_.end()) {
        return false;
    }

    objects_.erase(it);
    return true;
}

/*
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
*/

void Scene::each(size_t frame, std::function<void(Renderable&, Buffer&, VkDescriptorSet)> f) {
    for (size_t i = 0; i < objects_.size(); ++i) {
        f(objects_[i], uniforms_[frame][i].first, uniforms_[frame][i].second);
    }
}

void Scene::expandUniforms(size_t expand_size) {
    for (size_t i = 0; i < expand_size; ++i) {
        std::array<std::pair<Buffer, VkDescriptorSet>, kMaxConcurrentFrames> uniform{};
        for (auto& kvp : uniform) {
            kvp.first = createUniformBuffer(ctx_);
            kvp.second = createUniformDescriptor(kvp.first);
        }
        uniforms_.push_back(uniform);
    }
}

VkDescriptorSet Scene::createUniformDescriptor(const Buffer& buffer) {
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = ctx_.desc_pool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &desc_layout_;

    VkDescriptorSet desc_set;
    assert(vkAllocateDescriptorSets(ctx_.device, &alloc_info, &desc_set) == VK_SUCCESS);

    VkWriteDescriptorSet desc_write{};
    desc_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    desc_write.dstSet = desc_set;
    desc_write.dstBinding = 0;
    desc_write.descriptorCount = 1;
    desc_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    VkDescriptorBufferInfo buf_info{};
    buf_info.buffer = buffer.buffer;
    buf_info.offset = 0;
    buf_info.range  = buffer.size;
    desc_write.pBufferInfo = &buf_info;

    vkUpdateDescriptorSets(ctx_.device, 1, &desc_write, 0, nullptr);
    return desc_set;
}

static Buffer createUniformBuffer(RenderingContext& ctx) {
    Buffer buffer = Buffer::create(
        ctx,
        sizeof(Scene::ObjectUniform),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    vkMapMemory(ctx.device, buffer.memory, 0, VK_WHOLE_SIZE, 0, &buffer.mapped);
    return buffer;
}
