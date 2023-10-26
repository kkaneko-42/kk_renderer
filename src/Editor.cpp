#include "kk_renderer/Editor.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

using namespace kk;
using namespace kk::renderer;

struct kk::renderer::EditorImpl {
    void init(RenderingContext& ctx, Window& window, const Swapchain& swapchain, const Renderer& renderer) {
        imgui_ctx = ImGui::CreateContext();
        ImGui::SetCurrentContext(imgui_ctx);
        ImGui_ImplGlfw_InitForVulkan(static_cast<GLFWwindow*>(window.acquireHandle()), true);
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
        info.ImageCount = static_cast<uint32_t>(swapchain.images.size()); // TODO: get from swapchain
        info.CheckVkResultFn = VK_NULL_HANDLE;
        ImGui_ImplVulkan_Init(&info, renderer.getRenderPass());
    }

    void uploadFonts(RenderingContext& ctx) {
        ctx.submitCmdsImmediate([](VkCommandBuffer cmd) {
            ImGui_ImplVulkan_CreateFontsTexture(cmd);
        });
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    ImGuiContext* imgui_ctx;
};

Editor::Editor() {
    impl_ = new EditorImpl();
}

void Editor::init(RenderingContext& ctx, Window& window, const Swapchain& swapchain, const Renderer& renderer) {
    impl_->init(ctx, window, swapchain, renderer);
    impl_->uploadFonts(ctx);
}

void Editor::render(VkCommandBuffer cmd_buf) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("SAMPLE WINDOW");
    ImGui::Text("Hogehoge Fugafuga");
    ImGui::End();

    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(draw_data, cmd_buf);
}

void Editor::render(VkCommandBuffer cmd_buf, Transform& transform) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("model");
    ImGui::InputFloat3("position", reinterpret_cast<float*>(&transform.position));
    static float rot_input[3] = { 0.0f, 0.0f, 0.0f };
    ImGui::InputFloat3("rotation", rot_input);
    transform.rotation = kk::Quat(kk::Vec3(rot_input[0], rot_input[1], rot_input[2]));
    static float scale_input[3] = { 1.0f, 1.0f, 1.0f };
    ImGui::InputFloat3("scale", scale_input);
    transform.scale = kk::Vec3(scale_input[0], scale_input[1], scale_input[2]);
    ImGui::End();

    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(draw_data, cmd_buf);
}

static Quat angleAxis(float rad, const Vec3& axis) {
    return Quat(
        std::cos(rad / 2.0f),
        axis.x * std::sin(rad / 2.0f),
        axis.y * std::sin(rad / 2.0f),
        axis.z * std::sin(rad / 2.0f)
    );
}

#include <iostream>
static void handleCamera(GLFWwindow* window, Camera& camera) {
    const float translate_speed = 0.005f;
    const float rotate_speed = 0.001f;
    static double prev_x = 0.0, prev_y = 0.0;

    double x, y;
    glfwGetCursorPos(window, &x, &y);
    const double diff_x = x - prev_x;
    const double diff_y = y - prev_y;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_3) == GLFW_PRESS) {
        // Wheel click
        camera.transform.position += camera.transform.rotation * (translate_speed * Vec3(-diff_x, -diff_y, 0));
    }
    else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS) {
        // Right click
        const Vec3 axis = Vec3(-diff_y, diff_x, 0);
        camera.transform.rotation = angleAxis(rotate_speed, axis) * camera.transform.rotation;
    }

    prev_x = x;
    prev_y = y;
}

void Editor::update(VkCommandBuffer cmd_buf, void* window, Transform& model, Camera& camera) {
    handleCamera(static_cast<GLFWwindow*>(window), camera);
    render(cmd_buf, model);
}

void Editor::terminate() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
}
