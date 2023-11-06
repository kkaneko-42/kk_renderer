#include "kk_renderer/RenderingContext.h"
#include "kk_renderer/Window.h"
#include <iostream>
#include <set>
#include <cassert>
#include <functional>
#include <limits>

using namespace kk::renderer;

static VkInstance createInstance(
    const std::vector<const char*>& exts,
    const std::vector<const char*>& layers
);
static VkDebugUtilsMessengerEXT createDebugMessenger(VkInstance instance);
static VkPhysicalDevice pickGPU(VkInstance instance, const std::vector<const char*>& exts, VkSurfaceKHR surface);
static VkDevice createLogicalDevice(VkPhysicalDevice gpu, const std::vector<const char*>& exts, const std::set<uint32_t>& families);
static bool findQueueFamily(
    VkPhysicalDevice device,
    std::function<bool(uint32_t, const VkQueueFamilyProperties&)> cond,
    uint32_t* family
);
static VkQueue getQueue(VkDevice device, uint32_t family);
static Swapchain createSwapchain(RenderingContext& ctx, VkExtent2D window_extent);
static VkCommandPool createCommandPool(VkDevice device, uint32_t dst_queue_family);
static VkDescriptorPool createDescPool(VkDevice device);
static std::array<VkFence, kMaxConcurrentFrames> createFences(VkDevice device);
static std::array<VkSemaphore, kMaxConcurrentFrames> createSemaphores(VkDevice device);

RenderingContext RenderingContext::create(const WindowPtr& window) {
    std::vector<const char*> instance_exts = window->getRequiredExtensions();
    instance_exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    const std::vector<const char*> layers = {
        "VK_LAYER_KHRONOS_validation",
    };
    const std::vector<const char*> device_exts = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    RenderingContext ctx{};
    ctx.instance = createInstance(instance_exts, layers);
    ctx.debug_messenger = createDebugMessenger(ctx.instance);
    ctx.surface = window->createSurface(ctx.instance);
    ctx.gpu = pickGPU(ctx.instance, device_exts, ctx.surface);
    findQueueFamily(
        ctx.gpu,
        [](uint32_t i, const VkQueueFamilyProperties& prop){ return (prop.queueFlags & VK_QUEUE_GRAPHICS_BIT); },
        &ctx.graphics_family
    );
    findQueueFamily(
        ctx.gpu,
        [&ctx](uint32_t family, const VkQueueFamilyProperties& props) {
            VkBool32 is_supported = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(ctx.gpu, family, ctx.surface, &is_supported);
            return is_supported;
        },
        &ctx.present_family
    );
    ctx.device = createLogicalDevice(ctx.gpu, device_exts, { ctx.graphics_family, ctx.present_family });
    ctx.graphics_queue = getQueue(ctx.device, ctx.graphics_family);
    ctx.present_queue = getQueue(ctx.device, ctx.present_family);
    const VkExtent2D window_extent = { static_cast<uint32_t>(window->getSize().first), static_cast<uint32_t>(window->getSize().second) };
    ctx.swapchain = createSwapchain(ctx, window_extent);
    ctx.cmd_pool = createCommandPool(ctx.device, ctx.graphics_family);
    ctx.desc_pool = createDescPool(ctx.device);
    ctx.fences = createFences(ctx.device);
    ctx.present_complete = createSemaphores(ctx.device);
    ctx.render_complete = createSemaphores(ctx.device);

    return ctx;
}

void RenderingContext::destroy() {
    assert(vkDeviceWaitIdle(device) == VK_SUCCESS);

    for (size_t i = 0; i < kMaxConcurrentFrames; ++i) {
        vkDestroyFence(device, fences[i], nullptr);
        vkDestroySemaphore(device, render_complete[i], nullptr);
        vkDestroySemaphore(device, present_complete[i], nullptr);
    }

    vkDestroyDescriptorPool(device, desc_pool, nullptr);
    vkDestroyCommandPool(device, cmd_pool, nullptr);

    for (auto& view : swapchain.views) {
        vkDestroyImageView(device, view, nullptr);
    }
    vkDestroySwapchainKHR(device, swapchain.swapchain, nullptr);

    vkDestroyDevice(device, nullptr);
    auto destroyer = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (destroyer != nullptr) {
        destroyer(instance, debug_messenger, nullptr);
    }
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}

