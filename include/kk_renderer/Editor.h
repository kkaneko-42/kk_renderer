#pragma once

#include "RenderingContext.h"
#include <memory>

namespace kk {
    namespace renderer {
        struct EditorImpl;
        class Editor {
        public:
            Editor();

            void init(RenderingContext& ctx);

        private:
            std::unique_ptr<EditorImpl> impl_;
        };
    }
}
