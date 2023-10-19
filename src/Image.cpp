#include "kk_renderer/Image.h"
#include <cassert>

using namespace kk::renderer;

static void createImage(RenderingContext& ctx, Image& texture);
static void createImageView(RenderingContext& ctx, Image& texture);

Image Image::create(
    RenderingContext& ctx,
    uint32_t width,
    uint32_t height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags props,
    VkImageAspectFlags aspect
) {
    Image image{};
    image.width = width;
    image.height = height;
    image.format = format;
    image.tiling = tiling;
    image.usage = usage;
    image.props = props;
    image.aspect = aspect;

    createImage(ctx, image);
    createImageView(ctx, image);

    return image;
}

void Image::destroy(RenderingContext& ctx) {
    vkDestroyImageView(ctx.device, view, nullptr);
    vkFreeMemory(ctx.device, memory, nullptr);
    vkDestroyImage(ctx.device, image, nullptr);
}

static void createImage(RenderingContext& ctx, Image& image) {
    // Create image
    VkImageCreateInfo img_info{};
    img_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    img_info.imageType = VK_IMAGE_TYPE_2D;
    img_info.extent.width = image.width;
    img_info.extent.height = image.height;
    img_info.extent.depth = 1;
    img_info.mipLevels = 1;
    img_info.arrayLayers = 1;
    img_info.format = image.format;
    img_info.tiling = image.tiling;
    img_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    img_info.usage = image.usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    img_info.samples = VK_SAMPLE_COUNT_1_BIT;
    img_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    assert(vkCreateImage(ctx.device, &img_info, nullptr, &image.image) == VK_SUCCESS);

    // Allocate memory
    VkMemoryRequirements mem_reqs{};
    vkGetImageMemoryRequirements(ctx.device, image.image, &mem_reqs);
    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_reqs.size;
    alloc_info.memoryTypeIndex = ctx.findMemoryType(mem_reqs.memoryTypeBits, image.props);
    assert(vkAllocateMemory(ctx.device, &alloc_info, nullptr, &image.memory) == VK_SUCCESS);

    vkBindImageMemory(ctx.device, image.image, image.memory, 0);
}

static void createImageView(RenderingContext& ctx, Image& image) {
    image.view = ctx.createImageView(image.image, image.format, image.aspect);
}
