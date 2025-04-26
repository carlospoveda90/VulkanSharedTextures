#include "media/image_loader.hpp"
#include "core/vulkan_utils.hpp"
#include "tools/benchmark.hpp"

#include <stb_image.h>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <vector>

using namespace vst::vulkan_utils;

namespace vst
{

    extern PFN_vkGetMemoryFdKHR vkGetMemoryFdKHR;

    TextureImage ImageLoader::loadTexture(const std::string &path,
                                     VkDevice device,
                                     VkPhysicalDevice physicalDevice,
                                     VkCommandPool commandPool,
                                     VkQueue graphicsQueue,
                                     bool exportMemory)
    {
        int texWidth, texHeight, texChannels;
        stbi_uc *pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        if (!pixels)
        {
            throw std::runtime_error("Failed to load texture image: " + path);
        }

        VkDeviceSize imageSize = texWidth * texHeight * 4; // RGBA

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;
        createBuffer(device, physicalDevice, imageSize, stagingBuffer, stagingMemory,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        void *data;
        vkMapMemory(device, stagingMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(device, stagingMemory);

        stbi_image_free(pixels);

        TextureImage texture;
        texture.width = texWidth;
        texture.height = texHeight;

        createImage(device, physicalDevice, texWidth, texHeight, texture.image, texture.memory, exportMemory);

        transitionImageLayout(device, commandPool, graphicsQueue,
                              texture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        copyBufferToImage(device, commandPool, graphicsQueue,
                          stagingBuffer, texture.image, texWidth, texHeight);

        transitionImageLayout(device, commandPool, graphicsQueue,
                              texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        texture.view = createImageView(device, texture.image);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);

        return texture;
    }

} // namespace vst
