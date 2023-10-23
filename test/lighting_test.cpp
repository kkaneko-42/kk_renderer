#include <gtest/gtest.h>
#include "kk_renderer/kk_renderer.h"
#include "kk_renderer/Editor.h"
#ifndef TEST_RESOURCE_DIR
#define TEST_RESOURCE_DIR "./resources"
#endif

using namespace kk;
using namespace kk::renderer;

TEST(LightingTest, Lighting) {
    const std::pair<size_t, size_t> size = { 800, 800 };
    const std::string name = "lighting test";
    Window window = Window::create(size.first, size.second, name);
    RenderingContext ctx = RenderingContext::create();
    Swapchain swapchain = Swapchain::create(ctx, window);

    // Prepare an object
    auto triangle = std::make_shared<Geometry>(Geometry::create(ctx, TEST_RESOURCE_DIR + std::string("/models/cube.obj")));
    auto texture = std::make_shared<Texture>(Texture::create(ctx, TEST_RESOURCE_DIR + std::string("/textures/brown.jpg")));
    auto vert = std::make_shared<Shader>(Shader::create(ctx, TEST_RESOURCE_DIR + std::string("/shaders/light.vert.spv")));
    auto frag = std::make_shared<Shader>(Shader::create(ctx, TEST_RESOURCE_DIR + std::string("/shaders/light.frag.spv")));
    auto material = std::make_shared<Material>();
    material->setFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE);
    material->setTexture(texture);
    material->setVertexShader(vert);
    material->setFragmentShader(frag);
    Renderable renderable{ triangle, material };
    Transform tf;
    PerspectiveCamera camera(45.0f, swapchain.extent.width / (float)swapchain.extent.height, 0.1f, 10.0f);
    camera.transform.position.z = -5.0f;
    DirectionalLight light;

    Renderer renderer = Renderer::create(ctx, swapchain);
    Editor editor;
    editor.init(ctx, window, swapchain, renderer);
    while (!window.isClosed()) {
        window.pollEvents();
        if (renderer.beginFrame(ctx, swapchain, camera, light)) {
            renderer.render(ctx, renderable, tf);
            editor.render(renderer.getCmdBuf(), tf);
            renderer.endFrame(ctx, swapchain);
        }
    }

    vkDeviceWaitIdle(ctx.device);
    editor.terminate();
    material->destroy(ctx);
    frag->destroy(ctx);
    vert->destroy(ctx);
    texture->destroy(ctx);
    triangle->destroy(ctx);
    renderer.destroy(ctx);
    swapchain.destroy(ctx);
    ctx.destroy();
    window.destroy();
}
