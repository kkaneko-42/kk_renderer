#include "kk_renderer/platform/GlfwWindow.h"
#include <GLFW/glfw3.h>
#include <cassert>
#include <iostream>
#include <vulkan/vulkan.h>
#define WINDOW_HANDLE static_cast<GLFWwindow*>(handle_)

using namespace kk::renderer;

bool GlfwWindow::is_glfw_initialized_ = false;

GlfwWindow::GlfwWindow(size_t width, size_t height, const std::string& name)
    : Window(width, height, name)
{

}

std::vector<const char*> GlfwWindow::getRequiredExtensions() const {
    uint32_t count = 0;
    const char** raw_names = glfwGetRequiredInstanceExtensions(&count);

    std::vector<const char*> names;
    names.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        names.push_back(raw_names[i]);
    }

    return names;
}

VkSurfaceKHR GlfwWindow::createSurface(VkInstance instance) const {
    VkSurfaceKHR surface;
    glfwCreateWindowSurface(instance, WINDOW_HANDLE, nullptr, &surface);
    return surface;
}

static void errorCallback(int code, const char* msg) {
    std::cerr << "glfw error: code " << code << ": " << msg << std::endl;
}

bool GlfwWindow::launch() {
    if (!is_glfw_initialized_) {
        if (glfwInit() != GL_TRUE) {
            return false;
        }

        glfwSetErrorCallback(errorCallback);
        is_glfw_initialized_ = true;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    handle_  = glfwCreateWindow(
        static_cast<int>(width_),
        static_cast<int>(height_),
        name_.c_str(),
        nullptr,
        nullptr
    );

    return (handle_ != nullptr);
}

void GlfwWindow::terminate() {
    glfwDestroyWindow(WINDOW_HANDLE);
    glfwTerminate();
    is_glfw_initialized_ = false;
}

bool GlfwWindow::hasError() {
    return glfwWindowShouldClose(WINDOW_HANDLE);
}

void GlfwWindow::pollEvents() {
    glfwPollEvents();
}
