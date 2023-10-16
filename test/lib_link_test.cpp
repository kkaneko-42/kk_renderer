#include <gtest/gtest.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>

TEST(LibLinkTest, LibLinkTest) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::cout << extensionCount << " extensions supported\n";

    size_t i = 0;
    while(!glfwWindowShouldClose(window) && i < 10) {
        glfwPollEvents();
        ++i;
    }

    glfwDestroyWindow(window);

    glfwTerminate();
}
