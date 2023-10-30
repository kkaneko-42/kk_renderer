#include <gtest/gtest.h>
#include <glm/gtx/string_cast.hpp>
#include "kk_renderer/kk_renderer.h"
#include "kk_renderer/Editor.h"
#ifndef TEST_RESOURCE_DIR
#define TEST_RESOURCE_DIR "./resources"
#endif

using namespace kk;
using namespace kk::renderer;

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
