#include "media/texture_image.hpp"
#include "core/vulkan_utils.hpp"

#include <stdexcept>
#include <cstring>

namespace vst
{

    using namespace vst::vulkan_utils;

    TextureImage::TextureImage()
        : image(VK_NULL_HANDLE),
          memory(VK_NULL_HANDLE),
          view(VK_NULL_HANDLE),
          width(0),
          height(0)
    {
    }

    void TextureImage::updateFromRawData(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        const uint8_t *newData)
    {

        if (!newData)
        {
            throw std::runtime_error("updateFromRawData: input data is null");
        }
        if (width <= 0 || height <= 0)
        {
            throw std::runtime_error("updateFromRawData: invalid texture dimensions");
        }
        if (image == VK_NULL_HANDLE || memory == VK_NULL_HANDLE)
            throw std::runtime_error("Texture not initialized.");

        VkDeviceSize imageSize = static_cast<VkDeviceSize>(width * height * 4); // assuming RGBA8

        // Create temporary staging buffer
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;
        createBuffer(device, physicalDevice, imageSize, stagingBuffer, stagingMemory,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        // Map and copy frame data
        void *data;
        vkMapMemory(device, stagingMemory, 0, imageSize, 0, &data);
        std::memcpy(data, newData, static_cast<size_t>(imageSize));
        vkUnmapMemory(device, stagingMemory);

        // Layout transition → copy → layout transition back
        transitionImageLayout(device, commandPool, graphicsQueue,
                              image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        copyBufferToImage(device, commandPool, graphicsQueue,
                          stagingBuffer, image, width, height);

        transitionImageLayout(device, commandPool, graphicsQueue,
                              image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        // Clean up staging resources
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);
    }

    void TextureImage::destroy(VkDevice device, TextureImage &texture)
    {
        if (texture.view != VK_NULL_HANDLE)
        {
            vkDestroyImageView(device, texture.view, nullptr);
            texture.view = VK_NULL_HANDLE;
        }

        if (texture.image != VK_NULL_HANDLE)
        {
            vkDestroyImage(device, texture.image, nullptr);
            texture.image = VK_NULL_HANDLE;
        }

        if (texture.memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(device, texture.memory, nullptr);
            texture.memory = VK_NULL_HANDLE;
        }

        texture.width = 0;
        texture.height = 0;
    }

} // namespace vst
