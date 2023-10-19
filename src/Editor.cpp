#include "kk_renderer/Editor.h"
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

using namespace kk::renderer;

struct kk::renderer::EditorImpl {
    void init(RenderingContext& r_ctx, ) {
        ctx = ImGui::CreateContext();
        ImGui::SetCurrentContext(ctx);
        ImGui_ImplGlfw_InitForVulkan(/* window handle */nullptr, true);
        ImGui::StyleColorsDark();

        ImGui_ImplVulkan_InitInfo info{};
        info.Instance = r_ctx.instance;
        info.PhysicalDevice = r_ctx.gpu;
        info.Device = r_ctx.device;
        info.QueueFamily = r_ctx.graphics_family;
        info.Queue = r_ctx.graphics_queue;
        info.PipelineCache = VK_NULL_HANDLE;
        info.DescriptorPool = r_ctx.desc_pool;
        info.Allocator = VK_NULL_HANDLE;
        info.MinImageCount = 2;
        info.ImageCount = ;
        info.CheckVkResultFn = VK_NULL_HANDLE;
        VmGui_ImplVulkan_Init(&info);
    }

    ImGuiContext* ctx;
};

Editor::Editor() {
    impl_ = std::make_unique<EditorImpl>();
}

void Editor::init(RenderingContext& ctx) {
    impl_->init(ctx);
}
