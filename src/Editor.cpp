#include "kk_renderer/Editor.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

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
    ImGui::End();

    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(draw_data, cmd_buf);
}
