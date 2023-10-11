#include "kk_renderer/SwapChain.h"
#include <GLFW/glfw3.h>
#include <cassert>

using namespace kk::renderer;

SwapChain SwapChain::create(RenderingContext& ctx, Window& window) {
    SwapChain swap_chain{};
    assert(glfwCreateWindowSurface(
        ctx.instance,
        reinterpret_cast<GLFWwindow*>(window.acquireHandle()),
        nullptr,
        &swap_chain.surface_
    ) == VK_SUCCESS);

    return swap_chain;
}

void SwapChain::destroy(RenderingContext& ctx, SwapChain& swap_chain) {
    vkDestroySurfaceKHR(ctx.instance, swap_chain.surface_, nullptr);
}
