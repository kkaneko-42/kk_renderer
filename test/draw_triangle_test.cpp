#include <gtest/gtest.h>
#include "kk_renderer/kk_renderer.h"

using namespace kk::renderer;

static const std::vector<Vertex> kTriangleVertices = {
    {{ 0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
    {{ 0.5f,  0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
    {{-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
};

static const std::vector<uint32_t> kTriangleIndices = {
    0, 1, 2
};

TEST(DrawTriangleTest, BufferCreation) {
    RenderingContext ctx = RenderingContext::create();
    Buffer buf = Buffer::create(
        ctx,
        sizeof(float) * 1,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    buf.destroy(ctx);
}

TEST(DrawTriangleTest, GeometryCreation) {
    RenderingContext ctx = RenderingContext::create();
    Geometry geometry = Geometry::create(ctx, kTriangleVertices, kTriangleIndices);

    geometry.destroy(ctx);
    ctx.destroy();
}

TEST(DrawTriangleTest, GeometryDrawing) {
    const std::pair<size_t, size_t> size = { 800, 800 };
    const std::string name = "draw triangle test";
    Window window = Window::create(size.first, size.second, name);
    RenderingContext ctx = RenderingContext::create();
    Swapchain swapchain = Swapchain::create(ctx, window);

    Geometry triangle = Geometry::create(ctx, kTriangleVertices, kTriangleIndices);
    Renderer renderer = Renderer::create(ctx, swapchain);
    while (!window.isClosed()) {
        window.pollEvents();
        if (renderer.beginFrame(ctx, swapchain)) {
            renderer.render(triangle);
            renderer.endFrame(ctx, swapchain);
        }
    }

    triangle.destroy(ctx);
    renderer.destroy(ctx);
    swapchain.destroy(ctx);
    ctx.destroy();
    window.destroy();
}
