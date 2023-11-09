#include <gtest/gtest.h>
#include <glm/gtx/string_cast.hpp>
#include "kk_renderer/kk_renderer.h"
#include "kk_renderer/platform/GlfwWindow.h"
#include "kk_renderer/Editor.h"
#ifndef TEST_RESOURCE_DIR
#define TEST_RESOURCE_DIR "./resources"
#endif

using namespace kk;
using namespace kk::renderer;

static Quat angleAxis(float rad, const Vec3& axis) {
    return Quat(
        std::cos(rad / 2.0f),
        axis.x * std::sin(rad / 2.0f),
        axis.y * std::sin(rad / 2.0f),
        axis.z * std::sin(rad / 2.0f)
    );
}
/*
TEST(LightingTest, Lighting) {
    const std::pair<size_t, size_t> size = { 800, 800 };
    const std::string name = "lighting test";
    Window window = Window::create(size.first, size.second, name);
    RenderingContext ctx = RenderingContext::create();
    Swapchain swapchain = Swapchain::create(ctx, window);

    // Prepare an object
    auto triangle = std::make_shared<Geometry>(Geometry::create(ctx, TEST_RESOURCE_DIR + std::string("/models/sphere.obj")));
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
    light.transform.position = Vec3(0, -5, -5);
    light.transform.rotation = angleAxis(3.1415f / 3.0f, Vec3(-1, 0, 0));

    Renderer renderer = Renderer::create(ctx, swapchain);
    Editor editor;
    editor.init(ctx, window, swapchain, renderer);
    while (!window.isClosed()) {
        window.pollEvents();
        if (renderer.beginFrame(ctx, swapchain, camera, light)) {
            renderer.render(ctx, renderable, tf);
            editor.update(renderer.getCmdBuf(), window.acquireHandle(), tf, camera);
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
*/

TEST(LightingTest, Shadow) {
    WindowPtr window = std::make_unique<GlfwWindow>(800, 800, "ctx creation");
    window->launch();
    RenderingContext ctx = RenderingContext::create(window);
    Renderer renderer = Renderer::create(ctx);

    // Prepare an object
    auto sphere = std::make_shared<Geometry>(Geometry::create(ctx, TEST_RESOURCE_DIR + std::string("/models/sphere.obj")));
    auto plane = std::make_shared<Geometry>(Geometry::create(ctx, TEST_RESOURCE_DIR + std::string("/models/plane.obj")));
    auto cube = std::make_shared<Geometry>(Geometry::create(ctx, TEST_RESOURCE_DIR + std::string("/models/cube.obj")));
    auto brown = std::make_shared<Texture>(Texture::create(ctx, TEST_RESOURCE_DIR + std::string("/textures/brown.jpg")));
    auto white = std::make_shared<Texture>(Texture::create(ctx, TEST_RESOURCE_DIR + std::string("/textures/white.jpg")));
    auto vert = std::make_shared<Shader>(Shader::create(ctx, TEST_RESOURCE_DIR + std::string("/shaders/shadow_light.vert.spv")));
    auto frag = std::make_shared<Shader>(Shader::create(ctx, TEST_RESOURCE_DIR + std::string("/shaders/shadow_light.frag.spv")));
    auto sphere_mat = std::make_shared<Material>();
    sphere_mat->setTexture("texSampler", brown);
    sphere_mat->setVertexShader(vert);
    sphere_mat->setFragmentShader(frag);
    auto plane_mat = std::make_shared<Material>();
    plane_mat->setTexture("texSampler", white);
    plane_mat->setVertexShader(vert);
    plane_mat->setFragmentShader(frag);

    Renderable sphere_obj{ sphere, sphere_mat };
    // sphere_obj.transform.position.x = 1.0f;
    // sphere_obj.transform.position.z = 2.0f;

    Renderable sphere_2{ sphere, sphere_mat };
    sphere_2.transform.position = Vec3(-1, -3, -4);

    Renderable cube_obj{ cube, sphere_mat };

    Renderable plane_obj{ plane, plane_mat };
    plane_obj.transform.position.y = 1.5f;
    plane_obj.transform.rotation = angleAxis(glm::radians(180.0f), Vec3(1, 0, 0));
    plane_obj.transform.scale = Vec3(4.0f);

    PerspectiveCamera camera(45.0f, ctx.swapchain.extent.width / (float)ctx.swapchain.extent.height, 0.1f, 100.0f);
    camera.transform.position.z = -5.0f;
    DirectionalLight light;
    light.transform.position = Vec3(0.0f, -10.0f, -10.0f);
    light.transform.rotation = angleAxis(3.1415f / 4.0f, Vec3(-1, 0, 0));

    std::vector<Renderable> scene = { plane_obj, sphere_obj, sphere_2, cube_obj };

    Editor editor;
    editor.init(ctx, window);
    while (!window->hasError()) {
        window->pollEvents();
        if (renderer.beginFrame(ctx)) {
            renderer.render(ctx, scene, light, camera);
            editor.update(renderer.getCmdBuf(), renderer.getFramebuf(), window->acquireHandle(), scene[3].transform, camera);
            renderer.endFrame(ctx);
        }
    }

    vkDeviceWaitIdle(ctx.device);
    editor.terminate(ctx);
    plane_mat->destroy(ctx);
    sphere_mat->destroy(ctx);
    frag->destroy(ctx);
    vert->destroy(ctx);
    brown->destroy(ctx);
    white->destroy(ctx);
    cube->destroy(ctx);
    sphere->destroy(ctx);
    plane->destroy(ctx);
    renderer.destroy(ctx);
    ctx.destroy();
    window->terminate();
}
