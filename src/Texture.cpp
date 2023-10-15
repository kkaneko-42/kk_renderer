#include "kk_renderer/Texture.h"
#include "kk_renderer/Buffer.h"
#include <cassert>

using namespace kk::renderer;

static void createImage(RenderingContext& ctx, const void* texels, size_t texel_byte, Texture& texture);
static void createImageView(RenderingContext& ctx, Texture& texture);
static void createSampler(RenderingContext& ctx, Texture& texture);

Texture Texture::create(
    RenderingContext& ctx,
    const void* texels,
    size_t texel_byte,
    uint32_t width,
    uint32_t height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags props,
    VkImageAspectFlags aspect
) {
    Texture texture{};
    texture.width   = width;
    texture.height  = height;
    texture.format  = format;
    texture.tiling  = tiling;
    texture.usage   = usage;
    texture.props   = props;
    texture.aspect  = aspect;
    
    createImage(ctx, texels, texel_byte, texture);
    createImageView(ctx, texture);
    createSampler(ctx, texture);

    return texture;
}

void Texture::destroy(RenderingContext& ctx) {
    vkDeviceWaitIdle(ctx.device);

    vkDestroySampler(ctx.device, sampler, nullptr);
    vkDestroyImageView(ctx.device, view, nullptr);
    vkFreeMemory(ctx.device, memory, nullptr);
    vkDestroyImage(ctx.device, image, nullptr);
}

static void createImage(RenderingContext& ctx, const void* texels, size_t texel_byte, Texture& texture) {
    // Create image
    VkImageCreateInfo img_info{};
    img_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    img_info.imageType = VK_IMAGE_TYPE_2D;
    img_info.extent.width = texture.width;
    img_info.extent.height = texture.height;
    img_info.extent.depth = 1;
    img_info.mipLevels = 1;
    img_info.arrayLayers = 1;
    img_info.format = texture.format;
    img_info.tiling = texture.tiling;
    img_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    img_info.usage = texture.usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    img_info.samples = VK_SAMPLE_COUNT_1_BIT;
    img_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    assert(vkCreateImage(ctx.device, &img_info, nullptr, &texture.image) == VK_SUCCESS);

    // Allocate memory
    VkMemoryRequirements mem_reqs{};
    vkGetImageMemoryRequirements(ctx.device, texture.image, &mem_reqs);
    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_reqs.size;
    alloc_info.memoryTypeIndex = ctx.findMemoryType(mem_reqs.memoryTypeBits, texture.props);
    assert(vkAllocateMemory(ctx.device, &alloc_info, nullptr, &texture.memory) == VK_SUCCESS);

    vkBindImageMemory(ctx.device, texture.image, texture.memory, 0);

    // Set texels
    Buffer staging = Buffer::create(
        ctx,
        texel_byte * texture.width * texture.height,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    staging.setData(ctx, texels, staging.size);
    ctx.transitionImageLayout(texture.image, texture.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    staging.copyTo(ctx, texture, { texture.width, texture.height });
    ctx.transitionImageLayout(texture.image, texture.format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    staging.destroy(ctx);
}

static void createImageView(RenderingContext& ctx, Texture& texture) {
    texture.view = ctx.createImageView(texture.image, texture.format, texture.aspect);
}

static void createSampler(RenderingContext& ctx, Texture& texture) {
    VkSamplerCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = VK_FILTER_LINEAR;
    info.minFilter = VK_FILTER_LINEAR;
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.anisotropyEnable = VK_FALSE;
    info.maxAnisotropy = 0;
    info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    info.unnormalizedCoordinates = VK_FALSE;
    info.compareEnable = VK_FALSE;
    info.compareOp = VK_COMPARE_OP_ALWAYS;
    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    assert(vkCreateSampler(ctx.device, &info, nullptr, &texture.sampler) == VK_SUCCESS);
}
