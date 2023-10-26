#include "kk_renderer/Renderer.h"
#include <cassert>
#include <iostream>
#include <vector>
#include <cstring>
#include <glm/gtx/string_cast.hpp>

using namespace kk::renderer;

static VkRenderPass createRenderPass(RenderingContext& ctx, VkFormat swapchain_format);
static std::vector<VkFramebuffer> createFramebuffers(RenderingContext& ctx, const Swapchain& swapchain, const Image& depth, VkRenderPass render_pass);
static Image createDepthImage(RenderingContext& ctx, VkExtent2D extent);

Renderer Renderer::create(RenderingContext& ctx, Swapchain& swapchain) {
    Renderer renderer{};
    renderer.current_frame_ = renderer.img_idx_ = 0;
    
    // Allocate command buffer
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = ctx.cmd_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = kMaxConcurrentFrames;
    assert(vkAllocateCommandBuffers(ctx.device, &alloc_info, renderer.cmd_bufs_.data()) == VK_SUCCESS);

    // Create graphics pipelines and related objects
    renderer.render_pass_ = createRenderPass(ctx, swapchain.surface_format.format);
    renderer.depth_ = createDepthImage(ctx, swapchain.extent);
    renderer.framebuffers_ = createFramebuffers(ctx, swapchain, renderer.depth_, renderer.render_pass_);
    renderer.createDescriptors(ctx);

    return renderer;
}

void Renderer::destroy(RenderingContext& ctx) {
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

bool Renderer::beginFrame(RenderingContext& ctx, Swapchain& swapchain, const Camera& camera, const DirectionalLight& light) {
    VkResult ret = vkWaitForFences(ctx.device, 1, &ctx.fences[current_frame_], VK_TRUE, UINT64_MAX);
    if (ret != VK_SUCCESS) {
        return false;
    }

    setupView(camera, light);
    is_camera_binded_ = false;

    ret = vkAcquireNextImageKHR(ctx.device, swapchain.swapchain, UINT64_MAX, ctx.present_complete[current_frame_], VK_NULL_HANDLE, &img_idx_);
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

    vkCmdBeginRenderPass(current_buf, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    // Set viewport
    VkViewport viewport{};
    viewport.width = static_cast<float>(swapchain.extent.width);
    viewport.height = static_cast<float>(swapchain.extent.height);
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(current_buf, 0, 1, &viewport);

    // Set scissor
    VkRect2D scissor{};
    scissor.extent = swapchain.extent;
    vkCmdSetScissor(current_buf, 0, 1, &scissor);

    return true;
}

#include <glm/gtx/string_cast.hpp>
void Renderer::setupView(const Camera& camera, const DirectionalLight& light) {
    GlobalUniform uniform{};
    uniform.view = glm::lookAt(
        camera.transform.position,
        camera.transform.position + camera.transform.getForward(),
        Vec3(0, -1, 0)
    );
    uniform.proj = camera.getProjection();
    uniform.light_pos = light.transform.position;
    uniform.light_dir = light.transform.rotation * light.transform.getForward();
    uniform.light_color = light.color;
    uniform.light_intensity = light.intensity;

    std::cout << "light_dir: " << glm::to_string(uniform.light_dir) << std::endl;

    auto& dst_buf = global_uniforms_[current_frame_].first;
    std::memcpy(dst_buf.mapped, &uniform, dst_buf.size);
}

void Renderer::endFrame(RenderingContext& ctx, Swapchain& swapchain) {
    vkCmdEndRenderPass(cmd_bufs_[current_frame_]);
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
    present_info.pSwapchains = &swapchain.swapchain;
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

    VkAttachmentReference color_ref{};
    color_ref.attachment = 0;
    color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_ref{};
    depth_ref.attachment = 1;
    depth_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_ref;
    subpass.pDepthStencilAttachment = &depth_ref;

    VkSubpassDependency deps{};
    deps.srcSubpass = VK_SUBPASS_EXTERNAL;
    deps.dstSubpass = 0;
    deps.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    deps.srcAccessMask = 0;
    deps.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    deps.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = { color, depth };
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &deps;

    VkRenderPass render_pass;
    assert(vkCreateRenderPass(ctx.device, &renderPassInfo, nullptr, &render_pass) == VK_SUCCESS);

    return render_pass;
}

static std::vector<VkFramebuffer> createFramebuffers(RenderingContext& ctx, const Swapchain& swapchain, const Image& depth, VkRenderPass render_pass) {
    std::vector<VkFramebuffer> framebuffers(swapchain.images.size());
    for (size_t i = 0; i < swapchain.images.size(); i++) {
        std::array<VkImageView, 2> attachments = {
            swapchain.views[i],
            depth.view
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = render_pass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapchain.extent.width;
        framebufferInfo.height = swapchain.extent.height;
        framebufferInfo.layers = 1;

        assert(vkCreateFramebuffer(ctx.device, &framebufferInfo, nullptr, &framebuffers[i]) == VK_SUCCESS);
    }

    return framebuffers;
}

static Image createDepthImage(RenderingContext& ctx, VkExtent2D extent) {
    // TODO: Query format support
    VkFormat format = VK_FORMAT_D32_SFLOAT;

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
    VkDescriptorSetLayoutBinding global{};
    global.binding = 0;
    global.descriptorCount = 1;
    global.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    global.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo global_info{};
    global_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    global_info.bindingCount = 1;
    global_info.pBindings = &global;
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
        auto& target_buf = global_uniforms_[i].first;
        auto& target_desc = global_uniforms_[i].second;

        // Allocate
        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = ctx.desc_pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &global_uniform_layout_;
        assert(vkAllocateDescriptorSets(ctx.device, &alloc_info, &target_desc) == VK_SUCCESS);

        // Create buffer resource
        target_buf = Buffer::create(
            ctx,
            sizeof(GlobalUniform),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        vkMapMemory(ctx.device, target_buf.memory, 0, target_buf.size, 0, &target_buf.mapped);

        // Update descriptor
        VkWriteDescriptorSet write_buf{};
        write_buf.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_buf.dstSet = target_desc;
        write_buf.dstBinding = 0;
        write_buf.descriptorCount = 1;
        write_buf.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        
        VkDescriptorBufferInfo buf_info{};
        buf_info.buffer = target_buf.buffer;
        buf_info.offset = 0;
        buf_info.range = target_buf.size;
        write_buf.pBufferInfo = &buf_info;

        vkUpdateDescriptorSets(ctx.device, 1, &write_buf, 0, nullptr);
    }
}
