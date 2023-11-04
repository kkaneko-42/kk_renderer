#pragma once

#include "kk_renderer/Window.h"

namespace kk {
    namespace renderer {
        class GlfwWindow : public Window {
        public:
            GlfwWindow(size_t width, size_t height, const std::string& name);

            std::vector<const char*> getRequiredExtensions() const override;
            VkSurfaceKHR createSurface(VkInstance instance) const override;
            
            bool launch() override;
            void terminate() override;

            bool hasError() override;
            void pollEvents() override;

        private:
            static bool is_glfw_initialized_;
        };
    }
}
