#include "kk_renderer/Editor.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>

using namespace kk;
using namespace kk::renderer;

static VkRenderPass createRenderPass(RenderingContext& ctx, VkFormat swapchain_format /* TODO: remove swapchain_format */);

struct kk::renderer::EditorImpl {
    void init(RenderingContext& ctx, const WindowPtr& window) {
        swapchain_extent_ = ctx.swapchain.extent;

        imgui_ctx = ImGui::CreateContext();
        ImGui::SetCurrentContext(imgui_ctx);
        ImGui_ImplGlfw_InitForVulkan(static_cast<GLFWwindow*>(window->acquireHandle()), true);
        ImGui::StyleColorsDark();

        ImGui_ImplVulkan_InitInfo info{};
        info.Instance = ctx.instance;
        info.PhysicalDevice = ctx.gpu;
        info.Device = ctx.device;
        info.QueueFamily = ctx.graphics_family;
        info.Queue = ctx.graphics_queue;
        info.PipelineCache = VK_NULL_HANDLE;
        info.DescriptorPool = ctx.desc_pool;
        info.Allocator = VK_NULL_HANDLE;
        info.MinImageCount = 2;
        info.ImageCount = static_cast<uint32_t>(ctx.swapchain.images.size());
        info.CheckVkResultFn = VK_NULL_HANDLE;

        render_pass_ = createRenderPass(ctx, ctx.swapchain.format.format);
        ImGui_ImplVulkan_Init(&info, render_pass_);
    }

    void uploadFonts(RenderingContext& ctx) {
        ctx.submitCmdsImmediate([](VkCommandBuffer cmd) {
            ImGui_ImplVulkan_CreateFontsTexture(cmd);
        });
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    void beginRender(VkCommandBuffer cmd_buf, VkFramebuffer framebuffer) {
        VkRenderPassBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = render_pass_;
        info.framebuffer = framebuffer;
        info.renderArea.extent = swapchain_extent_;
        info.clearValueCount = 0;
        info.pClearValues = nullptr;

        vkCmdBeginRenderPass(cmd_buf, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    void endRender(VkCommandBuffer cmd_buf) {
        vkCmdEndRenderPass(cmd_buf);
    }

    void render(VkCommandBuffer cmd_buf, Transform& transform) {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("model");

        static float pos_input[3] = { transform.position.x, transform.position.y, transform.position.z };
        ImGui::InputFloat3("position", pos_input);
        transform.position = Vec3(pos_input[0], pos_input[1], pos_input[2]);

        static float rot_input[3] = { glm::eulerAngles(transform.rotation).x, glm::eulerAngles(transform.rotation).y, glm::eulerAngles(transform.rotation).z };
        ImGui::InputFloat3("rotation", rot_input);
        transform.rotation = kk::Quat(kk::Vec3(rot_input[0], rot_input[1], rot_input[2]));

        static float scale_input[3] = { transform.scale.x, transform.scale.y, transform.scale.z };
        ImGui::InputFloat3("scale", scale_input);
        transform.scale = kk::Vec3(scale_input[0], scale_input[1], scale_input[2]);

        ImGui::End();

        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();
        ImGui_ImplVulkan_RenderDrawData(draw_data, cmd_buf);
    }

    void destroy(RenderingContext& ctx) {
        vkDestroyRenderPass(ctx.device, render_pass_, nullptr);
    }

    ImGuiContext* imgui_ctx;
    VkRenderPass render_pass_;
    VkExtent2D swapchain_extent_;
};

Editor::Editor() {
    impl_ = new EditorImpl();
}

void Editor::init(RenderingContext& ctx, const WindowPtr& window) {
    impl_->init(ctx, window);
    impl_->uploadFonts(ctx);
}

static Quat angleAxis(float rad, const Vec3& axis) {
    return Quat(
        std::cos(rad / 2.0f),
        axis.x * std::sin(rad / 2.0f),
        axis.y * std::sin(rad / 2.0f),
        axis.z * std::sin(rad / 2.0f)
    );
}

static void handleCamera(GLFWwindow* window, Camera& camera) {
    const float translate_speed = 0.005f;
    const float rotate_speed = 0.001f;
    static float prev_x = 0.0, prev_y = 0.0;

    double x, y;
    glfwGetCursorPos(window, &x, &y);
    const float diff_x = (float)x - prev_x;
    const float diff_y = (float)y - prev_y;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_3) == GLFW_PRESS) {
        // Wheel click
        camera.transform.position += camera.transform.rotation * (translate_speed * Vec3(-diff_x, -diff_y, 0));
    }
    else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS) {
        // Right click
        const Vec3 axis = -diff_y * camera.transform.getRight() + -diff_x * camera.transform.getUp();
        camera.transform.rotation = angleAxis(rotate_speed, axis) * camera.transform.rotation;
    }

    prev_x = x;
    prev_y = y;
}

void Editor::update(VkCommandBuffer cmd_buf, VkFramebuffer framebuffer, void* window, Transform& model, Camera& camera) {
    handleCamera(static_cast<GLFWwindow*>(window), camera);
    impl_->beginRender(cmd_buf, framebuffer);
    impl_->render(cmd_buf, model);
    impl_->endRender(cmd_buf);
}

void Editor::terminate(RenderingContext& ctx) {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    impl_->destroy(ctx);
}

static VkRenderPass createRenderPass(RenderingContext& ctx, VkFormat swapchain_format /* TODO: remove swapchain_format */) {
    VkAttachmentDescription color{};
    color.format = swapchain_format;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depth{};
    depth.format = VK_FORMAT_D32_SFLOAT; // TODO: Query format support
    depth.samples = VK_SAMPLE_COUNT_1_BIT;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
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
