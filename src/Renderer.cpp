#include "kk_renderer/Renderer.h"
#include <cassert>
#include <iostream>
#include <vector>
#include <cstring>

using namespace kk::renderer;

static VkRenderPass createRenderPass(RenderingContext& ctx, VkFormat swapchain_format);
static std::vector<VkFramebuffer> createFramebuffers(RenderingContext& ctx, Swapchain& swapchain, VkRenderPass render_pass);

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
    renderer.framebuffers_ = createFramebuffers(ctx, swapchain, renderer.render_pass_);

    return renderer;
}

void Renderer::destroy(RenderingContext& ctx) {
    for (auto& uniform : uniforms_) {
        for (size_t i = 0; i < kMaxConcurrentFrames; ++i) {
            uniform.second[i].destroy(ctx);
        }
    }

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

bool Renderer::beginFrame(RenderingContext& ctx, Swapchain& swapchain) {
    VkResult ret = vkWaitForFences(ctx.device, 1, &ctx.fences[current_frame_], VK_TRUE, UINT64_MAX);
    if (ret != VK_SUCCESS) {
        return false;
    }

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
    VkClearValue clear_color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_color;
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
    // Setup uniform buffer
    if (uniforms_.find(renderable.id) == uniforms_.end()) {
        for (size_t i = 0; i < kMaxConcurrentFrames; ++i) {
            Buffer& uniform = uniforms_[renderable.id][i];
            uniform = Buffer::create(
                ctx,
                sizeof(Mat4),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );
            vkMapMemory(ctx.device, uniform.memory, 0, uniform.size, 0, &uniform.mapped);
        }
    }

    // Set pipeline state
    if (!renderable.material->isCompiled()) {
        renderable.material->compile(ctx, render_pass_);
    }

    // Setup descriptor sets
    // NOTE: Assert material is compiled (valid descriptor set layouts required)
    const auto& layouts = renderable.material->getDescriptorSetLayouts();
    if (renderable.desc_sets[0].size() == 0 && layouts.size() != 0) {
        for (size_t i = 0; i < kMaxConcurrentFrames; ++i) {
            VkDescriptorSetAllocateInfo alloc_info{};
            alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            alloc_info.descriptorPool = ctx.desc_pool;
            alloc_info.descriptorSetCount = static_cast<uint32_t>(layouts.size());
            alloc_info.pSetLayouts = layouts.data();

            auto& target_sets = renderable.desc_sets[i];
            target_sets.resize(layouts.size());
            assert(vkAllocateDescriptorSets(ctx.device, &alloc_info, target_sets.data()) == VK_SUCCESS);

            VkWriteDescriptorSet write_texture{};
            write_texture.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_texture.dstSet = target_sets[0];
            write_texture.dstBinding = 0;
            write_texture.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write_texture.descriptorCount = 1;

            VkDescriptorImageInfo tex_info{};
            tex_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            auto& texture = renderable.material->getTexture();
            tex_info.imageView = texture->view;
            tex_info.sampler = texture->sampler;
            write_texture.pImageInfo = &tex_info;

            VkWriteDescriptorSet write_uniform{};
            write_uniform.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_uniform.dstSet = target_sets[1];
            write_uniform.dstBinding = 0;
            write_uniform.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write_uniform.descriptorCount = 1;

            VkDescriptorBufferInfo buf_info{};
            buf_info.buffer = uniforms_[renderable.id][i].buffer;
            buf_info.offset = 0;
            buf_info.range = uniforms_[renderable.id][i].size;
            write_uniform.pBufferInfo = &buf_info;

            VkWriteDescriptorSet writes[] = { write_texture, write_uniform };
            vkUpdateDescriptorSets(ctx.device, 2, writes, 0, nullptr);
        }
    }
}

void Renderer::render(RenderingContext& ctx, Renderable& renderable, const Transform& transform, const Camera& camera) {
    prepareRendering(ctx, renderable);

    VkCommandBuffer cmd_buf = cmd_bufs_[current_frame_];
    Material& material = *renderable.material;
    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, material.getPipeline());

    // Build MVP matrix
    const Mat4 model =
        glm::scale(glm::mat4(1.0f), transform.scale) *
        glm::mat4_cast(transform.rotation) *
        glm::translate(glm::mat4(1.0f), transform.position)
        ;
    const Mat4 view = glm::lookAt(
        camera.transform.position,
        camera.transform.position + camera.transform.rotation * Vec3(0.0f, 0.0f, 1.0f),
        Vec3(0.0f, -1.0f, 0.0f)
    );
    const Mat4 proj = camera.getProjection();
    const Mat4 mvp = proj * view * model;

    // Copy MVP to uniform buffer
    std::memcpy(uniforms_[renderable.id][current_frame_].mapped, &mvp, sizeof(Mat4));

    // Set descriptor
    vkCmdBindDescriptorSets(
        cmd_bufs_[current_frame_],
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        material.getPipelineLayout(),
        0,
        static_cast<uint32_t>(renderable.desc_sets[current_frame_].size()),
        renderable.desc_sets[current_frame_].data(),
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
    VkAttachmentDescription color_attachment{};
    color_attachment.format = swapchain_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    VkSubpassDependency deps{};
    deps.srcSubpass = VK_SUBPASS_EXTERNAL;
    deps.dstSubpass = 0;
    deps.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    deps.srcAccessMask = 0;
    deps.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    deps.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &color_attachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &deps;

    VkRenderPass render_pass;
    assert(vkCreateRenderPass(ctx.device, &renderPassInfo, nullptr, &render_pass) == VK_SUCCESS);

    return render_pass;
}

static std::vector<VkFramebuffer> createFramebuffers(RenderingContext& ctx, Swapchain& swapchain, VkRenderPass render_pass) {
    std::vector<VkFramebuffer> framebuffers(swapchain.images.size());
    for (size_t i = 0; i < swapchain.images.size(); i++) {
        VkImageView attachments[] = {
            swapchain.views[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = render_pass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapchain.extent.width;
        framebufferInfo.height = swapchain.extent.height;
        framebufferInfo.layers = 1;

        assert(vkCreateFramebuffer(ctx.device, &framebufferInfo, nullptr, &framebuffers[i]) == VK_SUCCESS);
    }

    return framebuffers;
}

static std::array<ResourceDescriptor, kMaxConcurrentFrames> createUniformBuffers(RenderingContext& ctx) {

}
