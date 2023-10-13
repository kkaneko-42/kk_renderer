#include "kk_renderer/Texture.h"
#include <cassert>

using namespace kk::renderer;

Texture Texture::create(
    RenderingContext& ctx,
    uint32_t width,
    uint32_t height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags props,
    VkImageAspectFlags aspect
) {
    Texture texture{};

    // Create image
    VkImageCreateInfo img_info{};
    img_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    img_info.imageType = VK_IMAGE_TYPE_2D;
    img_info.extent.width = width;
    img_info.extent.height = height;
    img_info.extent.depth = 1;
    img_info.mipLevels = 1;
    img_info.arrayLayers = 1;
    img_info.format = format;
    img_info.tiling = tiling;
    img_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    img_info.usage = usage;
    img_info.samples = VK_SAMPLE_COUNT_1_BIT;
    img_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    assert(vkCreateImage(ctx.device, &img_info, nullptr, &texture.image) == VK_SUCCESS);

    // Allocate memory
    VkMemoryRequirements mem_reqs{};
    vkGetImageMemoryRequirements(ctx.device, texture.image, &mem_reqs);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_reqs.size;
    alloc_info.memoryTypeIndex = ctx.findMemoryType(mem_reqs.memoryTypeBits, props);
    assert(vkAllocateMemory(ctx.device, &alloc_info, nullptr, &texture.memory) == VK_SUCCESS);

    vkBindImageMemory(ctx.device, texture.image, texture.memory, 0);

    texture.view = ctx.createImageView(texture.image, format, aspect);

    return texture;
}


