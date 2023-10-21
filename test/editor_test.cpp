#include <gtest/gtest.h>
#include "kk_renderer/kk_renderer.h"
#include "kk_renderer/Editor.h"
#ifndef TEST_RESOURCE_DIR
#define TEST_RESOURCE_DIR "./resources"
#endif

using namespace kk::renderer;

TEST(EditorTest, RenderingLoop) {
    const std::pair<size_t, size_t> size = { 800, 800 };
    const std::string name = "editor window test";
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

    vkDeviceWaitIdle(ctx.device);
    renderer.destroy(ctx);
    swapchain.destroy(ctx);
    ctx.destroy();
    window.destroy();
}

TEST(EditorTest, ManipulateObject) {
    const std::pair<size_t, size_t> size = { 800, 800 };
    const std::string name = "editor manipulate test";
    Window window = Window::create(size.first, size.second, name);
    RenderingContext ctx = RenderingContext::create();
    Swapchain swapchain = Swapchain::create(ctx, window);

    auto model = std::make_shared<Geometry>(Geometry::create(ctx, TEST_RESOURCE_DIR + std::string("/models/viking_room.obj")));
    auto vert = std::make_shared<Shader>(Shader::create(ctx, TEST_RESOURCE_DIR + std::string("/shaders/texture.vert.spv")));
    auto frag = std::make_shared<Shader>(Shader::create(ctx, TEST_RESOURCE_DIR + std::string("/shaders/texture.frag.spv")));
    auto texture = std::make_shared<Texture>(Texture::create(ctx, TEST_RESOURCE_DIR + std::string("/textures/viking_room.png")));
    auto material = std::make_shared<Material>();
    material->setFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE);
    material->setVertexShader(vert);
    material->setFragmentShader(frag);
    material->setTexture(texture);

    Renderable renderable{ model, material };
    Transform tf{};
    PerspectiveCamera camera(45.0f, swapchain.extent.width / (float)swapchain.extent.height, 0.1f, 10.0f);
    camera.transform.position.z = -3.0f;

    Renderer renderer = Renderer::create(ctx, swapchain);
    Editor editor;
    editor.init(ctx, window, swapchain, renderer);
    while (!window.isClosed()) {
        window.pollEvents();
        if (renderer.beginFrame(ctx, swapchain)) {
            renderer.render(ctx, renderable, tf, camera);
            editor.render(renderer.getCmdBuf(), tf);
            renderer.endFrame(ctx, swapchain);
        }
    }

    vkDeviceWaitIdle(ctx.device);
    material->destroy(ctx);
    texture->destroy(ctx);
    frag->destroy(ctx);
    vert->destroy(ctx);
    model->destroy(ctx);
    renderer.destroy(ctx);
    swapchain.destroy(ctx);
    ctx.destroy();
    window.destroy();
}
