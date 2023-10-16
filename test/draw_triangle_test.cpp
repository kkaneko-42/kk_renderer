#include <gtest/gtest.h>
#include <GLFW/glfw3.h>
#include "kk_renderer/kk_renderer.h"
#ifndef TEST_RESOURCE_DIR
#define TEST_RESOURCE_DIR "./resources"
#endif

using namespace kk::renderer;

static const std::vector<Vertex> kTriangleVertices = {
    {{ 0.0f, -0.5f, 0.0f}, {}, {1.0f, 0.0f, 0.0f, 1.0f}},
    {{ 0.5f,  0.5f, 0.0f}, {}, {0.0f, 1.0f, 0.0f, 1.0f}},
    {{-0.5f,  0.5f, 0.0f}, {}, {0.0f, 0.0f, 1.0f, 1.0f}},
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

/*
template <class TransformedObject>
static void handleKey(int code, int state, TransformedObject& obj) {
    if (state == GLFW_RELEASE) {
        return;
    }

    std::cout << "called!" << std::endl;

    if (code == GLFW_KEY_W) {
        obj.transform.position.y -= 0.1f;
    }
    else if (code == GLFW_KEY_A) {
        obj.transform.position.x -= 0.1f;
    }
    else if (code == GLFW_KEY_S) {
        obj.transform.position.y += 0.1f;
    }
    else if (code == GLFW_KEY_D) {
        obj.transform.position.x += 0.1f;
    }
}

TEST(DrawTriangleTest, TransformedGeometryDrawing) {
    const std::pair<size_t, size_t> size = { 800, 800 };
    const std::string name = "draw transformed triangle test";
    Window window = Window::create(size.first, size.second, name);
    RenderingContext ctx = RenderingContext::create();
    Swapchain swapchain = Swapchain::create(ctx, window);
    
    // Prepare an object
    Geometry triangle = Geometry::create(ctx, kTriangleVertices, kTriangleIndices);
    Renderable renderable = Renderable::create(ctx, triangle);

    Renderer renderer = Renderer::create(ctx, swapchain);
    while (!window.isClosed()) {
        window.pollEvents();
        handleKey(GLFW_KEY_W, glfwGetKey(static_cast<GLFWwindow*>(window.acquireHandle()), GLFW_KEY_W), renderable);
        handleKey(GLFW_KEY_A, glfwGetKey(static_cast<GLFWwindow*>(window.acquireHandle()), GLFW_KEY_A), renderable);
        handleKey(GLFW_KEY_S, glfwGetKey(static_cast<GLFWwindow*>(window.acquireHandle()), GLFW_KEY_S), renderable);
        handleKey(GLFW_KEY_D, glfwGetKey(static_cast<GLFWwindow*>(window.acquireHandle()), GLFW_KEY_D), renderable);
        if (renderer.beginFrame(ctx, swapchain)) {
            renderer.render(renderable);
            renderer.endFrame(ctx, swapchain);
        }
    }

    renderable.destroy(ctx);
    triangle.destroy(ctx);
    renderer.destroy(ctx);
    swapchain.destroy(ctx);
    ctx.destroy();
    window.destroy();
}

TEST(DrawTriangleTest, MultipleTransformDrawing) {
    const size_t transform_count = 5;
    const std::pair<size_t, size_t> size = { 800, 800 };
    const std::string name = "multiple transform triangle test";
    Window window = Window::create(size.first, size.second, name);
    RenderingContext ctx = RenderingContext::create();
    Swapchain swapchain = Swapchain::create(ctx, window);

    // Prepare objects
    Geometry triangle = Geometry::create(ctx, kTriangleVertices, kTriangleIndices);
    std::vector<Renderable> renderables(transform_count);
    for (size_t i = 0; i < transform_count; ++i) {
        renderables[i] = Renderable::create(ctx, triangle);
        renderables[i].transform.position.x = (transform_count / 2.0f - i) / 2.0f;
        renderables[i].transform.scale = glm::vec3(0.5f);
    }
   
    Renderer renderer = Renderer::create(ctx, swapchain);
    while (!window.isClosed()) {
        window.pollEvents();
        if (renderer.beginFrame(ctx, swapchain)) {
            for (auto& r : renderables) {
                renderer.render(r);
            }
            renderer.endFrame(ctx, swapchain);
        }
    }

    for (auto& r : renderables) {
        r.destroy(ctx);
    }
    triangle.destroy(ctx);
    renderer.destroy(ctx);
    swapchain.destroy(ctx);
    ctx.destroy();
    window.destroy();
}

TEST(DrawTriangleTest, CameraDrawing) {
    const std::pair<size_t, size_t> size = { 800, 800 };
    const std::string name = "camera triangle test";
    Window window = Window::create(size.first, size.second, name);
    RenderingContext ctx = RenderingContext::create();
    Swapchain swapchain = Swapchain::create(ctx, window);

    // Prepare objects
    Geometry triangle = Geometry::create(ctx, kTriangleVertices, kTriangleIndices);
    Renderable renderable = Renderable::create(ctx, triangle);
    PerspectiveCamera camera(45.0f, swapchain.extent.width / (float)swapchain.extent.height, 0.1f, 10.0f);
    camera.transform.position = kk::Vec3(0.0f, 0.0f, -2.0f);

    Renderer renderer = Renderer::create(ctx, swapchain);
    while (!window.isClosed()) {
        window.pollEvents();
        handleKey(GLFW_KEY_W, glfwGetKey(static_cast<GLFWwindow*>(window.acquireHandle()), GLFW_KEY_W), camera);
        handleKey(GLFW_KEY_A, glfwGetKey(static_cast<GLFWwindow*>(window.acquireHandle()), GLFW_KEY_A), camera);
        handleKey(GLFW_KEY_S, glfwGetKey(static_cast<GLFWwindow*>(window.acquireHandle()), GLFW_KEY_S), camera);
        handleKey(GLFW_KEY_D, glfwGetKey(static_cast<GLFWwindow*>(window.acquireHandle()), GLFW_KEY_D), camera);
        if (renderer.beginFrame(ctx, swapchain)) {
            renderer.render(renderable, camera);
            renderer.endFrame(ctx, swapchain);
        }
    }

    renderable.destroy(ctx);
    triangle.destroy(ctx);
    renderer.destroy(ctx);
    swapchain.destroy(ctx);
    ctx.destroy();
    window.destroy();
}
*/
