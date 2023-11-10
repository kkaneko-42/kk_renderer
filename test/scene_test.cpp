#include "kk_renderer/kk_renderer.h"
#include "kk_renderer/platform/GlfwWindow.h"
#include <gtest/gtest.h>

using namespace kk::renderer;

TEST(SceneTest, SceneCreation) {
    WindowPtr window = std::make_unique<GlfwWindow>(800, 800, "window");
    window->launch();
    RenderingContext ctx = RenderingContext::create(window);
    Scene scene = Scene::create(ctx);
    scene.destroy();
    ctx.destroy();
}
