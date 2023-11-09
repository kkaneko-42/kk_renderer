#include "kk_renderer/Texture.h"
#include "kk_renderer/Buffer.h"
#include <cassert>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <iostream>

using namespace kk::renderer;

static void createImage(RenderingContext& ctx, const void* texels, size_t texel_byte, Texture& texture);
static void createImageView(RenderingContext& ctx, Texture& texture);
static void createSampler(RenderingContext& ctx, Texture& texture);

Texture Texture::create(RenderingContext& ctx, const std::string& path) {
    int x, y, channels;
    void* texels = stbi_load(path.c_str(), &x, &y, &channels, STBI_rgb_alpha);
    if (texels == nullptr) {
        std::cerr << "Failed to load " << path << std::endl;
        assert(false);
    }

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
    stbi_image_free(texels);

    return texture;
}

Texture Texture::create(
    RenderingContext& ctx,
    const std::string& neg_x_path,
    const std::string& pos_x_path,
    const std::string& neg_y_path,
    const std::string& pos_y_path,
    const std::string& neg_z_path,
    const std::string& pos_z_path
) {
    const std::array<std::string, 6> files = {
        pos_x_path, neg_x_path, neg_y_path, pos_y_path, pos_z_path, neg_z_path
    };

    int x = 0, y = 0, ch = 0;
    std::vector<unsigned char> texels;
    for (const auto& file : files) {
        int loaded_x, loaded_y, loaded_ch;
        stbi_uc* result = stbi_load(file.c_str(), &loaded_x, &loaded_y, &loaded_ch, STBI_rgb_alpha);
        if (result == nullptr) {
            std::cerr << "Failed to open " << file << std::endl;
            assert(false);
        }
        else if (x != 0 && (loaded_x != x || loaded_y != y)) {
            std::cerr << "Error: All cube faces must be the same extent" << std::endl;
            assert(false);
        }
        x = loaded_x;
        y = loaded_y;
        ch = loaded_ch;

        for (size_t i = 0; i < static_cast<size_t>(x * y * 4); ++i) {
            texels.push_back(result[i]);
        }

        stbi_image_free(result);
    }

    Texture texture{};
    texture.width = static_cast<uint32_t>(x);
    texture.height = static_cast<uint32_t>(y);
    texture.format = VK_FORMAT_R8G8B8A8_SRGB;
    texture.array_layers = 6;
    texture.tiling = VK_IMAGE_TILING_OPTIMAL;
    texture.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    texture.props = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    texture.aspect = VK_IMAGE_ASPECT_COLOR_BIT;

    // Create image
    VkImageCreateInfo img_info{};
    img_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    img_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    img_info.imageType = VK_IMAGE_TYPE_2D;
    img_info.extent.width = texture.width;
    img_info.extent.height = texture.height;
    img_info.extent.depth = 1;
    img_info.format = texture.format;
    img_info.mipLevels = 1;
    img_info.arrayLayers = texture.array_layers;
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
    
    // Build copy regions
    std::array<VkBufferImageCopy, 6> regions{};
    for (uint32_t face = 0; face < 6; ++face) {
        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.baseArrayLayer = face;
        region.imageSubresource.layerCount = 1;
        region.imageExtent.width = texture.width;
        region.imageExtent.height = texture.height;
        region.imageExtent.depth = 1;
        region.bufferOffset = static_cast<VkDeviceSize>(face * texture.width * texture.height * 4);
        regions[face] = region;
    }

    // Set data
    Buffer staging = Buffer::create(
        ctx,
        texels.size(),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    staging.setData(ctx, texels.data(), texels.size());
    texture.transitionLayout(ctx, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    ctx.submitCmdsImmediate([&staging, &texture, &regions](VkCommandBuffer cmd_buf) {
        vkCmdCopyBufferToImage(
            cmd_buf,
            staging.buffer,
            texture.image,
            texture.layout,
            static_cast<uint32_t>(regions.size()),
            regions.data()
        );
    });
    texture.transitionLayout(ctx, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    staging.destroy(ctx);
    
    // Create image view
    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = texture.image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    view_info.format = texture.format;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 6;
    assert(vkCreateImageView(ctx.device, &view_info, nullptr, &texture.view) == VK_SUCCESS);

    createSampler(ctx, texture);

    return texture;
}

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
    texture.width       = width;
    texture.height      = height;
    texture.format      = format;
    texture.array_layers= 1;
    texture.tiling      = tiling;
    texture.usage       = usage;
    texture.props       = props;
    texture.aspect      = aspect;
    
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

void Texture::transitionLayout(RenderingContext& ctx, VkImageLayout new_layout) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = array_layers;

    VkPipelineStageFlags src_stage;
    VkPipelineStageFlags dst_stage;

    if (layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else {
        assert(false && "Unsupported layout transition");
    }

    ctx.submitCmdsImmediate([src_stage, dst_stage, &barrier](VkCommandBuffer buf) {
        vkCmdPipelineBarrier(
            buf,
            src_stage, dst_stage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    });
    layout = new_layout;
}

static void createImage(RenderingContext& ctx, const void* texels, size_t texel_byte, Texture& texture) {
    // Create image
    VkImageCreateInfo img_info{};
    img_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    img_info.imageType = VK_IMAGE_TYPE_2D;
    img_info.extent.width = texture.width;
    img_info.extent.height = texture.height;
    img_info.extent.depth = 1;
    img_info.format = texture.format;
    img_info.mipLevels = 1;
    img_info.arrayLayers = texture.array_layers;
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

    if (texels != nullptr) {
        // Set texels
        Buffer staging = Buffer::create(
            ctx,
            texel_byte * texture.width * texture.height,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        staging.setData(ctx, texels, staging.size);
        texture.transitionLayout(ctx, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        staging.copyTo(ctx, texture, { texture.width, texture.height });
        texture.transitionLayout(ctx, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        staging.destroy(ctx);
    }
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