uint32_t RenderingContext::findMemoryType(uint32_t type_filter, VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(gpu, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & props) == props) {
            return i;
        }
    }

    std::cerr << "Error: Memory property " << props << " not found" << std::endl;
    assert(false);
    return UINT32_MAX;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

static VkInstance createInstance(
    const std::vector<const char*>& exts,
    const std::vector<const char*>& layers
) {
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "KK Renderer";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info.pApplicationInfo = &app_info;
    info.enabledExtensionCount = static_cast<uint32_t>(exts.size());
    info.ppEnabledExtensionNames = exts.data();
    info.enabledLayerCount = static_cast<uint32_t>(layers.size());
    info.ppEnabledLayerNames = layers.data();
    VkDebugUtilsMessengerCreateInfoEXT debug_info{};
    populateDebugMessengerCreateInfo(debug_info);
    info.pNext = &debug_info;

    VkInstance instance;
    assert(vkCreateInstance(&info, nullptr, &instance) == VK_SUCCESS);

    return instance;
}

static VkDebugUtilsMessengerEXT createDebugMessenger(VkInstance instance) {
    VkDebugUtilsMessengerCreateInfoEXT info{};
    populateDebugMessengerCreateInfo(info);

    VkDebugUtilsMessengerEXT debug_messenger{};
    auto creator = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    assert(creator != nullptr);
    creator(instance, &info, nullptr, &debug_messenger);

    return debug_messenger;
}

static bool isExtensionsSupported(VkPhysicalDevice gpu, const std::vector<const char*>& exts_required) {
    uint32_t ext_count = 0;
    vkEnumerateDeviceExtensionProperties(gpu, nullptr, &ext_count, nullptr);

    std::vector<VkExtensionProperties> exts_available(ext_count);
    vkEnumerateDeviceExtensionProperties(gpu, nullptr, &ext_count, exts_available.data());

    std::set<std::string> exts_unsupported(exts_required.begin(), exts_required.end());
    for (const auto& ext : exts_available) {
        exts_unsupported.erase(ext.extensionName);
    }

    return exts_unsupported.empty();
}

Swapchain::SupportInfo Swapchain::SupportInfo::query(VkPhysicalDevice gpu, VkSurfaceKHR surface) {
    Swapchain::SupportInfo availables;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &availables.caps);

    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &format_count, nullptr);
    if (format_count != 0) {
        availables.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &format_count, availables.formats.data());
    }

    uint32_t mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &mode_count, nullptr);
    if (mode_count != 0) {
        availables.present_modes.resize(mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &mode_count, availables.present_modes.data());
    }

    return availables;
}

static VkPhysicalDevice pickGPU(VkInstance instance, const std::vector<const char*>& exts, VkSurfaceKHR surface) {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

    for (const auto& gpu : devices) {
        if (!isExtensionsSupported(gpu, exts)) {
            continue;
        }

        const bool is_graphics_supported = findQueueFamily(
            gpu,
            [](uint32_t, const  VkQueueFamilyProperties& props) {
                return (props.queueFlags & VK_QUEUE_GRAPHICS_BIT);
            },
            nullptr
        );
        if (!is_graphics_supported) {
            continue;
        }

        const bool is_present_supported = findQueueFamily(
            gpu,
            [gpu, surface](uint32_t family, const  VkQueueFamilyProperties& props) {
                VkBool32 is_supported = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(gpu, family, surface, &is_supported);
                return is_supported;
            },
            nullptr
        );
        if (!is_present_supported) {
            continue;
        }

        const auto swapchain_support = Swapchain::SupportInfo::query(gpu, surface);
        if (swapchain_support.formats.size() == 0 ||
            swapchain_support.present_modes.size() == 0
        ) {
            continue;
        }

        return gpu;
    }

    assert(false && "Suitable physical device not found");
    return VK_NULL_HANDLE;
}

