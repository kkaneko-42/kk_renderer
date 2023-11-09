#include "kk_renderer/Renderer.h"
#include "kk_renderer/PipelineBuilder.h"
#include <cassert>
#include <iostream>
#include <vector>
#include <cstring>
#include <glm/gtx/string_cast.hpp>
// FIXME
#ifndef TEST_RESOURCE_DIR
#define TEST_RESOURCE_DIR "./resources"
#endif

using namespace kk::renderer;

static VkRenderPass createRenderPass(RenderingContext& ctx, VkFormat swapchain_format);
static Texture createShadowMap(RenderingContext& ctx, VkExtent2D extent);
static Image createDepthImage(RenderingContext& ctx, VkExtent2D extent);

Renderer Renderer::create(RenderingContext& ctx) {
    Renderer renderer{};
    renderer.current_frame_ = renderer.img_idx_ = 0;
    
    // Allocate command buffer
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = ctx.cmd_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = static_cast<uint32_t>(kMaxConcurrentFrames);
    assert(vkAllocateCommandBuffers(ctx.device, &alloc_info, renderer.cmd_bufs_.data()) == VK_SUCCESS);

    // Create graphics pipelines and related objects
    renderer.shadow_map_ = createShadowMap(ctx, VkExtent2D{ 1024, 1024 });
    renderer.createDescriptors(ctx);
    renderer.createShadowResources(ctx);
    renderer.render_pass_ = createRenderPass(ctx, ctx.swapchain.format.format);
    renderer.depth_ = createDepthImage(ctx, ctx.swapchain.extent);
    renderer.createFramebuffers(ctx, ctx.swapchain);

    return renderer;
}

void Renderer::destroy(RenderingContext& ctx) {
    // Destroy shadow resources
    for (auto& kvp : shadow_global_uniforms_) {
        kvp.first.destroy(ctx);
    }
    vkDestroyFramebuffer(ctx.device, shadow_framebuffer_, nullptr);
    vkDestroyPipeline(ctx.device, shadow_pipeline_, nullptr);
    vkDestroyPipelineLayout(ctx.device, shadow_layout_, nullptr);
    vkDestroyRenderPass(ctx.device, shadow_pass_, nullptr);
    shadow_map_.destroy(ctx);

    for (auto& kvp : object_uniforms_) {
        for (size_t i = 0; i < kMaxConcurrentFrames; ++i) {
            kvp.second[i].first.destroy(ctx);
        }
    }
    vkDestroyDescriptorSetLayout(ctx.device, object_uniform_layout_, nullptr);

    for (auto& kvp : global_uniforms_) {
        kvp.first.destroy(ctx);
    }
    vkDestroyDescriptorSetLayout(ctx.device, global_uniform_layout_, nullptr);

    depth_.destroy(ctx);

    for (auto& framebuffer : framebuffers_) {
        vkDestroyFramebuffer(ctx.device, framebuffer, nullptr);
    }
    
    vkDestroyRenderPass(ctx.device, render_pass_, nullptr);

    vkFreeCommandBuffers(
        ctx.device,
        ctx.cmd_pool,
        static_cast<uint32_t>(cmd_bufs_.size()),
        cmd_bufs_.data()
    );
}

bool Renderer::beginFrame(RenderingContext& ctx) {
    VkResult ret = vkWaitForFences(ctx.device, 1, &ctx.fences[current_frame_], VK_TRUE, UINT64_MAX);
    if (ret != VK_SUCCESS) {
        return false;
    }

    ret = vkAcquireNextImageKHR(ctx.device, ctx.swapchain.swapchain, UINT64_MAX, ctx.present_complete[current_frame_], VK_NULL_HANDLE, &img_idx_);
    if (ret != VK_SUCCESS) {
        if (ret == VK_ERROR_OUT_OF_DATE_KHR) {
            // TODO: recreate swapchain
        }
        return false;
    }

    ret = vkResetFences(ctx.device, 1, &ctx.fences[current_frame_]);
    if (ret != VK_SUCCESS) {
        // NOTE: ret == VK_ERROR_OUT_OF_DEVICE_MEMORY
        return false;
    }

    VkCommandBuffer current_buf = cmd_bufs_[current_frame_];
    ret = vkResetCommandBuffer(current_buf, 0);
    if (ret != VK_SUCCESS) {
        // NOTE: ret == VK_ERROR_OUT_OF_DEVICE_MEMORY
        return false;
    }

    // Begin comamnd buffer
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    assert(vkBeginCommandBuffer(current_buf, &begin_info) == VK_SUCCESS);

    return true;
}

