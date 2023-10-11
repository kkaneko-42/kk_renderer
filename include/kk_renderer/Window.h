#pragma once

#include <string>
#include <functional>

namespace kk {
    namespace renderer {
        class Window {
        public:
            static Window create(size_t width, size_t height, const std::string& name);

            inline const std::string& getName() const {
                return name_;
            }

            inline std::pair<size_t, size_t> getSize() const {
                return { width_, height_ };
            }

            inline void* acquireHandle() { return handle_; }

            static void destroy(Window& window);

        private:
            std::string name_;
            size_t width_, height_;
            void* handle_;
        };
    }
}
