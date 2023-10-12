#include "kk_renderer/RenderingContext.h"
#include "kk_renderer/Window.h"
#include <iostream>
#include <set>
#include <cassert>
#include <functional>

using namespace kk::renderer;

static VkInstance createInstance(
    const std::vector<const char*>& exts,
    const std::vector<const char*>& layers
);
static VkDebugUtilsMessengerEXT createDebugMessenger(VkInstance instance);
static VkPhysicalDevice pickGPU(VkInstance instance, const std::vector<const char*>& exts);
static VkDevice createLogicalDevice(VkPhysicalDevice gpu, const std::vector<const char*>& exts, const std::set<uint32_t>& families);
static uint32_t findQueueFamily(
    VkPhysicalDevice device,
    std::function<bool(uint32_t, const VkQueueFamilyProperties&)> cond
);
static VkQueue getQueue(VkDevice device, uint32_t family);
static VkCommandPool createCommandPool(VkDevice device, uint32_t dst_queue_family);

RenderingContext RenderingContext::create() {
    std::vector<const char*> instance_exts = Window::getRequiredExtensions();
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
    ctx.gpu = pickGPU(ctx.instance, device_exts);
    ctx.graphics_family = findQueueFamily(ctx.gpu, [](uint32_t i, const VkQueueFamilyProperties& prop) {
        return (prop.queueFlags & VK_QUEUE_GRAPHICS_BIT);
    });
    ctx.present_family = ctx.graphics_family; // CONCERN
    ctx.device = createLogicalDevice(ctx.gpu, device_exts, { ctx.graphics_family, ctx.present_family });
    ctx.graphics_queue = getQueue(ctx.device, ctx.graphics_family);
    ctx.present_queue = getQueue(ctx.device, ctx.present_family);
    ctx.cmd_pool = createCommandPool(ctx.device, ctx.graphics_family);

    return ctx;
}

void RenderingContext::destroy() {
    vkDestroyCommandPool(device, cmd_pool, nullptr);
    vkDestroyDevice(device, nullptr);
    auto destroyer = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (destroyer != nullptr) {
        destroyer(instance, debug_messenger, nullptr);
    }
    vkDestroyInstance(instance, nullptr);
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

static VkPhysicalDevice pickGPU(VkInstance instance, const std::vector<const char*>& exts) {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

    for (const auto& gpu : devices) {
        if (isExtensionsSupported(gpu, exts)) {
            return gpu;
        }
    }

    assert(false && "Suitable physical device not found");
    return VK_NULL_HANDLE;
}

static uint32_t findQueueFamily(
    VkPhysicalDevice device,
    std::function<bool(uint32_t, const VkQueueFamilyProperties&)> cond
) {
    uint32_t family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, nullptr);
    std::vector<VkQueueFamilyProperties> families(family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, families.data());

    for (uint32_t i = 0; i < families.size(); ++i) {
        if (cond(i, families[i])) {
            return i;
        }
    }

    assert(false && "Device queue required is unsupported");
    return UINT32_MAX;
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

static VkCommandPool createCommandPool(VkDevice device, uint32_t dst_queue_family) {
    VkCommandPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    info.queueFamilyIndex = dst_queue_family;

    VkCommandPool pool;
    assert(vkCreateCommandPool(device, &info, nullptr, &pool) == VK_SUCCESS);
    return pool;
}
