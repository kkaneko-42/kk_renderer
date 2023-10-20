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

        private:
            EditorImpl* impl_;
        };
    }
}
