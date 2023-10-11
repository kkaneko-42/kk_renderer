#include "kk_renderer/Window.h"
#include <GLFW/glfw3.h>
#include <cassert>

using namespace kk::renderer;

Window Window::create(size_t width, size_t height, const std::string& name) {
    static bool is_glfw_initialized = false;
    if (!is_glfw_initialized) {
        glfwInit();
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
    glfwDestroyWindow(reinterpret_cast<GLFWwindow*>(window.handle_));
}
