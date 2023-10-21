#include <gtest/gtest.h>
#include "kk_renderer/kk_renderer.h"
#ifndef TEST_RESOURCE_DIR
#define TEST_RESOURCE_DIR "./resources"
#endif

using namespace kk::renderer;

const std::vector<Vertex> kVertices = {
    {{-0.5f, -0.5f, 0.0f}, {}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
    {{0.5f, -0.5f, 0.0f}, {}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
    {{0.5f, 0.5f, 0.0f}, {}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}
};

const std::vector<uint32_t> kIndices = {
    0, 1, 2, 2, 3, 0
};

TEST(DrawTextureTest, TextureCreation) {
    const std::pair<size_t, size_t> size = { 800, 800 };
    const std::string name = "create texture test";
    Window window = Window::create(size.first, size.second, name);

    RenderingContext ctx = RenderingContext::create();
    Swapchain swapchain = Swapchain::create(ctx, window);

    auto vert = std::make_shared<Shader>(Shader::create(ctx, TEST_RESOURCE_DIR + std::string("/shaders/texture.vert.spv")));
    auto frag = std::make_shared<Shader>(Shader::create(ctx, TEST_RESOURCE_DIR + std::string("/shaders/texture.frag.spv")));
    auto texture = std::make_shared<Texture>(Texture::create(ctx, TEST_RESOURCE_DIR + std::string("/textures/statue.jpg")));
    auto material = std::make_shared<Material>();
    material->setVertexShader(vert);
    material->setFragmentShader(frag);
    material->setTexture(texture);

    Renderer renderer = Renderer::create(ctx, swapchain);
    renderer.compileMaterial(ctx, material);

    material->destroy(ctx);
    texture->destroy(ctx);
    frag->destroy(ctx);
    vert->destroy(ctx);
    renderer.destroy(ctx);
    swapchain.destroy(ctx);
    ctx.destroy();
    window.destroy();
}

TEST(DrawTextureTest, TextureDrawing) {
    const std::pair<size_t, size_t> size = { 800, 800 };
    const std::string name = "draw texture test";
    Window window = Window::create(size.first, size.second, name);

    RenderingContext ctx = RenderingContext::create();
    Swapchain swapchain = Swapchain::create(ctx, window);

    auto rect = std::make_shared<Geometry>(Geometry::create(ctx, kVertices, kIndices));
    auto vert = std::make_shared<Shader>(Shader::create(ctx, TEST_RESOURCE_DIR + std::string("/shaders/texture.vert.spv")));
    auto frag = std::make_shared<Shader>(Shader::create(ctx, TEST_RESOURCE_DIR + std::string("/shaders/texture.frag.spv")));
    auto texture = std::make_shared<Texture>(Texture::create(ctx, TEST_RESOURCE_DIR + std::string("/textures/statue.jpg")));
    auto material = std::make_shared<Material>();
    material->setVertexShader(vert);
    material->setFragmentShader(frag);
    material->setTexture(texture);

    Renderable renderable{ rect, material };
    Transform tf{};
    PerspectiveCamera camera(45.0f, swapchain.extent.width / (float)swapchain.extent.height, 0.1f, 10.0f);
    camera.transform.position.z = -2.0f;

    Renderer renderer = Renderer::create(ctx, swapchain);
    while (!window.isClosed()) {
        window.pollEvents();
        if (renderer.beginFrame(ctx, swapchain)) {
            renderer.render(ctx, renderable, tf, camera);
            renderer.endFrame(ctx, swapchain);
        }
    }

    vkDeviceWaitIdle(ctx.device);
    material->destroy(ctx);
    texture->destroy(ctx);
    frag->destroy(ctx);
    vert->destroy(ctx);
    rect->destroy(ctx);
    renderer.destroy(ctx);
    swapchain.destroy(ctx);
    ctx.destroy();
    window.destroy();
}
