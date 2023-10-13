#include <gtest/gtest.h>
#include "kk_renderer/kk_renderer.h"

using namespace kk::renderer;

TEST(DrawTriangleTest, DrawTriangle) {
    const std::pair<size_t, size_t> size = { 800, 800 };
    const std::string name = "draw triangle test";
    Window window = Window::create(size.first, size.second, name);
    RenderingContext ctx = RenderingContext::create();
    Swapchain swapchain = Swapchain::create(ctx, window);

    Renderer renderer = Renderer::create(ctx, swapchain);
    while (!window.isClosed()) {
        window.pollEvents();
        if (renderer.beginFrame(ctx, swapchain)) {
            renderer.recordCommands(swapchain);
            renderer.endFrame(ctx, swapchain);
        }
    }

    renderer.destroy(ctx);
    swapchain.destroy(ctx);
    ctx.destroy();
    window.destroy();
}

