#include <gtest/gtest.h>
#include "kk_renderer/kk_renderer.h"
#include "kk_renderer/platform/GlfwWindow.h"

using namespace kk::renderer;

TEST(RenderingLoopTest, WindowCreation) {
    const std::pair<size_t, size_t> size = { 800, 800 };
    const std::string name = "create window test";
    WindowPtr window = std::make_unique<GlfwWindow>(size.first, size.second, name);

    EXPECT_EQ(window->getName(), name);
    EXPECT_EQ(window->getSize(), size);
}

TEST(RenderingLoopTest, WindowLoop) {
    const size_t width = 800, height = 800;
    const std::string name = "window loop test";
    WindowPtr window = std::make_unique<GlfwWindow>(width, height, name);
    
    EXPECT_TRUE(window->launch());
    while (!window->hasError()) {
        window->pollEvents();
    }

    window->terminate();
}

TEST(RenderingLoopTest, ContextCreation) {
    WindowPtr window = std::make_unique<GlfwWindow>(800, 800, "ctx creation");
    window->launch();

    RenderingContext ctx = RenderingContext::create(window);
    ctx.destroy();
    window->terminate();
}

/*
TEST(RenderingLoopTest, RendererCreation) {
    WindowPtr window = std::make_unique<GlfwWindow>(800, 800, "ctx creation");
    window->launch();
    RenderingContext ctx = RenderingContext::create(window);

    Renderer renderer = Renderer::create(ctx);

    renderer.destroy(ctx);
    ctx.destroy();
    window->terminate();
}

TEST(RenderingLoopTest, RenderingLoop) {
    const std::pair<size_t, size_t> size = { 800, 800 };
    const std::string name = "rendering loop test";
    Window window = Window::create(size.first, size.second, name);
    RenderingContext ctx = RenderingContext::create();
    Swapchain swapchain = Swapchain::create(ctx, window);

    Renderer renderer = Renderer::create(ctx, swapchain);
    while (!window.isClosed()) {
        window.pollEvents();
        if (renderer.beginFrame(ctx, swapchain)) {
            renderer.endFrame(ctx, swapchain);
        }
    }

    vkDeviceWaitIdle(ctx.device);
    renderer.destroy(ctx);
    swapchain.destroy(ctx);
    ctx.destroy();
    window.destroy();
}
*/