static bool findQueueFamily(
    VkPhysicalDevice device,
    std::function<bool(uint32_t, const VkQueueFamilyProperties&)> cond,
    uint32_t* family
) {
    uint32_t family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, nullptr);
    std::vector<VkQueueFamilyProperties> families(family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, families.data());

    for (uint32_t i = 0; i < families.size(); ++i) {
        if (cond(i, families[i])) {
            // Required queue family found
            if (family != nullptr) {
                *family = i;
            }
            return true;
        }
    }

    // Queue family not found
    return false;
}

static VkDevice createLogicalDevice(VkPhysicalDevice gpu, const std::vector<const char*>& exts, const std::set<uint32_t>& families) {
    VkPhysicalDeviceFeatures features{};

    std::vector<VkDeviceQueueCreateInfo> queue_infos;
    queue_infos.reserve(families.size());
    float priority = 1.0f;
    for (uint32_t family : families) {
        VkDeviceQueueCreateInfo queue_info{};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = family;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &priority;
        queue_infos.push_back(queue_info);
    }

    VkDeviceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    info.pEnabledFeatures = &features;
    info.enabledExtensionCount = static_cast<uint32_t>(exts.size());
    info.ppEnabledExtensionNames = exts.data();
    info.queueCreateInfoCount = static_cast<uint32_t>(queue_infos.size());
    info.pQueueCreateInfos = queue_infos.data();

    VkDevice device;
    assert(vkCreateDevice(gpu, &info, nullptr, &device) == VK_SUCCESS);

    return device;
}

static VkQueue getQueue(VkDevice device, uint32_t family) {
    VkQueue queue;
    vkGetDeviceQueue(device, family, 0, &queue);
    return queue;
}

template <class T>
constexpr const T& clamp(const T& v, const T& low, const T& high) {
    return ((v < low) ? low : (v > high) ? high : v);
}

static void configureSwapchainSettings(VkPhysicalDevice gpu, VkSurfaceKHR surface, VkExtent2D window_extent, Swapchain& swapchain) {
    swapchain.availables = Swapchain::SupportInfo::query(gpu, surface);
    const auto& caps = swapchain.availables.caps;
    const auto& formats = swapchain.availables.formats;
    const auto& modes = swapchain.availables.present_modes;

    // Configure format
    swapchain.format = formats[0];
    for (const auto& format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        ) {
            swapchain.format = format;
            break;
        }
    }

    // Configure present mode
    swapchain.present_mode = modes[0];
    for (const auto& mode : modes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            swapchain.present_mode = mode;
            break;
        }
    }

    // Configure extent
    if (swapchain.extent.width != std::numeric_limits<uint32_t>::max()) {
        swapchain.extent = caps.currentExtent;
    } else {
        swapchain.extent.width = clamp(
            window_extent.width,
            caps.minImageExtent.width,
            caps.maxImageExtent.width
        );
        swapchain.extent.height = clamp(
            window_extent.height,
            caps.minImageExtent.height,
            caps.maxImageExtent.height
        );
    }

    // Configure image count
    uint32_t min_img_count = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 &&
        min_img_count > caps.maxImageCount
    ) {
        min_img_count = caps.maxImageCount;
    }
    swapchain.images.resize(min_img_count);

    // Configure pre transform
    swapchain.pre_transform = caps.currentTransform;
}

