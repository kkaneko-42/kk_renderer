#include "kk_renderer/Renderer.h"
#include <cassert>
#include <iostream>

using namespace kk::renderer;

VkRenderPass createRenderPass(RenderingContext& ctx, VkFormat swapchain_format /* TODO: remove swapchain_format */);

Renderer Renderer::create(RenderingContext& ctx, VkFormat swapchain_format /* TODO: remove swapchain_format */) {
    Renderer renderer{};
    renderer.current_frame_ = renderer.img_idx_ = 0;
    
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = ctx.cmd_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = kMaxConcurrentFrames;
    assert(vkAllocateCommandBuffers(ctx.device, &alloc_info, renderer.cmd_bufs_.data()) == VK_SUCCESS);

    renderer.render_pass_ = createRenderPass(ctx, swapchain_format);

    return renderer;
}

void Renderer::destroy(RenderingContext& ctx) {
    vkDestroyRenderPass(ctx.device, render_pass_, nullptr);
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

    ret = vkResetCommandBuffer(cmd_bufs_[current_frame_], 0);
    if (ret != VK_SUCCESS) {
        // NOTE: ret == VK_ERROR_OUT_OF_DEVICE_MEMORY
        return false;
    }

    return true;
}

void Renderer::endFrame(RenderingContext& ctx, Swapchain& swapchain) {
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
    present_info.pImageIndices = &img_idx_; // TODO

    ret = vkQueuePresentKHR(ctx.present_queue, &present_info);
    if (ret == VK_ERROR_OUT_OF_DATE_KHR || ret == VK_SUBOPTIMAL_KHR) {
        // TODO: Recreate swapchain
    }
    else if (ret != VK_SUCCESS) {
        std::cerr << "Failed to present submit. Idx: " << current_frame_ << std::endl;
    }

    current_frame_ = (current_frame_ + 1) % kMaxConcurrentFrames;
}

VkRenderPass createRenderPass(RenderingContext& ctx, VkFormat swapchain_format /* TODO: remove swapchain_format */) {
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