void Renderer::endFrame(RenderingContext& ctx) {
    assert(vkEndCommandBuffer(cmd_bufs_[current_frame_]) == VK_SUCCESS);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &ctx.present_complete[current_frame_];
    VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd_bufs_[current_frame_];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &ctx.render_complete[current_frame_];

    VkResult ret = vkQueueSubmit(ctx.graphics_queue, 1, &submit_info, ctx.fences[current_frame_]);
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to graphics submit. Idx: " << current_frame_ << std::endl;
        return;
    }

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &ctx.render_complete[current_frame_];
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &ctx.swapchain.swapchain;
    present_info.pImageIndices = &img_idx_; // CONCERN

    ret = vkQueuePresentKHR(ctx.present_queue, &present_info);
    if (ret == VK_ERROR_OUT_OF_DATE_KHR || ret == VK_SUBOPTIMAL_KHR) {
        // TODO: Recreate swapchain
    }
    else if (ret != VK_SUCCESS) {
        std::cerr << "Failed to present submit. Idx: " << current_frame_ << std::endl;
    }

    current_frame_ = (current_frame_ + 1) % kMaxConcurrentFrames;
}

void Renderer::prepareRendering(RenderingContext& ctx, Renderable& renderable) {
    // Set pipeline state
    if (!renderable.material->isCompiled()) {
        renderable.material->compile(ctx, render_pass_);
    }

    // Setup uniform buffers
    if (object_uniforms_.find(renderable.id) == object_uniforms_.end()) {
        for (size_t i = 0; i < kMaxConcurrentFrames; ++i) {
            auto& uniform = object_uniforms_[renderable.id][i].first;
            auto& desc = object_uniforms_[renderable.id][i].second;

            uniform = Buffer::create(
                ctx,
                sizeof(ObjectUniform),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );
            vkMapMemory(ctx.device, uniform.memory, 0, uniform.size, 0, &uniform.mapped);

            VkDescriptorSetAllocateInfo alloc_info{};
            alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            alloc_info.descriptorPool = ctx.desc_pool;
            alloc_info.descriptorSetCount = 1;
            alloc_info.pSetLayouts = &object_uniform_layout_;
            assert(vkAllocateDescriptorSets(ctx.device, &alloc_info, &desc) == VK_SUCCESS);

            // Update descriptor
            VkWriteDescriptorSet write_buf{};
            write_buf.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_buf.dstSet = desc;
            write_buf.dstBinding = 0;
            write_buf.descriptorCount = 1;
            write_buf.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

            VkDescriptorBufferInfo buf_info{};
            buf_info.buffer = uniform.buffer;
            buf_info.offset = 0;
            buf_info.range = uniform.size;
            write_buf.pBufferInfo = &buf_info;

            vkUpdateDescriptorSets(ctx.device, 1, &write_buf, 0, nullptr);
        }
    }
}

void Renderer::render(RenderingContext& ctx, Renderable& renderable, const Transform& transform) {
    prepareRendering(ctx, renderable);

    VkCommandBuffer cmd_buf = cmd_bufs_[current_frame_];
    Material& material = *renderable.material;
    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, material.getPipeline());

    // Build object uniform
    ObjectUniform uniform{};
    uniform.model_to_world = 
        glm::translate(glm::mat4(1.0f), transform.position) *
        glm::mat4_cast(transform.rotation) *
        glm::scale(glm::mat4(1.0f), transform.scale)
        ;
    uniform.world_to_model = glm::inverse(uniform.model_to_world);

    // Copy object uniform
    auto& dst_buf = object_uniforms_[renderable.id][current_frame_].first;
    std::memcpy(dst_buf.mapped, &uniform, dst_buf.size);

    // Set descriptor
    if (!is_camera_binded_) {
        vkCmdBindDescriptorSets(
            cmd_buf,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            material.getPipelineLayout(),
            0,
            1,
            &global_uniforms_[current_frame_].second,
            0,
            nullptr
        );
        is_camera_binded_ = true;
    }

    VkDescriptorSet desc_sets[] = { renderable.material->getDescriptorSet(), object_uniforms_[renderable.id][current_frame_].second };
    vkCmdBindDescriptorSets(
        cmd_buf,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        material.getPipelineLayout(),
        1, // first set
        2, // desc sets count
        desc_sets, // desc sets
        0,
        nullptr
    );

    // Set geometry
    Geometry& geometry = *renderable.geometry;
    const VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd_buf, 0, 1, &geometry.vertex_buffer.buffer, offsets);
    vkCmdBindIndexBuffer(cmd_buf, geometry.index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    // Draw
    vkCmdDrawIndexed(cmd_buf, static_cast<uint32_t>(geometry.indices.size()), 1, 0, 0, 0);
}

