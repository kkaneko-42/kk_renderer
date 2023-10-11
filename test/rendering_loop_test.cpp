#include <gtest/gtest.h>
#include "kk_renderer/kk_renderer.h"

using namespace kk::renderer;

TEST(RenderingLoopTest, WindowCreation) {
    const std::pair<size_t, size_t> size = { 800, 800 };
    const std::string name = "test";
    Window window = Window::create(size.first, size.second, name);

    EXPECT_EQ(window.getName(), name);
    EXPECT_EQ(window.getSize(), size);
    EXPECT_TRUE(window.acquireHandle() != nullptr);

    Window::destroy(window);
}

TEST(RenderingLoopTest, ContextCreation) {
    RenderingContext ctx = RenderingContext::create();
    RenderingContext::destory(ctx);
}
