#include "kk_renderer/RenderingContext.h"
#include <iostream>
#include <set>

using namespace kk::renderer;

static VkInstance createInstance(
    const std::vector<const char*>& exts,
    const std::vector<const char*>& layers
);
static VkPhysicalDevice pickGPU(VkInstance instance, const std::vector<const char*>& exts);

RenderingContext RenderingContext::create() {
    const std::vector<const char*> instance_exts = {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    };
    const std::vector<const char*> layers = {
        "VK_LAYER_KHRONOS_validation",
    };
    const std::vector<const char*> device_exts = {};

    RenderingContext ctx{};
    ctx.instance = createInstance(instance_exts, layers);
    ctx.gpu = pickGPU(ctx.instance, device_exts);

    return ctx;
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
    info.enabledExtensionCount = exts.size();
    info.ppEnabledExtensionNames = exts.data();
    info.enabledLayerCount = layers.size();
    info.ppEnabledLayerNames = layers.data();
    VkDebugUtilsMessengerCreateInfoEXT debug_info{};
    populateDebugMessengerCreateInfo(debug_info);
    info.pNext = &debug_info;

    VkInstance instance;
    if (vkCreateInstance(&info, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create instance");
    }

    return instance;
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

    throw std::runtime_error("Suitable physical device not found");
}
