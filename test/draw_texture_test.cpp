#include <gtest/gtest.h>
#include "kk_renderer/kk_renderer.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#ifndef TEST_RESOURCE_DIR
#define TEST_RESOURCE_DIR "./resources"
#endif

using namespace kk::renderer;
/*
class DrawTextureTest : public ::testing::Test {
protected:
    static void SetUpTestCase() {
        const std::string texture_path = TEST_RESOURCE_DIR + std::string("textures/statue.jpg");
        texels_ = stbi_load((TEST_RESOURCE_DIR + std::string("textures/statue.jpg")).c_str(), &x_, &y_, &channels_, STBI_rgb_alpha);
        if (texels_ == nullptr) {
            std::cerr << "Failed to load " << texture_path << std::endl;
            assert(false);
        }
    }

    static void TearDownTestCase() {
        stbi_image_free(texels_);
    }

    static stbi_uc* texels_;
    static int x_, y_, channels_;
};

stbi_uc* DrawTextureTest::texels_ = nullptr;
int DrawTextureTest::x_ = 0;
int DrawTextureTest::y_ = 0;
int DrawTextureTest::channels_ = 0;

const std::vector<Vertex> kVertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}
};

const std::vector<uint32_t> kIndices = {
    0, 1, 2, 2, 3, 0
};

TEST_F(DrawTextureTest, TextureCreation) {
    const std::pair<size_t, size_t> size = { 800, 800 };
    const std::string name = "create texture test";
    Window window = Window::create(size.first, size.second, name);

    RenderingContext ctx = RenderingContext::create();
    Swapchain swapchain = Swapchain::create(ctx, window);
    Texture texture = Texture::create(
        ctx,
        texels_,
        4,
        static_cast<uint32_t>(x_),
        static_cast<uint32_t>(y_),
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    );

    Renderer renderer = Renderer::create(ctx, swapchain);
    while (!window.isClosed()) {
        window.pollEvents();
        if (renderer.beginFrame(ctx, swapchain)) {
            renderer.endFrame(ctx, swapchain);
        }
    }

    texture.destroy(ctx);
    renderer.destroy(ctx);
    swapchain.destroy(ctx);
    ctx.destroy();
    window.destroy();
}

TEST_F(DrawTextureTest, TextureDrawing) {
    const std::pair<size_t, size_t> size = { 800, 800 };
    const std::string name = "create texture test";
    Window window = Window::create(size.first, size.second, name);

    RenderingContext ctx = RenderingContext::create();
    Swapchain swapchain = Swapchain::create(ctx, window);
    Texture texture = Texture::create(
        ctx,
        texels_,
        4,
        static_cast<uint32_t>(x_),
        static_cast<uint32_t>(y_),
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    );
    Geometry geometry = Geometry::create(ctx, kVertices, kIndices);
    Renderable renderable = Renderable::create(ctx, geometry, std::make_shared<Texture>(texture));

    Renderer renderer = Renderer::create(ctx, swapchain);
    while (!window.isClosed()) {
        window.pollEvents();
        if (renderer.beginFrame(ctx, swapchain)) {
            renderer.render(renderable);
            renderer.endFrame(ctx, swapchain);
        }
    }

    vkDeviceWaitIdle(ctx.device);
    renderable.destroy(ctx);
    geometry.destroy(ctx);
    texture.destroy(ctx);
    renderer.destroy(ctx);
    swapchain.destroy(ctx);
    ctx.destroy();
    window.destroy();

}
*/