void Renderer::renderShadowMap(RenderingContext& ctx, std::vector<Renderable>& scene, const DirectionalLight& light) {
    VkCommandBuffer cmd_buf = cmd_bufs_[current_frame_];
    const VkExtent2D shadow_map_extent{ shadow_map_.width, shadow_map_.height };

    VkRenderPassBeginInfo pass_info{};
    pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    pass_info.renderPass = shadow_pass_;
    pass_info.framebuffer = shadow_framebuffer_;
    pass_info.renderArea.extent = shadow_map_extent;

    std::array<VkClearValue, 1> clear_values{};
    clear_values[0].depthStencil = { 1.0f, 0 };
    pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
    pass_info.pClearValues = clear_values.data();

    vkCmdBeginRenderPass(cmd_buf, &pass_info, VK_SUBPASS_CONTENTS_INLINE);

    // Set viewport
    VkViewport viewport{};
    viewport.width = static_cast<float>(shadow_map_extent.width);
    viewport.height = static_cast<float>(shadow_map_extent.height);
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

    // Set scissor
    VkRect2D scissor{};
    scissor.extent = shadow_map_extent;
    vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

    // Setup global uniform
    GlobalUniform global_uniform{};
    global_uniform.view = glm::lookAt(
        light.transform.position,
        light.transform.position + light.transform.getForward(),
        Vec3(0, -1, 0)
    );
    // global_uniform.proj = glm::ortho<float>(-10, 10, -10, 10, -10, 20); // TODO: Set from outside
    global_uniform.proj = glm::ortho<float>(-10, 10, -10, 10, -10, 20); // TODO: Set from outside
    global_uniform.proj[1][1] *= -1;
    auto& current_global = shadow_global_uniforms_[current_frame_].first;
    std::memcpy(current_global.mapped, &global_uniform, current_global.size);

    vkCmdBindDescriptorSets(
        cmd_buf,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        shadow_layout_,
        0,
        1,
        &shadow_global_uniforms_[current_frame_].second,
        0,
        nullptr
    );

    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, shadow_pipeline_);

    // Render objects
    for (auto& renderable : scene) {
        prepareRendering(ctx, renderable);

        // Setup object uniform
        ObjectUniform uniform{};
        uniform.model_to_world =
            glm::translate(glm::mat4(1.0f), renderable.transform.position) *
            glm::mat4_cast(renderable.transform.rotation) *
            glm::scale(glm::mat4(1.0f), renderable.transform.scale)
            ;
        uniform.world_to_model = glm::inverse(uniform.model_to_world);
        auto& current_object = object_uniforms_[renderable.id][current_frame_].first;
        std::memcpy(current_object.mapped, &uniform, current_object.size);
        vkCmdBindDescriptorSets(
            cmd_buf,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            shadow_layout_,
            1,
            1,
            &object_uniforms_[renderable.id][current_frame_].second,
            0,
            nullptr
        );

        Geometry& geometry = *renderable.geometry;
        const VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd_buf, 0, 1, &geometry.vertex_buffer.buffer, offsets);
        vkCmdBindIndexBuffer(cmd_buf, geometry.index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(cmd_buf, static_cast<uint32_t>(geometry.indices.size()), 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(cmd_buf);
}

void Renderer::renderColor(RenderingContext& ctx, std::vector<Renderable>& scene, const DirectionalLight& light, const Camera& camera, Swapchain& swapchain) {
    VkCommandBuffer cmd_buf = cmd_bufs_[current_frame_];

    // Begin render pass
    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = render_pass_;
    render_pass_info.framebuffer = framebuffers_[img_idx_];
    render_pass_info.renderArea.extent = swapchain.extent;

    std::array<VkClearValue, 2> clear_values{};
    clear_values[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clear_values[1].depthStencil = { 1.0f, 0 };
    render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
    render_pass_info.pClearValues = clear_values.data();

    vkCmdBeginRenderPass(cmd_buf, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    // Set viewport
    VkViewport viewport{};
    viewport.width = static_cast<float>(swapchain.extent.width);
    viewport.height = static_cast<float>(swapchain.extent.height);
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

    // Set scissor
    VkRect2D scissor{};
    scissor.extent = swapchain.extent;
    vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

    // Set global uniform
    GlobalUniform global{};
    global.view = glm::lookAt(
        camera.transform.position,
        camera.transform.position + camera.transform.getForward(),
        Vec3(0, -1, 0)
    );
    global.proj = camera.getProjection();
    global.light_view = glm::lookAt(
        light.transform.position,
        light.transform.position + light.transform.getForward(),
        Vec3(0, -1, 0)
    );
    global.light_proj = glm::ortho<float>(-10, 10, -10, 10, -10, 20);
    global.light_proj[1][1] *= -1;
    global.light_pos = light.transform.position;
    global.light_dir = light.transform.getForward();
    global.light_color = light.color;
    global.light_intensity = light.intensity;
    auto& current_global = global_uniforms_[current_frame_].first;
    std::memcpy(current_global.mapped, &global, current_global.size);

    for (auto& renderable : scene) {
        prepareRendering(ctx, renderable);

        Material& material = *renderable.material;
        vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, material.getPipeline());

        ObjectUniform uniform{};
        uniform.model_to_world =
            glm::translate(glm::mat4(1.0f), renderable.transform.position) *
            glm::mat4_cast(renderable.transform.rotation) *
            glm::scale(glm::mat4(1.0f), renderable.transform.scale)
            ;
        uniform.world_to_model = glm::inverse(uniform.model_to_world);
        auto& current_object = object_uniforms_[renderable.id][current_frame_].first;
        std::memcpy(current_object.mapped, &uniform, current_object.size);

        VkDescriptorSet desc_sets[] = { 
            global_uniforms_[current_frame_].second, // FIXME: This should be bound once
            renderable.material->getDescriptorSet(),
            object_uniforms_[renderable.id][current_frame_].second
        };
        vkCmdBindDescriptorSets(
            cmd_buf,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            material.getPipelineLayout(),
            0, // first set
            3, // desc sets count
            desc_sets, // desc sets
            0,
            nullptr
        );

        Geometry& geometry = *renderable.geometry;
        const VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd_buf, 0, 1, &geometry.vertex_buffer.buffer, offsets);
        vkCmdBindIndexBuffer(cmd_buf, geometry.index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(cmd_buf, static_cast<uint32_t>(geometry.indices.size()), 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(cmd_buf);
}

// TODO: Frame beginning
void Renderer::render(RenderingContext& ctx, std::vector<Renderable>& scene, const DirectionalLight& light, const Camera& camera) {
    renderShadowMap(ctx, scene, light);
    renderColor(ctx, scene, light, camera, ctx.swapchain);
}

void Renderer::compileMaterial(RenderingContext& ctx, const std::shared_ptr<Material>& material) {
    material->compile(ctx, render_pass_);
}

static VkRenderPass createRenderPass(RenderingContext& ctx, VkFormat swapchain_format /* TODO: remove swapchain_format */) {
    VkAttachmentDescription color{};
    color.format = swapchain_format;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depth{};
    depth.format = VK_FORMAT_D32_SFLOAT; // TODO: Query format support
    depth.samples = VK_SAMPLE_COUNT_1_BIT;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    std::array<VkAttachmentDescription, 2> attachments = { color, depth };
    VkAttachmentReference color_ref{};
    color_ref.attachment = 0;
    color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_ref{};
    depth_ref.attachment = 1;
    depth_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    std::array<VkSubpassDescription, 1> subpass{};
    subpass[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass[0].colorAttachmentCount = 1;
    subpass[0].pColorAttachments = &color_ref;
    subpass[0].pDepthStencilAttachment = &depth_ref;

    std::array<VkSubpassDependency, 1> deps{};
    deps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    deps[0].dstSubpass = 0;
    deps[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    deps[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    deps[0].srcAccessMask = 0;
    deps[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = static_cast<uint32_t>(attachments.size());
    info.pAttachments = attachments.data();
    info.subpassCount = static_cast<uint32_t>(subpass.size());
    info.pSubpasses = subpass.data();
    info.dependencyCount = static_cast<uint32_t>(deps.size());
    info.pDependencies = deps.data();

    VkRenderPass render_pass;
    assert(vkCreateRenderPass(ctx.device, &info, nullptr, &render_pass) == VK_SUCCESS);

    return render_pass;
}

void Renderer::createFramebuffers(RenderingContext& ctx, const Swapchain& swapchain) {
    framebuffers_.resize(swapchain.images.size());
    for (size_t i = 0; i < swapchain.images.size(); i++) {
        std::array<VkImageView, 2> attachments = {
            swapchain.views[i],
            depth_.view,
        };

        VkFramebufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = render_pass_;
        info.attachmentCount = static_cast<uint32_t>(attachments.size());
        info.pAttachments = attachments.data();
        info.width = swapchain.extent.width;
        info.height = swapchain.extent.height;
        info.layers = 1;

        assert(vkCreateFramebuffer(ctx.device, &info, nullptr, &framebuffers_[i]) == VK_SUCCESS);
    }
}

static Image createDepthImage(RenderingContext& ctx, VkExtent2D extent) {
    // TODO: Query format support
    // VkFormat format = VK_FORMAT_D32_SFLOAT;
    VkFormat format;
    if (!ctx.findFormat(
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
            &format
    )) {
        assert(false && "Depth format not found");
    }

    Image depth = Image::create(
        ctx,
        extent.width,
        extent.height,
        format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT
    );

    return depth;
}

void Renderer::createDescriptors(RenderingContext& ctx) {
    // Create descriptor set layouts
    std::array<VkDescriptorSetLayoutBinding, 2> globals{};
    // Uniform(camera view, proj, light dir, etc)
    globals[0].binding = 0;
    globals[0].descriptorCount = 1;
    globals[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    globals[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    // Shadow map
    globals[1].binding = 1;
    globals[1].descriptorCount = 1;
    globals[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    globals[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo global_info{};
    global_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    global_info.bindingCount = static_cast<uint32_t>(globals.size());
    global_info.pBindings = globals.data();
    assert(vkCreateDescriptorSetLayout(ctx.device, &global_info, nullptr, &global_uniform_layout_) == VK_SUCCESS);

    VkDescriptorSetLayoutBinding object{};
    object.binding = 0;
    object.descriptorCount = 1;
    object.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    object.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo object_info{};
    object_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    object_info.bindingCount = 1;
    object_info.pBindings = &object;
    assert(vkCreateDescriptorSetLayout(ctx.device, &object_info, nullptr, &object_uniform_layout_) == VK_SUCCESS);

    // Create descriptor sets
    for (size_t i = 0; i < kMaxConcurrentFrames; ++i) {
        auto& color_buf = global_uniforms_[i].first;
        auto& color_desc = global_uniforms_[i].second;
        auto& shadow_buf = shadow_global_uniforms_[i].first;
        auto& shadow_desc = shadow_global_uniforms_[i].second;

        // Allocate
        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = ctx.desc_pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &global_uniform_layout_;
        assert(vkAllocateDescriptorSets(ctx.device, &alloc_info, &color_desc) == VK_SUCCESS);
        assert(vkAllocateDescriptorSets(ctx.device, &alloc_info, &shadow_desc) == VK_SUCCESS);

        // Create buffer resource
        color_buf = Buffer::create(
            ctx,
            sizeof(GlobalUniform),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        vkMapMemory(ctx.device, color_buf.memory, 0, color_buf.size, 0, &color_buf.mapped);
        shadow_buf = Buffer::create(
            ctx,
            sizeof(GlobalUniform),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        vkMapMemory(ctx.device, shadow_buf.memory, 0, shadow_buf.size, 0, &shadow_buf.mapped);

        // Color pass uniform
        VkWriteDescriptorSet write_color_buf{};
        write_color_buf.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_color_buf.dstSet = color_desc;
        write_color_buf.dstBinding = 0;
        write_color_buf.descriptorCount = 1;
        write_color_buf.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

        VkDescriptorBufferInfo color_buf_info{};
        color_buf_info.buffer = color_buf.buffer;
        color_buf_info.offset = 0;
        color_buf_info.range = color_buf.size;
        write_color_buf.pBufferInfo = &color_buf_info;

        // Color pass shadow map
        VkWriteDescriptorSet write_shadow_map = write_color_buf;
        write_shadow_map.dstBinding = 1;
        write_shadow_map.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

        VkDescriptorImageInfo shadow_map_info{};
        shadow_map_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        shadow_map_info.imageView = shadow_map_.view;
        shadow_map_info.sampler = shadow_map_.sampler;
        write_shadow_map.pImageInfo = &shadow_map_info;

        // Shadow pass uniform
        VkWriteDescriptorSet write_shadow_buf = write_color_buf;
        write_shadow_buf.dstSet = shadow_desc;
        
        VkDescriptorBufferInfo shadow_buf_info{};
        shadow_buf_info.buffer = shadow_buf.buffer;
        shadow_buf_info.offset = 0;
        shadow_buf_info.range = shadow_buf.size;
        write_shadow_buf.pBufferInfo = &shadow_buf_info;

        VkWriteDescriptorSet writes[] = { write_color_buf, write_shadow_map, write_shadow_buf };
        vkUpdateDescriptorSets(ctx.device, 3, writes, 0, nullptr);
    }
}

static Texture createShadowMap(RenderingContext& ctx, VkExtent2D extent) {
    VkFormat format;
    if (!ctx.findFormat(
        {VK_FORMAT_D16_UNORM, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
        &format
    )) {
        assert(false && "Shadow map format not found");
    }

    return Texture::create(
        ctx,
        nullptr,
        2,
        extent.width,
        extent.height,
        format, // TODO: query format
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT
    );
}

void Renderer::createShadowResources(RenderingContext& ctx) {
    // Create render pass
    VkAttachmentDescription shadow{};
    // shadow.format = VK_FORMAT_D16_UNORM; // TODO: query format
    shadow.format = shadow_map_.format;
    shadow.samples = VK_SAMPLE_COUNT_1_BIT;
    shadow.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    shadow.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    shadow.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    shadow.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    shadow.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    shadow.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    std::array<VkAttachmentDescription, 1> attachments = { shadow };
    VkAttachmentReference shadow_output{};
    shadow_output.attachment = 0;
    shadow_output.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    std::array<VkSubpassDescription, 1> subpass{};
    subpass[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass[0].colorAttachmentCount = 0;
    subpass[0].pColorAttachments = nullptr;
    subpass[0].pDepthStencilAttachment = &shadow_output;

    std::array<VkSubpassDependency, 2> deps{};
    deps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    deps[0].dstSubpass = 0;
    deps[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    deps[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    deps[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    deps[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    deps[1].srcSubpass = VK_SUBPASS_EXTERNAL;
    deps[1].dstSubpass = 0;
    deps[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    deps[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    deps[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    deps[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    render_pass_info.pAttachments = attachments.data();
    render_pass_info.subpassCount = static_cast<uint32_t>(subpass.size());
    render_pass_info.pSubpasses = subpass.data();
    render_pass_info.dependencyCount = static_cast<uint32_t>(deps.size());
    render_pass_info.pDependencies = deps.data();

    assert(vkCreateRenderPass(ctx.device, &render_pass_info, nullptr, &shadow_pass_) == VK_SUCCESS);

    // Create pieline layout
    VkDescriptorSetLayout set_layouts[] = { global_uniform_layout_, object_uniform_layout_ };
    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 2;
    pipeline_layout_info.pSetLayouts = set_layouts;
    assert(vkCreatePipelineLayout(ctx.device, &pipeline_layout_info, nullptr, &shadow_layout_) == VK_SUCCESS);

    // Create shader modules
    auto vert = std::make_shared<Shader>(Shader::create(ctx, TEST_RESOURCE_DIR + std::string("shaders/shadow.vert.spv")));
    auto frag = std::make_shared<Shader>(Shader::create(ctx, TEST_RESOURCE_DIR + std::string("shaders/shadow.frag.spv")));

    // Create pipeline
    PipelineBuilder builder;
    shadow_pipeline_ = builder
        .setDefault()
        .setVertexShader(vert)
        .setFragmentShader(frag)
        .setFrontFace(VK_FRONT_FACE_CLOCKWISE)
        .build(ctx, 0, shadow_layout_, shadow_pass_)
    ;
    vert->destroy(ctx);
    frag->destroy(ctx);

    // Create frame buffer
    VkFramebufferCreateInfo framebuf_info{};
    framebuf_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuf_info.attachmentCount = 1;
    framebuf_info.pAttachments = &shadow_map_.view;
    framebuf_info.renderPass = shadow_pass_;
    framebuf_info.width = shadow_map_.width;
    framebuf_info.height = shadow_map_.height;
    framebuf_info.layers = 1;
    assert(vkCreateFramebuffer(ctx.device, &framebuf_info, nullptr, &shadow_framebuffer_) == VK_SUCCESS);
}