static Swapchain createSwapchain(RenderingContext& ctx, VkExtent2D window_extent) {
    Swapchain swapchain;
    configureSwapchainSettings(ctx.gpu, ctx.surface, window_extent, swapchain);

    VkSwapchainCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = ctx.surface;
    info.minImageCount = static_cast<uint32_t>(swapchain.images.size());
    info.imageFormat = swapchain.format.format;
    info.imageColorSpace = swapchain.format.colorSpace;
    info.imageExtent = swapchain.extent;
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    const std::vector<uint32_t> families = { ctx.graphics_family, ctx.present_family };
    if (ctx.graphics_family == ctx.present_family) {
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    else {
        info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        info.queueFamilyIndexCount = static_cast<uint32_t>(families.size());
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

    swapchain.views.resize(img_count);
    for (uint32_t i = 0; i < img_count; ++i) {
        swapchain.views[i] = ctx.createImageView(swapchain.images[i], swapchain.format.format, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    return swapchain;
}

static VkCommandPool createCommandPool(VkDevice device, uint32_t dst_queue_family) {
    VkCommandPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    info.queueFamilyIndex = dst_queue_family;

    VkCommandPool pool;
    assert(vkCreateCommandPool(device, &info, nullptr, &pool) == VK_SUCCESS);
    return pool;
}

static VkDescriptorPool createDescPool(VkDevice device) {
    std::array<VkDescriptorPoolSize, 2> pool_sizes{};
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[0].descriptorCount = static_cast<uint32_t>(kMaxConcurrentFrames) * 256;
    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes[1].descriptorCount = pool_sizes[0].descriptorCount;

    VkDescriptorPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    info.pPoolSizes = pool_sizes.data();
    info.maxSets = static_cast<uint32_t>(kMaxConcurrentFrames) * 256; // TODO: Set max number of objects

    VkDescriptorPool pool;
    assert(vkCreateDescriptorPool(device, &info, nullptr, &pool) == VK_SUCCESS);
    return pool;
}

static std::array<VkFence, kMaxConcurrentFrames> createFences(VkDevice device) {
    VkFenceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    std::array<VkFence, kMaxConcurrentFrames> fences;
    for (size_t i = 0; i < kMaxConcurrentFrames; ++i) {
        assert(vkCreateFence(device, &info, nullptr, &fences[i]) == VK_SUCCESS);
    }

    return fences;
}

static std::array<VkSemaphore, kMaxConcurrentFrames> createSemaphores(VkDevice device) {
    VkSemaphoreCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    std::array<VkSemaphore, kMaxConcurrentFrames> semaphores;
    for (size_t i = 0; i < kMaxConcurrentFrames; ++i) {
        assert(vkCreateSemaphore(device, &info, nullptr, &semaphores[i]) == VK_SUCCESS);
    }

    return semaphores;
}

VkImageView RenderingContext::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

void RenderingContext::submitCmdsImmediate(std::function<void(VkCommandBuffer)> cmds_recorder) {
    // Create command buffer
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = cmd_pool;
    alloc_info.commandBufferCount = 1;
    VkCommandBuffer cmd_buf;
    vkAllocateCommandBuffers(device, &alloc_info, &cmd_buf);

    // Build commands
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    assert(vkBeginCommandBuffer(cmd_buf, &begin_info) == VK_SUCCESS);
    cmds_recorder(cmd_buf);
    assert(vkEndCommandBuffer(cmd_buf) == VK_SUCCESS);

    // Submit commands
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd_buf;
    assert(vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE) == VK_SUCCESS);
    assert(vkQueueWaitIdle(graphics_queue) == VK_SUCCESS);

    vkFreeCommandBuffers(device, cmd_pool, 1, &cmd_buf);
}

void RenderingContext::transitionImageLayout(
    VkImage image,
    VkFormat format,
    VkImageLayout old_layout,
    VkImageLayout new_layout
) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags src_stage;
    VkPipelineStageFlags dst_stage;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else {
        assert(false && "Unsupported layout transition");
    }

    submitCmdsImmediate([src_stage, dst_stage, &barrier](VkCommandBuffer buf) {
        vkCmdPipelineBarrier(
            buf,
            src_stage, dst_stage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    });
}
