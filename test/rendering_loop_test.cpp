#include <gtest/gtest.h>
#include "kk_renderer/kk_renderer.h"

using namespace kk::renderer;

TEST(RenderingLoopTest, ContextCreation) {
    RenderingContext ctx = RenderingContext::create();
    RenderingContext::destory(ctx);
}
