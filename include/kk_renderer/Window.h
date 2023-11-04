#pragma once

#include <string>
#include <vector>
#include <memory>
#include <vulkan/vulkan.h> // FIXME: vulkan dependency(VkSurfaceKHR, VkInstance)

namespace kk {
    namespace renderer {
        class Window {
        public:
            explicit Window(size_t width, size_t height, const std::string& name)
                : width_(width), height_(height), name_(name) {}
            virtual ~Window() {}

            virtual std::vector<const char*> getRequiredExtensions() const = 0;
            virtual VkSurfaceKHR createSurface(VkInstance instance) const = 0;
            virtual bool launch() = 0;
            virtual void terminate() = 0;

            inline const std::string& getName() const {
                return name_;
            }

            inline std::pair<size_t, size_t> getSize() const {
                return { width_, height_ };
            }

            inline void* acquireHandle() { return handle_; }

            virtual bool hasError() = 0;
            virtual void pollEvents() = 0;

        protected:
            std::string name_;
            size_t width_, height_;
            void* handle_;
        };

        using WindowPtr = std::unique_ptr<Window>;
    }
}
