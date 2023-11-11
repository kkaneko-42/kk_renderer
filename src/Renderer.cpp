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
    renderer.shadow_map_ = createShadowMap(ctx, VkExtent2D{1024, 1024});
    std::cout << "shadow material created..." << std::endl;
    renderer.createShadowMaterial(ctx);
    std::cout << "descriptors created..." << std::endl;
    renderer.createDescriptors(ctx);
    std::cout << "shadow resources created..." << std::endl;
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
    shadow_material_.destroy(ctx);
    vkDestroyRenderPass(ctx.device, shadow_pass_, nullptr);
    shadow_map_.destroy(ctx);

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

void Renderer::render(RenderingContext& ctx, Scene& scene, const Camera& camera) {
    beginShadowPass(ctx, scene, camera);
    scene.each(current_frame_, [&ctx, this](Renderable& renderable, Buffer& uniform_buf, VkDescriptorSet uniform_desc) {
        VkCommandBuffer cmd_buf = cmd_bufs_[current_frame_];

        // Update uniform buffer
        Scene::ObjectUniform uniform{};
        uniform.model_to_world = 
            glm::translate(glm::mat4(1.0f), renderable.transform.position) *
            glm::mat4_cast(renderable.transform.rotation) *
            glm::scale(glm::mat4(1.0f), renderable.transform.scale)
        ;
        uniform.world_to_model = glm::inverse(uniform.model_to_world);
        std::memcpy(uniform_buf.mapped, &uniform, sizeof(uniform));

        // Bind descriptor set
        vkCmdBindDescriptorSets(
            cmd_buf,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            shadow_material_.getPipelineLayout(),
            1, // first set
            1, // desc count
            &uniform_desc,
            0, nullptr // dynamic param
        );

        // Bind geometry
        Geometry& geometry = *renderable.geometry;
        const VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd_buf, 0, 1, &geometry.vertex_buffer.buffer, offsets);
        vkCmdBindIndexBuffer(cmd_buf, geometry.index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        // Draw
        vkCmdDrawIndexed(cmd_buf, static_cast<uint32_t>(geometry.indices.size()), 1, 0, 0, 0);
    });
    vkCmdEndRenderPass(cmd_bufs_[current_frame_]);
    beginColorPass(ctx, scene, camera);
    scene.each(current_frame_, [&ctx, &scene, this](Renderable& renderable, Buffer& uniform_buf, VkDescriptorSet uniform_desc) {
        VkCommandBuffer cmd_buf = cmd_bufs_[current_frame_];
        if (!renderable.material->isCompiled()) {
            renderable.material->compile(ctx, render_pass_, global_uniform_layout_, scene.getObjectUniformLayout());
        }
        vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, renderable.material->getPipeline());

        // Update uniform buffer
        Scene::ObjectUniform uniform{};
        uniform.model_to_world = 
            glm::translate(glm::mat4(1.0f), renderable.transform.position) *
            glm::mat4_cast(renderable.transform.rotation) *
            glm::scale(glm::mat4(1.0f), renderable.transform.scale)
        ;
        uniform.world_to_model = glm::inverse(uniform.model_to_world);
        std::memcpy(uniform_buf.mapped, &uniform, sizeof(uniform));

        VkDescriptorSet sets[] = { renderable.material->getDescriptorSet(), uniform_desc };
        vkCmdBindDescriptorSets(
            cmd_buf,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            renderable.material->getPipelineLayout(),
            1, 2, sets, // first set, set count, sets ptr
            0, nullptr
        );

        // Bind geometry
        Geometry& geometry = *renderable.geometry;
        const VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd_buf, 0, 1, &geometry.vertex_buffer.buffer, offsets);
        vkCmdBindIndexBuffer(cmd_buf, geometry.index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        // Draw
        vkCmdDrawIndexed(cmd_buf, static_cast<uint32_t>(geometry.indices.size()), 1, 0, 0, 0);
    });
    vkCmdEndRenderPass(cmd_bufs_[current_frame_]);
}

