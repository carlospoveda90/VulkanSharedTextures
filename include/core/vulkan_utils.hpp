#pragma once

#include <vulkan/vulkan.h>

namespace vst::vulkan_utils
{

    uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
    
    void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size,
                      VkBuffer &buffer, VkDeviceMemory &bufferMemory,
                      VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties);

    void createImage(VkDevice device, VkPhysicalDevice physicalDevice, int width, int height,
                     VkImage &image, VkDeviceMemory &memory, bool exportMemory);

    VkImageView createImageView(VkDevice device, VkImage image);

    void transitionImageLayout(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue,
                               VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

    VkSampler createSampler(VkDevice device);

    void copyBufferToImage(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue,
                           VkBuffer buffer, VkImage image, int width, int height);

} // namespace vst::vulkan_utils
