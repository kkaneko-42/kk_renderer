#include "kk_renderer/Vertex.h"

using namespace kk::renderer;

VkVertexInputBindingDescription Vertex::getBindingDescription() {
    VkVertexInputBindingDescription binding_desc{};
    binding_desc.binding = 0;
    binding_desc.stride = sizeof(Vertex);
    binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return binding_desc;
}

std::array<VkVertexInputAttributeDescription, 3> Vertex::getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 3> attr_descs{};

    attr_descs[0].binding = 0;
    attr_descs[0].location = 0;
    attr_descs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attr_descs[0].offset = offsetof(Vertex, position);

    attr_descs[1].binding = 0;
    attr_descs[1].location = 1;
    attr_descs[1].format = VK_FORMAT_R32G32_SFLOAT;
    attr_descs[1].offset = offsetof(Vertex, uv);

    attr_descs[2].binding = 0;
    attr_descs[2].location = 2;
    attr_descs[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attr_descs[2].offset = offsetof(Vertex, color);

    return attr_descs;
}

bool kk::renderer::operator==(const Vertex& lhs, const Vertex& rhs) {
    return (
        lhs.position == rhs.position &&
        lhs.uv == rhs.uv &&
        lhs.color == rhs.color
    );
}
