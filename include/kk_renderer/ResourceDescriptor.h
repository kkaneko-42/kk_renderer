#pragma once

#include <vulkan/vulkan.h>
#include <unordered_map>
#include <array>
#include <memory>
#include "RenderingContext.h"
#include "Buffer.h"

namespace kk {
    namespace renderer {
        // TODO: This class should manage lifecycle of VkDescriptorSetLayout, and VkDescriptorSet.
        //       In other words, vkDestroyDescriptorSetLayout() and VkFreeDescriptorSets() shouldn't be called outside this class.
        class ResourceDescriptor {
        public:
            ResourceDescriptor();
            void destroy(RenderingContext& ctx);

            void bindBuffer(
                uint32_t binding,
                const std::shared_ptr<Buffer>& buffer,
                VkDescriptorType type,
                VkShaderStageFlags stage
            );
            std::shared_ptr<Buffer> getBuffer(uint32_t binding);

            void buildLayout(RenderingContext& ctx);
            inline VkDescriptorSetLayout getLayout() { return layout_; };

            void buildSet(RenderingContext& ctx);
            inline VkDescriptorSet getSet() { return set_; };

        private:
            std::unordered_map<uint32_t, std::pair<VkDescriptorSetLayoutBinding, std::shared_ptr<void>>> resources_;
            VkDescriptorSetLayout layout_;
            VkDescriptorSet set_;
        };
    }
}
