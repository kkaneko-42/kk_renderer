#include "kk_renderer/RenderingContext.h"
#include <iostream>

using namespace kk::renderer;

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

    return ctx;
}

VkInstance RenderingContext::createInstance(
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
        throw std::runtime_error("failed to create instance");
    }

    return instance;
}

void RenderingContext::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = &RenderingContext::debugCallback;
}

VKAPI_ATTR VkBool32 VKAPI_CALL RenderingContext::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}
