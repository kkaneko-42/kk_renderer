#include "kk_renderer/Swapchain.h"
#include <GLFW/glfw3.h>
#include <cassert>
#include <set>
#include <algorithm>

using namespace kk::renderer;

template <class T>
constexpr const T& clamp(const T& val, const T& low, const T& high) {
    return (val < low) ? low : (val > high) ? high : val;
}

static void configureSettings(RenderingContext& ctx, Window& window, Swapchain& swapchain);

Swapchain Swapchain::create(RenderingContext& ctx, Window& window) {
    Swapchain swapchain{};
    assert(glfwCreateWindowSurface(
        ctx.instance,
        reinterpret_cast<GLFWwindow*>(window.acquireHandle()),
        nullptr,
        &swapchain.surface
    ) == VK_SUCCESS);

    configureSettings(ctx, window, swapchain);
    VkSwapchainCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = swapchain.surface;
    info.minImageCount = swapchain.images.size();
    info.imageFormat = swapchain.surface_format.format;
    info.imageColorSpace = swapchain.surface_format.colorSpace;
    info.imageExtent = swapchain.extent;
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    const std::vector<uint32_t> families = { ctx.graphics_family, ctx.present_family };
    if (ctx.graphics_family == ctx.present_family) {
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    else {
        info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        info.queueFamilyIndexCount = families.size();
        info.pQueueFamilyIndices = families.data();
    }

    info.preTransform = swapchain.pre_transform;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode = swapchain.present_mode;
    info.clipped = VK_TRUE;

    assert(vkCreateSwapchainKHR(ctx.device, &info, nullptr, &swapchain.swapchain) == VK_SUCCESS);

    uint32_t img_count = 0;
    vkGetSwapchainImagesKHR(ctx.device, swapchain.swapchain, &img_count, nullptr);
    swapchain.images.resize(img_count);
    vkGetSwapchainImagesKHR(ctx.device, swapchain.swapchain, &img_count, swapchain.images.data());

    return swapchain;
}

void Swapchain::destroy(RenderingContext& ctx) {
    vkDestroySwapchainKHR(ctx.device, swapchain, nullptr);
    vkDestroySurfaceKHR(ctx.instance, surface, nullptr);
}

static void configureSettings(RenderingContext& ctx, Window& window, Swapchain& swapchain) {
    // Configure format
    uint32_t format_count = 0;
    assert(vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.gpu, swapchain.surface, &format_count, nullptr) == VK_SUCCESS);
    assert(format_count != 0);
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    assert(vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.gpu, swapchain.surface, &format_count, formats.data()) == VK_SUCCESS);
    swapchain.surface_format = formats[0];

    uint32_t present_mode_count = 0;
    // Configure present mode
    assert(vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.gpu, swapchain.surface, &present_mode_count, nullptr) == VK_SUCCESS);
    assert(present_mode_count != 0);
    std::vector<VkPresentModeKHR> present_modes;
    present_modes.resize(present_mode_count);
    assert(vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.gpu, swapchain.surface, &present_mode_count, present_modes.data()) == VK_SUCCESS);
    swapchain.present_mode = present_modes[0];

    // Configure extent
    VkSurfaceCapabilitiesKHR caps{};
    assert(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.gpu, swapchain.surface, &caps) == VK_SUCCESS);
    if (caps.currentExtent.width == UINT32_MAX) {
        const std::pair<size_t, size_t> window_extent = window.getSize();
        swapchain.extent.width = clamp<uint32_t>(
            window_extent.first,
            caps.minImageExtent.width,
            caps.maxImageExtent.width
        );
        swapchain.extent.height = clamp<uint32_t>(
            window_extent.second,
            caps.minImageExtent.height,
            caps.maxImageExtent.height
        );
    }
    else {
        swapchain.extent = caps.currentExtent;
    }

    // Configure image count
    uint32_t min_img_count = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && min_img_count > caps.maxImageCount) {
        min_img_count = caps.maxImageCount;
    }
    swapchain.images.resize(min_img_count);

    // Configure pre transform
    swapchain.pre_transform = caps.currentTransform;
}
