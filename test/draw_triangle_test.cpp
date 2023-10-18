#include <gtest/gtest.h>
#include <GLFW/glfw3.h>
#include "kk_renderer/kk_renderer.h"
#ifndef TEST_RESOURCE_DIR
#define TEST_RESOURCE_DIR "./resources"
#endif

using namespace kk;
using namespace kk::renderer;

static const std::vector<Vertex> kTriangleVertices = {
    {{ 0.0f, -0.5f, 0.0f}, {}, {1.0f, 0.0f, 0.0f, 1.0f}},
    {{ 0.5f,  0.5f, 0.0f}, {}, {0.0f, 1.0f, 0.0f, 1.0f}},
    {{-0.5f,  0.5f, 0.0f}, {}, {0.0f, 0.0f, 1.0f, 1.0f}},
};

static const std::vector<uint32_t> kTriangleIndices = {
    0, 1, 2
};

/*
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

TEST(DrawTriangleTest, MaterialCreation) {
    const std::pair<size_t, size_t> size = { 800, 800 };
    const std::string name = "material creation test";
    Window window = Window::create(size.first, size.second, name);
    RenderingContext ctx = RenderingContext::create();
    Swapchain swapchain = Swapchain::create(ctx, window);
    Renderer renderer = Renderer::create(ctx, swapchain);

    auto vert = std::make_shared<Shader>(Shader::create(ctx, TEST_RESOURCE_DIR + std::string("/shaders/triangle.vert.spv")));
    auto frag = std::make_shared<Shader>(Shader::create(ctx, TEST_RESOURCE_DIR + std::string("/shaders/triangle.frag.spv")));
    auto material = std::make_shared<Material>();
    material->setVertexShader(vert);
    material->setFragmentShader(frag);
    renderer.compileMaterial(ctx, material);

    material->destroy(ctx);
    vert->destroy(ctx);
    frag->destroy(ctx);
    renderer.destroy(ctx);
    swapchain.destroy(ctx);
    ctx.destroy();
    window.destroy();
}

static void handleKey(Window& window, Transform& tf) {
    const std::unordered_map<int, std::function<void(Transform&)>> handlers = {
        {GLFW_KEY_W, [](auto& tf) {tf.position.y -= 0.01f; }},
        {GLFW_KEY_A, [](auto& tf) {tf.position.x -= 0.01f; }},
        {GLFW_KEY_S, [](auto& tf) {tf.position.y += 0.01f; }},
        {GLFW_KEY_D, [](auto& tf) {tf.position.x += 0.01f; }},
    };

    for (const auto& kvp : handlers) {
        const int key = kvp.first;
        const auto& handler = kvp.second;

        if (glfwGetKey(static_cast<GLFWwindow*>(window.acquireHandle()), key) == GLFW_PRESS) {
            handler(tf);
        }
    }
}

TEST(DrawTriangleTest, TransformedGeometryDrawing) {
    const std::pair<size_t, size_t> size = { 800, 800 };
    const std::string name = "draw transformed triangle test";
    Window window = Window::create(size.first, size.second, name);
    RenderingContext ctx = RenderingContext::create();
    Swapchain swapchain = Swapchain::create(ctx, window);
    
    // Prepare an object
    auto triangle = std::make_shared<Geometry>(Geometry::create(ctx, kTriangleVertices, kTriangleIndices));
    auto vert = std::make_shared<Shader>(Shader::create(ctx, TEST_RESOURCE_DIR + std::string("/shaders/triangle.vert.spv")));
    auto frag = std::make_shared<Shader>(Shader::create(ctx, TEST_RESOURCE_DIR + std::string("/shaders/triangle.frag.spv")));
    auto material = std::make_shared<Material>();
    material->setVertexShader(vert);
    material->setFragmentShader(frag);
    Renderable renderable{ triangle, material };
    Transform tf;
    PerspectiveCamera camera(45.0f, swapchain.extent.width / (float)swapchain.extent.height, 0.1f, 10.0f);
    camera.transform.position.z = -2.0f;

    Renderer renderer = Renderer::create(ctx, swapchain);
    while (!window.isClosed()) {
        window.pollEvents();
        handleKey(window, tf);
        if (renderer.beginFrame(ctx, swapchain)) {
            renderer.render(ctx, renderable, tf, camera);
            renderer.endFrame(ctx, swapchain);
        }
    }

    vkDeviceWaitIdle(ctx.device);
    material->destroy(ctx);
    frag->destroy(ctx);
    vert->destroy(ctx);
    triangle->destroy(ctx);
    renderer.destroy(ctx);
    swapchain.destroy(ctx);
    ctx.destroy();
    window.destroy();
}

TEST(DrawTriangleTest, MultipleTransformDrawing) {
    const size_t triangle_count = 5;
    const std::pair<size_t, size_t> size = { 800, 800 };
    const std::string name = "multiple transform test";
    Window window = Window::create(size.first, size.second, name);
    RenderingContext ctx = RenderingContext::create();
    Swapchain swapchain = Swapchain::create(ctx, window);

    // Prepare an object
    auto triangle = std::make_shared<Geometry>(Geometry::create(ctx, kTriangleVertices, kTriangleIndices));
    auto vert = std::make_shared<Shader>(Shader::create(ctx, TEST_RESOURCE_DIR + std::string("/shaders/triangle.vert.spv")));
    auto frag = std::make_shared<Shader>(Shader::create(ctx, TEST_RESOURCE_DIR + std::string("/shaders/triangle.frag.spv")));
    auto material = std::make_shared<Material>();
    material->setVertexShader(vert);
    material->setFragmentShader(frag);
    std::vector<Renderable> renderables(triangle_count);
    std::vector<Transform> transforms(triangle_count);
    for (size_t i = 0; i < triangle_count; ++i) {
        renderables[i] = Renderable(triangle, material);
        transforms[i].position.x = i / 2.0f;
        transforms[i].scale = Vec3(0.2f, 0.2f, 0.2f);
    }
    PerspectiveCamera camera(45.0f, swapchain.extent.width / (float)swapchain.extent.height, 0.1f, 10.0f);
    camera.transform.position.z = -2.0f;

    Renderer renderer = Renderer::create(ctx, swapchain);
    while (!window.isClosed()) {
        window.pollEvents();
        
        if (renderer.beginFrame(ctx, swapchain)) {
            for (size_t i = 0; i < triangle_count; ++i) {
                renderer.render(ctx, renderables[i], transforms[i], camera);
            }
            renderer.endFrame(ctx, swapchain);
        }
    }

    vkDeviceWaitIdle(ctx.device);
    material->destroy(ctx);
    frag->destroy(ctx);
    vert->destroy(ctx);
    triangle->destroy(ctx);
    renderer.destroy(ctx);
    swapchain.destroy(ctx);
    ctx.destroy();
    window.destroy();
}
*/
