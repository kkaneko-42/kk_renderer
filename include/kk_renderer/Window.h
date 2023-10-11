#pragma once

#include <string>
#include <vector>
#include <functional>


namespace kk {
    namespace renderer {
        class Window {
        public:
            static std::vector<const char*> getRequiredExtensions();
            static Window create(size_t width, size_t height, const std::string& name);
            static void destroy(Window& window);

            inline const std::string& getName() const {
                return name_;
            }

            inline std::pair<size_t, size_t> getSize() const {
                return { width_, height_ };
            }

            inline void* acquireHandle() { return handle_; }

            bool isClosed() const;
            void pollEvents();

        private:
            std::string name_;
            size_t width_, height_;
            void* handle_;
        };
    }
}
