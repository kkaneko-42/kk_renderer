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

    Window::destroy(window);
}

TEST(RenderingLoopTest, WindowLoop) {
    const std::pair<size_t, size_t> size = { 800, 800 };
    const std::string name = "window loop test";
    Window window = Window::create(size.first, size.second, name);

    while (!window.isClosed()) {
        window.pollEvents();
    }

    Window::destroy(window);
}

TEST(RenderingLoopTest, ContextCreation) {
    RenderingContext ctx = RenderingContext::create();
    RenderingContext::destory(ctx);
}

TEST(RenderingLoopTest, SwapChainCreation) {
    const std::pair<size_t, size_t> size = { 800, 800 };
    const std::string name = "swap chain creation test";
    Window window = Window::create(size.first, size.second, name);
    RenderingContext ctx = RenderingContext::create();

    SwapChain swap_chain = SwapChain::create(ctx, window);

    SwapChain::destroy(ctx, swap_chain);
    RenderingContext::destory(ctx);
    Window::destroy(window);
}
