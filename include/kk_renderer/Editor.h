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
            void update(VkCommandBuffer cmd_buf, VkFramebuffer framebuffer, void* window, Transform& model, Camera& camera);
            void terminate();

        private:
            EditorImpl* impl_;
        };
    }
}
