#include <gtest/gtest.h>
#include <glm/gtx/string_cast.hpp>
#include "kk_renderer/kk_renderer.h"
#include "kk_renderer/Editor.h"
#ifndef TEST_RESOURCE_DIR
#define TEST_RESOURCE_DIR "./resources"
#endif

using namespace kk;
using namespace kk::renderer;
/*
TEST(SkyboxTest, CubmapCreation) {
    const std::pair<size_t, size_t> size = { 800, 800 };
    const std::string name = "lighting test";
    Window window = Window::create(size.first, size.second, name);
    RenderingContext ctx = RenderingContext::create();
    Swapchain swapchain = Swapchain::create(ctx, window);

    Texture cubemap = Texture::create(
        ctx,
        TEST_RESOURCE_DIR + std::string("/textures/skybox/left.jpg"),
        TEST_RESOURCE_DIR + std::string("/textures/skybox/right.jpg"),
        TEST_RESOURCE_DIR + std::string("/textures/skybox/top.jpg"),
        TEST_RESOURCE_DIR + std::string("/textures/skybox/bottom.jpg"),
        TEST_RESOURCE_DIR + std::string("/textures/skybox/back.jpg"),
        TEST_RESOURCE_DIR + std::string("/textures/skybox/front.jpg")
    );

    Renderer renderer = Renderer::create(ctx, swapchain);
    while (true) {
        renderer.beginFrame(ctx, swapchain);
        renderer.endFrame(ctx, swapchain);
    }

    cubemap.destroy(ctx);
    ctx.destroy();
}
*/

TEST(SkyboxTest, SkyboxRendering) {
    const std::pair<size_t, size_t> size = { 800, 800 };
    const std::string name = "lighting test";
    Window window = Window::create(size.first, size.second, name);
    RenderingContext ctx = RenderingContext::create();
    Swapchain swapchain = Swapchain::create(ctx, window);

    auto skybox_geometry = std::make_shared<Geometry>(Geometry::create(ctx, TEST_RESOURCE_DIR + std::string("/models/cube.obj")));
    auto skybox_tex = std::make_shared<Texture>(Texture::create(
        ctx,
        TEST_RESOURCE_DIR + std::string("/textures/skybox/left.jpg"),
        TEST_RESOURCE_DIR + std::string("/textures/skybox/right.jpg"),
        TEST_RESOURCE_DIR + std::string("/textures/skybox/top.jpg"),
        TEST_RESOURCE_DIR + std::string("/textures/skybox/bottom.jpg"),
        TEST_RESOURCE_DIR + std::string("/textures/skybox/back.jpg"),
        TEST_RESOURCE_DIR + std::string("/textures/skybox/front.jpg")
    ));
    auto skybox_vert = std::make_shared<Shader>(Shader::create(
        ctx,
        TEST_RESOURCE_DIR + std::string("/shaders/skybox.vert.spv")
    ));
    auto skybox_frag = std::make_shared<Shader>(Shader::create(
        ctx,
        TEST_RESOURCE_DIR + std::string("/shaders/skybox.frag.spv")
    ));
    auto skybox_mat = std::make_shared<Material>();
    skybox_mat->setTexture(skybox_tex);
    skybox_mat->setVertexShader(skybox_vert);
    skybox_mat->setFragmentShader(skybox_frag);
    skybox_mat->setFrontFace(VK_FRONT_FACE_CLOCKWISE);
    skybox_mat->setDepthCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL);

    auto sphere_geometry = std::make_shared<Geometry>(Geometry::create(ctx, TEST_RESOURCE_DIR + std::string("/models/sphere.obj")));
    auto brown = std::make_shared<Texture>(Texture::create(ctx, TEST_RESOURCE_DIR + std::string("/textures/brown.jpg")));
    auto sphere_vert = std::make_shared<Shader>(Shader::create(ctx, TEST_RESOURCE_DIR + std::string("/shaders/shadow_light.vert.spv")));
    auto sphere_frag = std::make_shared<Shader>(Shader::create(ctx, TEST_RESOURCE_DIR + std::string("/shaders/shadow_light.frag.spv")));
    auto sphere_mat = std::make_shared<Material>();
    sphere_mat->setTexture(brown);
    sphere_mat->setVertexShader(sphere_vert);
    sphere_mat->setFragmentShader(sphere_frag);

    Renderable skybox;
    skybox.geometry = skybox_geometry;
    skybox.material = skybox_mat;
    Renderable sphere;
    sphere.geometry = sphere_geometry;
    sphere.material = sphere_mat;
    std::vector<Renderable> scene = { skybox, sphere };

    DirectionalLight light;
    PerspectiveCamera camera(45.0f, swapchain.extent.width / (float)swapchain.extent.height, 0.1f, 100.0f);
    camera.transform.position.z = -5.0f;

    Renderer renderer = Renderer::create(ctx, swapchain);
    Editor editor;
    editor.init(ctx, window, swapchain, renderer);
    while (!window.isClosed()) {
        window.pollEvents();
        if (renderer.beginFrame(ctx, swapchain)) {
            renderer.render(ctx, scene, light, camera, swapchain);
            editor.update(renderer.getCmdBuf(), renderer.getFramebuf(), window.acquireHandle(), scene[1].transform, camera);
            renderer.endFrame(ctx, swapchain);
        }
    }

    vkDeviceWaitIdle(ctx.device);
    editor.terminate(ctx);
    renderer.destroy(ctx);

}