void Renderer::beginShadowPass(RenderingContext& ctx, Scene& scene, const Camera& camera) {
    if (!shadow_material_.isCompiled()) {
        shadow_material_.compile(ctx, shadow_pass_, global_uniform_layout_, scene.getObjectUniformLayout());
    }
    VkCommandBuffer cmd_buf = cmd_bufs_[current_frame_];

    // Begin render pass
    VkRenderPassBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    begin_info.renderPass = shadow_pass_;
    begin_info.framebuffer = shadow_framebuffer_;
    begin_info.renderArea.extent = VkExtent2D{shadow_map_.width, shadow_map_.height};
    VkClearValue clear_value{};
    clear_value.depthStencil = { 1.0f, 0 };
    begin_info.clearValueCount = 1;
    begin_info.pClearValues = &clear_value;
    vkCmdBeginRenderPass(cmd_buf, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

    // Set viewport
    VkViewport viewport{};
    viewport.width = static_cast<float>(shadow_map_.width);
    viewport.height = static_cast<float>(shadow_map_.height);
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

    // Set scissor
    VkRect2D scissor{};
    scissor.extent = VkExtent2D{shadow_map_.width, shadow_map_.height};
    vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

    // Setup global uniform
    const auto& light = scene.getLight();
    GlobalUniform global_uniform{};
    global_uniform.view = glm::lookAt(
        light.transform.position,
        light.transform.position + light.transform.getForward(),
        Vec3(0, -1, 0)
    );
    global_uniform.proj = glm::ortho<float>(-10, 10, -10, 10, -10, 20); // TODO: Set from outside
    global_uniform.proj[1][1] *= -1;
    auto& current_global = shadow_global_uniforms_[current_frame_].first;
    std::memcpy(current_global.mapped, &global_uniform, current_global.size);
    vkCmdBindDescriptorSets(
        cmd_buf,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        shadow_material_.getPipelineLayout(),
        0,
        1,
        &shadow_global_uniforms_[current_frame_].second,
        0,
        nullptr
    );

    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, shadow_material_.getPipeline());
}

void Renderer::beginColorPass(RenderingContext& ctx, Scene& scene, const Camera& camera) {
    VkCommandBuffer cmd_buf = cmd_bufs_[current_frame_];

    // Begin render pass
    VkRenderPassBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    begin_info.renderPass = render_pass_;
    begin_info.framebuffer = framebuffers_[img_idx_];
    begin_info.renderArea.extent = ctx.swapchain.extent;
    std::array<VkClearValue, 2> clear_values{};
    clear_values[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clear_values[1].depthStencil = {1.0f, 0};
    begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
    begin_info.pClearValues = clear_values.data();
    vkCmdBeginRenderPass(cmd_buf, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

    // Set viewport
    VkViewport viewport{};
    viewport.width = static_cast<float>(ctx.swapchain.extent.width);
    viewport.height = static_cast<float>(ctx.swapchain.extent.height);
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

    // Set scissor
    VkRect2D scissor{};
    scissor.extent = ctx.swapchain.extent;
    vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

    // Setup global uniform
    const auto& light = scene.getLight();
    GlobalUniform global_uniform{};
    global_uniform.view = glm::lookAt(
        camera.transform.position,
        camera.transform.position + camera.transform.getForward(),
        Vec3(0, -1, 0)
    );
    global_uniform.proj = camera.getProjection();
    global_uniform.light_view = glm::lookAt(
        light.transform.position,
        light.transform.position + light.transform.getForward(),
        Vec3(0, -1, 0)
    );
    global_uniform.light_proj = glm::ortho<float>(-10, 10, -10, 10, -10, 20); // TODO: Set from outside
    global_uniform.light_proj[1][1] *= -1;
    global_uniform.light_pos = light.transform.position;
    global_uniform.light_dir = light.transform.getForward();
    global_uniform.light_color = light.color;
    global_uniform.light_intensity = light.intensity;
    auto& current_global = shadow_global_uniforms_[current_frame_].first;
    std::memcpy(current_global.mapped, &global_uniform, current_global.size);
    vkCmdBindDescriptorSets(
        cmd_buf,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        shadow_material_.getPipelineLayout(), // CONCERN: Pipeline layout compatible
        0, // first set
        1, // set count
        &shadow_global_uniforms_[current_frame_].second,
        0, nullptr // dynamic params
    );
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

void Renderer::createShadowMaterial(RenderingContext& ctx) {
    auto vert = std::make_shared<Shader>(Shader::create(ctx, TEST_RESOURCE_DIR + std::string("shaders/shadow.vert.spv")));
    auto frag = std::make_shared<Shader>(Shader::create(ctx, TEST_RESOURCE_DIR + std::string("shaders/shadow.frag.spv")));
    shadow_material_.setVertexShader(vert);
    shadow_material_.setFragmentShader(frag);
    shadow_material_.setCullMode(VK_CULL_MODE_FRONT_BIT);
}
