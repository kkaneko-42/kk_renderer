#include "kk_renderer/Window.h"
#include <GLFW/glfw3.h>
#include <cassert>
#include <iostream>
#define AS_GLFW_WINDOW(win_ptr) reinterpret_cast<GLFWwindow*>(win_ptr)

using namespace kk::renderer;

static void errorCallback(int code, const char* msg) {
    std::cerr << "glfw error: code " << code << ": " << msg << std::endl;
}

std::vector<const char*> Window::getRequiredExtensions() {
    uint32_t count = 0;
    const char** raw_names = glfwGetRequiredInstanceExtensions(&count);

    std::vector<const char*> names;
    names.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        names.push_back(raw_names[i]);
    }

    return names;
}

Window Window::create(size_t width, size_t height, const std::string& name) {
    static bool is_glfw_initialized = false;
    if (!is_glfw_initialized) {
        assert(glfwInit() == GL_TRUE);
        glfwSetErrorCallback(errorCallback);

        is_glfw_initialized = true;
    }

    Window window{};
    window.name_    = name;
    window.width_   = width;
    window.height_  = height;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window.handle_  = glfwCreateWindow(
        static_cast<int>(width),
        static_cast<int>(height),
        name.c_str(),
        nullptr,
        nullptr
    );
    assert(window.handle_ != nullptr);

    return window;
}

void Window::destroy(Window& window) {
    glfwDestroyWindow(AS_GLFW_WINDOW(window.handle_));
}

bool Window::isClosed() const {
    assert(handle_ != nullptr);
    return glfwWindowShouldClose(AS_GLFW_WINDOW(handle_));
}

void Window::pollEvents() {
    glfwPollEvents();
}
