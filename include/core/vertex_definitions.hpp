#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>
#include <vector>

namespace vst
{
    struct Vertex
    {
        glm::vec2 pos;
        glm::vec2 texCoord;
        

        static VkVertexInputBindingDescription getBindingDescription()
        {
            VkVertexInputBindingDescription binding{};
            binding.binding = 0;
            binding.stride = sizeof(Vertex);
            binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            return binding;
        }

        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
        {
            std::array<VkVertexInputAttributeDescription, 2> attributes{};

            attributes[0].binding = 0;
            attributes[0].location = 0;
            attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
            attributes[0].offset = offsetof(Vertex, pos);

            attributes[1].binding = 0;
            attributes[1].location = 1;
            attributes[1].format = VK_FORMAT_R32G32_SFLOAT;
            attributes[1].offset = offsetof(Vertex, texCoord);

            return attributes;
        }
    };

    extern const std::vector<Vertex> FULLSCREEN_QUAD;

} // namespace vst
