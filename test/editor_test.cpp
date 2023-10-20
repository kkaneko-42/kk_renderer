#include <gtest/gtest.h>
#include "kk_renderer/kk_renderer.h"
#include "kk_renderer/Editor.h"

using namespace kk::renderer;

TEST(EditorTest, RenderingLoop) {
    const std::pair<size_t, size_t> size = { 800, 800 };
    const std::string name = "draw transformed triangle test";
    Window window = Window::create(size.first, size.second, name);
    RenderingContext ctx = RenderingContext::create();
    Swapchain swapchain = Swapchain::create(ctx, window);

    Renderer renderer = Renderer::create(ctx, swapchain);
    Editor editor;
    editor.init(ctx, window, swapchain, renderer);
    while (!window.isClosed()) {
        window.pollEvents();
        if (renderer.beginFrame(ctx, swapchain)) {
            editor.render(renderer.getCmdBuf());
            renderer.endFrame(ctx, swapchain);
        }
    }

}
