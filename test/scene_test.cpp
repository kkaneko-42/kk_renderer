#include "kk_renderer/kk_renderer.h"
#include "kk_renderer/platform/GlfwWindow.h"
#include <gtest/gtest.h>
#ifndef TEST_RESOURCE_DIR
#define TEST_RESOURCE_DIR "./resources"
#endif

using namespace kk::renderer;
using namespace kk;

static Quat angleAxis(float rad, const Vec3& axis) {
    return Quat(
        std::cos(rad / 2.0f),
        axis.x * std::sin(rad / 2.0f),
        axis.y * std::sin(rad / 2.0f),
        axis.z * std::sin(rad / 2.0f)
    );
}

TEST(SceneTest, SceneCreation) {
    WindowPtr window = std::make_unique<GlfwWindow>(800, 800, "window");
    window->launch();
    RenderingContext ctx = RenderingContext::create(window);
    Scene scene = Scene::create(ctx);
    scene.destroy();
    ctx.destroy();
}

TEST(SceneTest, Rendering) {
    WindowPtr window = std::make_unique<GlfwWindow>(800, 800, "window");
    window->launch();
    RenderingContext ctx = RenderingContext::create(window);
    Scene scene = Scene::create(ctx);

    auto vert = std::make_shared<Shader>(Shader::create(ctx, TEST_RESOURCE_DIR + std::string("/shaders/shadow_light.vert.spv")));
    auto frag = std::make_shared<Shader>(Shader::create(ctx, TEST_RESOURCE_DIR + std::string("/shaders/shadow_light.frag.spv")));
    auto brown = std::make_shared<Texture>(Texture::create(ctx, TEST_RESOURCE_DIR + std::string("/textures/brown.jpg")));

    Renderable cube;
    cube.geometry = std::make_shared<Geometry>(Geometry::create(ctx, TEST_RESOURCE_DIR + std::string("/models/cube.obj")));
    cube.material = std::make_shared<Material>();
    cube.material->setVertexShader(vert);
    cube.material->setFragmentShader(frag);
    cube.material->setTexture("texSampler", brown);
    scene.addObject(cube);

    DirectionalLight light;
    light.transform.position = Vec3(0.0f, -10.0f, -10.0f);
    light.transform.rotation = angleAxis(3.1415f / 4.0f, Vec3(-1, 0, 0));
    scene.setLight(light);

    PerspectiveCamera camera(45.0f, ctx.swapchain.extent.width / (float)ctx.swapchain.extent.height, 0.1f, 100.0f);
    camera.transform.position.z = -5.0f;

    Renderer renderer = Renderer::create(ctx);
    while (!window->hasError()) {
        renderer.beginFrame(ctx);
        renderer.render(ctx, scene, camera);
        renderer.endFrame(ctx);
    }

    scene.destroy();
    ctx.destroy();
}
