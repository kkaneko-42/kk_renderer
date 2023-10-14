#pragma once

#include <vulkan/vulkan.h>
#include <unordered_map>
#include <array>
#include "RenderingContext.h"
#include "Buffer.h"

namespace kk {
    namespace renderer {
        // TODO: This class should manage lifecycle of VkDescriptorSetLayout, and VkDescriptorSet.
        //       In other words, vkDestroyDescriptorSetLayout() and VkFreeDescriptorSets() shouldn't be called outside this class.
        class ResourceDescriptor {
        public:
            ResourceDescriptor();

            void bindBuffer(uint32_t binding, const Buffer& buffer, VkDescriptorType type, VkShaderStageFlags stage);
            VkDescriptorSetLayout buildLayout(RenderingContext& ctx);
            VkDescriptorSet buildSet(RenderingContext& ctx, VkDescriptorSetLayout layout);

        private:
            std::unordered_map<uint32_t, std::pair<VkDescriptorSetLayoutBinding, const void*>> resources_;
        };
    }
}
