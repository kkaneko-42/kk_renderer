#include <gtest/gtest.h>
#include "kk_renderer/kk_renderer.h"
#ifndef TEST_RESOURCE_DIR
#define TEST_RESOURCE_DIR "./resources"
#endif

using namespace kk::renderer;

TEST(DrawModelTest, ModelCreation) {
    RenderingContext ctx = RenderingContext::create();
    Geometry geometry = Geometry::create(ctx, TEST_RESOURCE_DIR + std::string("/models/sphere.obj"));

    geometry.destroy(ctx);
    ctx.destroy();
}
