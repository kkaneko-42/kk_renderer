#include <gtest/gtest.h>
#include "kk_renderer/kk_renderer.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#ifndef TEST_RESOURCE_DIR
#define TEST_RESOURCE_DIR "./resources"
#endif

using namespace kk::renderer;

TEST(DrawTextureTest, TextureCreation) {
    RenderingContext ctx = RenderingContext::create();

    const std::string texture_path = TEST_RESOURCE_DIR + std::string("textures/statue.jpg");
    int x, y, channels;
    stbi_uc* texels = stbi_load((TEST_RESOURCE_DIR + std::string("textures/statue.jpg")).c_str(), &x, &y, &channels, STBI_rgb_alpha);
    if (texels == nullptr) {
        std::cerr << "Failed to load " << texture_path << std::endl;
        assert(false);
    }

    const std::pair<size_t, size_t> size = { 800, 800 };
    const std::string name = "create texture test";
    Window window = Window::create(size.first, size.second, name);
    Swapchain swapchain = Swapchain::create(ctx, window);
    Texture texture = Texture::create(
        ctx,
        texels,
        4,
        static_cast<uint32_t>(x),
        static_cast<uint32_t>(y),
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
    stbi_image_free(texels);
    renderer.destroy(ctx);
    swapchain.destroy(ctx);
    ctx.destroy();
    window.destroy();
}
