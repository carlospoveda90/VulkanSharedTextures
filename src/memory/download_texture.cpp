// download_texture.cpp
#include "memory/download_texture.hpp"
#include <stdexcept>
#include <cstring>

namespace vst
{

    static uint32_t findMemoryType(VkPhysicalDevice phys, uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(phys, &memProps);
        for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties)
                return i;
        }
        throw std::runtime_error("Failed to find suitable memory type.");
    }

    static VkCommandBuffer beginSingleCommandBuffer(VkDevice device, VkCommandPool pool)
    {
        VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        allocInfo.commandPool = pool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer cmd;
        vkAllocateCommandBuffers(device, &allocInfo, &cmd);

        VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &beginInfo);

        return cmd;
    }

    static void endSingleCommandBuffer(VkDevice device, VkCommandPool pool, VkQueue queue, VkCommandBuffer cmd)
    {
        vkEndCommandBuffer(cmd);
        VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;
        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);
        vkFreeCommandBuffers(device, pool, 1, &cmd);
    }

    CpuTextureData TextureDownloader::downloadImageToCpu(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        VkImage srcImage,
        uint32_t width,
        uint32_t height)
    {
        VkDeviceSize imageSize = width * height * 4;

        // Create buffer
        VkBuffer buffer;
        VkDeviceMemory bufferMemory;

        VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufferInfo.size = imageSize;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
            throw std::runtime_error("Failed to create buffer for image download.");

        VkMemoryRequirements memReq;
        vkGetBufferMemoryRequirements(device, buffer, &memReq);

        VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memReq.memoryTypeBits,
                                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate memory for image download.");

        vkBindBufferMemory(device, buffer, bufferMemory, 0);

        // Copy image to buffer
        VkCommandBuffer cmd = beginSingleCommandBuffer(device, commandPool);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = {width, height, 1};

        vkCmdCopyImageToBuffer(cmd, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region);

        endSingleCommandBuffer(device, commandPool, graphicsQueue, cmd);

        void *mapped;
        vkMapMemory(device, bufferMemory, 0, imageSize, 0, &mapped);

        return CpuTextureData{
            mapped, bufferMemory, buffer, imageSize};
    }

} // namespace vst
