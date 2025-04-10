#include "media/image_loader.hpp"
#include <stb_image.h>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <vulkan/vulkan.h>
#include <vector>
#include "tools/benchmark.hpp"

namespace vst
{
    extern PFN_vkGetMemoryFdKHR vkGetMemoryFdKHR;

    static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

        for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties)
                return i;
        }

        throw std::runtime_error("Failed to find suitable memory type.");
    }

    static void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBuffer &buffer, VkDeviceMemory &memory, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
    {
        VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
            throw std::runtime_error("Failed to create buffer.");

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements(device, buffer, &memReqs);

        VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memReqs.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate buffer memory.");

        vkBindBufferMemory(device, buffer, memory, 0);
    }

    static void createImage(VkDevice device, VkPhysicalDevice physicalDevice, int width, int height, VkImage &image, VkDeviceMemory &memory, bool exportMemory)
    {
        VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

        VkExternalMemoryImageCreateInfo externalCreateInfo{VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO};
        if (exportMemory)
        {
            externalCreateInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
            imageInfo.pNext = &externalCreateInfo;
        }

        if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS)
            throw std::runtime_error("Failed to create image.");

        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(device, image, &memReqs);
        std::cout << "Image memoryTypeBits from Image Loader: " << memReqs.memoryTypeBits << std::endl;

        VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkExportMemoryAllocateInfo exportAllocInfo{VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO};
        if (exportMemory)
        {
            exportAllocInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
            allocInfo.pNext = &exportAllocInfo;
        }

        if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate image memory.");

        vkBindImageMemory(device, image, memory, 0);
    }

    static VkCommandBuffer beginSingleUseCommandBuffer(VkDevice device, VkCommandPool commandPool)
    {
        VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer cmd;
        vkAllocateCommandBuffers(device, &allocInfo, &cmd);

        VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &beginInfo);

        return cmd;
    }

    static void endSingleUseCommandBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer cmd)
    {
        vkEndCommandBuffer(cmd);

        VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;

        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);

        vkFreeCommandBuffers(device, commandPool, 1, &cmd);
    }

    static void transitionImageLayout(VkDevice device, VkCommandPool pool, VkQueue queue, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
    {
        VkCommandBuffer cmd = beginSingleUseCommandBuffer(device, pool);

        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.layerCount = 1;

        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = 0;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        endSingleUseCommandBuffer(device, pool, queue, cmd);
    }

    static void copyBufferToImage(VkDevice device, VkCommandPool pool, VkQueue queue, VkBuffer buffer, VkImage image, int width, int height)
    {
        VkCommandBuffer cmd = beginSingleUseCommandBuffer(device, pool);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};

        vkCmdCopyBufferToImage(cmd, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        endSingleUseCommandBuffer(device, pool, queue, cmd);
    }

    static VkImageView createImageView(VkDevice device, VkImage image)
    {
        VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView view;
        if (vkCreateImageView(device, &viewInfo, nullptr, &view) != VK_SUCCESS)
            throw std::runtime_error("Failed to create image view.");
        return view;
    }

    // === MAIN FUNCTION IMPLEMENTATION ===

    Texture ImageLoader::loadTexture(const std::string &path, VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, bool exportMemory)
    {
        // benchmarking
        // vst::Timer timer;
        // timer.start();

        int texWidth, texHeight, texChannels;
        stbi_uc *pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels)
            throw std::runtime_error("Failed to load texture image.");

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

        // Record timing and optionally export fd
        // timer.stop();
        // BenchmarkLogger logger("dma_buf_transfer.csv");
        // logger.log("Image GPU allocation (" + std::string(exportMemory ? "DMA-BUF" : "Normal") + ")", timer.elapsedMilliseconds());
        // logger.flush();

        Texture texture{};
        texture.width = texWidth;
        texture.height = texHeight;

        createImage(device, physicalDevice, texWidth, texHeight, texture.image, texture.memory, exportMemory);

        transitionImageLayout(device, commandPool, graphicsQueue, texture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(device, commandPool, graphicsQueue, stagingBuffer, texture.image, texWidth, texHeight);
        transitionImageLayout(device, commandPool, graphicsQueue, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        texture.view = createImageView(device, texture.image);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);

        // benchmarking
        // timer.stop();
        // vst::BenchmarkLogger logger("load_texture.csv");
        // logger.log("Image upload to GPU", timer.elapsedMilliseconds());
        // logger.flush();

        // Get rowPitch using vkGetImageSubresourceLayout
        // VkImageSubresource subresource{};
        // subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        // subresource.mipLevel = 0;
        // subresource.arrayLayer = 0;

        // VkSubresourceLayout layout;
        // vkGetImageSubresourceLayout(device, texture.image, &subresource, &layout);

        // Export rowPitch to log and IPC
        // logger.log("Image Row Pitch", static_cast<double>(layout.rowPitch));
        // texture.rowPitch = layout.rowPitch; // Make sure Texture struct has this field

        return texture;
    }

} // namespace vst
