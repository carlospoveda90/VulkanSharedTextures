#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>

namespace vst
{
    class TextureImage
    {
    public:
        TextureImage();

        void updateFromRawData(
            VkDevice device,
            VkPhysicalDevice physicalDevice,
            VkCommandPool commandPool,
            VkQueue graphicsQueue,
            const uint8_t *newData);
        static void destroy(VkDevice device, TextureImage &texture);
        VkSampler getSampler() const { return sampler; }
        bool isInitialized() const { return image != VK_NULL_HANDLE; }

        VkImage image;
        VkDeviceMemory memory;
        VkSampler sampler = VK_NULL_HANDLE;
        VkImageView view;
        int width;
        int height;
    };

} // namespace vst
