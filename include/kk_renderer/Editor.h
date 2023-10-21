#pragma once

#include "RenderingContext.h"
#include "Renderer.h"
#include "Swapchain.h"
#include <memory>

namespace kk {
    namespace renderer {
        struct EditorImpl;
        class Editor {
        public:
            Editor();

            void init(RenderingContext& ctx, Window& window, const Swapchain& swapchain, const Renderer& renderer);
            void render(VkCommandBuffer cmd_buf);
            void render(VkCommandBuffer cmd_buf, Transform& transform);

        private:
            EditorImpl* impl_;
        };
    }
}
