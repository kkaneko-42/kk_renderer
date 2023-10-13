#include <gtest/gtest.h>
#include "kk_renderer/kk_renderer.h"

using namespace kk::renderer;

TEST(RenderingLoopTest, WindowCreation) {
    const std::pair<size_t, size_t> size = { 800, 800 };
    const std::string name = "create window test";
    Window window = Window::create(size.first, size.second, name);

    EXPECT_EQ(window.getName(), name);
    EXPECT_EQ(window.getSize(), size);
    EXPECT_TRUE(window.acquireHandle() != nullptr);

    window.destroy();
}

TEST(RenderingLoopTest, WindowLoop) {
    const std::pair<size_t, size_t> size = { 800, 800 };
    const std::string name = "window loop test";
    Window window = Window::create(size.first, size.second, name);

    while (!window.isClosed()) {
        window.pollEvents();
    }

    window.destroy();
}

TEST(RenderingLoopTest, ContextCreation) {
    RenderingContext ctx = RenderingContext::create();
    ctx.destroy();
}

TEST(RenderingLoopTest, SwapchainCreation) {
    const std::pair<size_t, size_t> size = { 800, 800 };
    const std::string name = "swap chain creation test";
    Window window = Window::create(size.first, size.second, name);
    RenderingContext ctx = RenderingContext::create();

    Swapchain swapchain = Swapchain::create(ctx, window);

    swapchain.destroy(ctx);
    ctx.destroy();
    window.destroy();
}

TEST(RenderingLoopTest, RendererCreation) {
    const std::pair<size_t, size_t> size = { 800, 800 };
    const std::string name = "rendering loop test";
    Window window = Window::create(size.first, size.second, name);
    RenderingContext ctx = RenderingContext::create();
    Swapchain swapchain = Swapchain::create(ctx, window);

    Renderer renderer = Renderer::create(ctx, swapchain.surface_format.format);

    renderer.destroy(ctx);
    swapchain.destroy(ctx);
    ctx.destroy();
    window.destroy();
}

/*
TEST(RenderingLoopTest, RenderingLoop) {
    const std::pair<size_t, size_t> size = { 800, 800 };
    const std::string name = "rendering loop test";
    Window window = Window::create(size.first, size.second, name);
    RenderingContext ctx = RenderingContext::create();
    Swapchain swapchain = Swapchain::create(ctx, window);

    Renderer renderer = Renderer::create(ctx, swapchain.surface_format.format);
    while (!window.isClosed()) {
        window.pollEvents();
        if (renderer.beginFrame(ctx, swapchain)) {
            // Record command buffer
            renderer.endFrame(ctx, swapchain);
        }
    }

    swapchain.destroy(ctx);
    ctx.destroy();
    window.destroy();
}
*/
