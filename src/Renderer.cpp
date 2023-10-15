#include "kk_renderer/Renderer.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>

#ifndef TEST_RESOURCE_DIR
#define TEST_RESOURCE_DIR "./resources"
#endif

using namespace kk::renderer;

static VkRenderPass createRenderPass(RenderingContext& ctx, VkFormat swapchain_format);
static VkPipelineLayout createPipelineLayout(RenderingContext& ctx, const VkDescriptorSetLayout& set_layout);
static VkPipeline createPipeline(RenderingContext& ctx, VkPipelineLayout layout, VkRenderPass render_pass);
static std::array<VkFramebuffer, kMaxConcurrentFrames> createFramebuffers(RenderingContext& ctx, Swapchain& swapchain, VkRenderPass render_pass);

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

    // Prepare uniform buffer and its descriptor
    ResourceDescriptor resources;
    resources.bindBuffer(0, std::make_shared<Buffer>(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
    resources.buildLayout(ctx);
    renderer.desc_layout_ = resources.getLayout();

    // Create graphics pipelines and related objects
    renderer.render_pass_ = createRenderPass(ctx, swapchain.surface_format.format);
    renderer.pipeline_layout_ = createPipelineLayout(ctx, renderer.desc_layout_);
    renderer.pipeline_ = createPipeline(ctx, renderer.pipeline_layout_, renderer.render_pass_);
    renderer.framebuffers_ = createFramebuffers(ctx, swapchain, renderer.render_pass_);

    return renderer;
}

void Renderer::destroy(RenderingContext& ctx) {
    vkDeviceWaitIdle(ctx.device);

    for (auto& framebuffer : framebuffers_) {
        vkDestroyFramebuffer(ctx.device, framebuffer, nullptr);
    }
    
    vkDestroyPipeline(ctx.device, pipeline_, nullptr);
    vkDestroyPipelineLayout(ctx.device, pipeline_layout_, nullptr);
    vkDestroyRenderPass(ctx.device, render_pass_, nullptr);
    vkDestroyDescriptorSetLayout(ctx.device, desc_layout_, nullptr);

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

void Renderer::recordCommands() {
    VkCommandBuffer current_buf = cmd_bufs_[current_frame_];

    vkCmdBindPipeline(current_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
    vkCmdDraw(current_buf, 3, 1, 0, 0);
}

void Renderer::render(const Geometry& geometry) {
    VkCommandBuffer current_buf = cmd_bufs_[current_frame_];

    vkCmdBindPipeline(current_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);

    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(current_buf, 0, 1, &geometry.vertex_buffer.buffer, offsets);
    vkCmdBindIndexBuffer(current_buf, geometry.index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(current_buf, static_cast<uint32_t>(geometry.indices.size()), 1, 0, 0, 0);
}

std::ostream& operator<<(std::ostream& os, const glm::mat4& mat) {
    for (size_t i = 0; i < 4; ++i) {
        for (size_t j = 0; j < 4; ++j) {
            std::cout << mat[i][j] << " ";
        }
        std::cout << std::endl;
    }
    return os;
}

void Renderer::render(Renderable& renderable) {
    // Build MVP matrix
    const glm::mat4 model = 
        glm::scale(glm::mat4(1.0f), renderable.transform.scale) *
        glm::mat4_cast(renderable.transform.rotation) *
        glm::translate(glm::mat4(1.0f), renderable.transform.position)
    ;
    // TODO: These should be got from camera
    const glm::mat4 view  = glm::lookAt(glm::vec3(0.0f, 0.0f, -2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 10.0f);
    projection[1][1] *= -1;
    const glm::mat4 mvp = projection * view * model;

    // Copy MVP to uniform buffer
    std::memcpy(renderable.resources[current_frame_].getBuffer(0)->mapped, &mvp, sizeof(glm::mat4));

    VkDescriptorSet set = renderable.resources[current_frame_].getSet();
    vkCmdBindDescriptorSets(
        cmd_bufs_[current_frame_],
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_layout_,
        0,
        1,
        &set,
        0,
        nullptr
    );
    render(renderable.geometry);
}

void Renderer::render(Renderable& renderable, const Camera& camera) {
    // Build MVP matrix
    const Mat4 model =
        glm::scale(glm::mat4(1.0f), renderable.transform.scale) *
        glm::mat4_cast(renderable.transform.rotation) *
        glm::translate(glm::mat4(1.0f), renderable.transform.position)
        ;
    const Mat4 view = glm::lookAt(
        camera.transform.position,
        Vec3(0.0f),
        Vec3(0.0f, -1.0f, 0.0f)
    );
    const Mat4 proj = camera.getProjection();
    const Mat4 mvp = proj * view * model;

    // Copy MVP to uniform buffer
    std::memcpy(renderable.resources[current_frame_].getBuffer(0)->mapped, &mvp, sizeof(glm::mat4));

    VkDescriptorSet set = renderable.resources[current_frame_].getSet();
    vkCmdBindDescriptorSets(
        cmd_bufs_[current_frame_],
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_layout_,
        0,
        1,
        &set,
        0,
        nullptr
    );
    render(renderable.geometry);
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

static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

static VkShaderModule createShaderModule(RenderingContext& ctx, const std::vector<char>& code) {
    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = static_cast<uint32_t>(code.size());
    info.pCode = reinterpret_cast<const uint32_t*>(code.data());
    
    VkShaderModule shader;
    assert(vkCreateShaderModule(ctx.device, &info, nullptr, &shader) == VK_SUCCESS);
    return shader;
}

static VkPipelineLayout createPipelineLayout(RenderingContext& ctx, const VkDescriptorSetLayout& set_layout) {
    VkPipelineLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.setLayoutCount = 1;
    info.pSetLayouts = &set_layout;
    info.pushConstantRangeCount = 0;

    VkPipelineLayout layout;
    assert(vkCreatePipelineLayout(ctx.device, &info, nullptr, &layout) == VK_SUCCESS);
    return layout;
}

static VkPipeline createPipeline(RenderingContext& ctx, VkPipelineLayout layout, VkRenderPass render_pass) {
    auto vert_code = readFile(TEST_RESOURCE_DIR + std::string("shaders/triangle.vert.spv"));
    auto frag_code = readFile(TEST_RESOURCE_DIR + std::string("shaders/triangle.frag.spv"));

    VkShaderModule vert_module = createShaderModule(ctx, vert_code);
    VkShaderModule frag_module = createShaderModule(ctx, frag_code);

    VkPipelineShaderStageCreateInfo vert_info{};
    vert_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_info.module = vert_module;
    vert_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_info{};
    frag_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_info.module = frag_module;
    frag_info.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vert_info, frag_info };

    const auto binding_desc = Vertex::getBindingDescription();
    const auto attr_desc = Vertex::getAttributeDescriptions();
    VkPipelineVertexInputStateCreateInfo vert_input{};
    vert_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vert_input.vertexBindingDescriptionCount = 1;
    vert_input.pVertexBindingDescriptions = &binding_desc;
    vert_input.vertexAttributeDescriptionCount = static_cast<uint32_t>(attr_desc.size());
    vert_input.pVertexAttributeDescriptions = attr_desc.data();

    VkPipelineInputAssemblyStateCreateInfo input_asm{};
    input_asm.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_asm.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_asm.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewport{};
    viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport.viewportCount = 1;
    viewport.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blending{};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;
    color_blending.blendConstants[0] = 0.0f;
    color_blending.blendConstants[1] = 0.0f;
    color_blending.blendConstants[2] = 0.0f;
    color_blending.blendConstants[3] = 0.0f;

    std::vector<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamicState.pDynamicStates = dynamic_states.data();

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vert_input;
    pipelineInfo.pInputAssemblyState = &input_asm;
    pipelineInfo.pViewportState = &viewport;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &color_blending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = layout;
    pipelineInfo.renderPass = render_pass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    VkPipeline pipeline;
    assert(vkCreateGraphicsPipelines(ctx.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) == VK_SUCCESS);

    vkDestroyShaderModule(ctx.device, vert_module, nullptr);
    vkDestroyShaderModule(ctx.device, frag_module, nullptr);

    return pipeline;
}

static std::array<VkFramebuffer, kMaxConcurrentFrames> createFramebuffers(RenderingContext& ctx, Swapchain& swapchain, VkRenderPass render_pass) {
    std::array<VkFramebuffer, kMaxConcurrentFrames> framebuffers;
    for (size_t i = 0; i < kMaxConcurrentFrames; i++) {
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
