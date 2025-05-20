#include "media/texture_video.hpp"
#include "core/vulkan_context.hpp"
#include "core/vulkan_utils.hpp"
#include <stdexcept>
#include <cstring>

namespace vst
{
    TextureVideo::TextureVideo(VulkanContext &ctx)
        : context(ctx)
    {
    }

    TextureVideo::~TextureVideo()
    {
        destroy();
    }

    // bool TextureVideo::createFromSize(uint32_t width, uint32_t height)
    // {
    //     texWidth = width;
    //     texHeight = height;

    //     // External memory flags for DMA-BUF export
    //     VkExternalMemoryImageCreateInfo extMemoryImageInfo{};
    //     extMemoryImageInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
    //     extMemoryImageInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;

    //     // Create image with external memory support
    //     VkImageCreateInfo imageInfo{};
    //     imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    //     imageInfo.pNext = &extMemoryImageInfo; // Add external memory support
    //     imageInfo.imageType = VK_IMAGE_TYPE_2D;
    //     imageInfo.extent.width = texWidth;
    //     imageInfo.extent.height = texHeight;
    //     imageInfo.extent.depth = 1;
    //     imageInfo.mipLevels = 1;
    //     imageInfo.arrayLayers = 1;
    //     imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    //     imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    //     imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    //     // imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    //     //imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    //     imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
    //                   VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    //     imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    //     imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    //     if (vkCreateImage(context.getDevice(), &imageInfo, nullptr, &image) != VK_SUCCESS)
    //     {
    //         throw std::runtime_error("failed to create image!");
    //     }

    //     VkMemoryRequirements memRequirements;
    //     vkGetImageMemoryRequirements(context.getDevice(), image, &memRequirements);

    //     // External memory allocation info for DMA-BUF export
    //     VkExportMemoryAllocateInfo exportAllocInfo{};
    //     exportAllocInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
    //     exportAllocInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;

    //     VkMemoryAllocateInfo allocInfo{};
    //     allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    //     allocInfo.pNext = &exportAllocInfo; // Add export support
    //     allocInfo.allocationSize = memRequirements.size;
    //     allocInfo.memoryTypeIndex = vst::vulkan_utils::findMemoryType(context.getPhysicalDevice(),
    //                                                                   memRequirements.memoryTypeBits,
    //                                                                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    //     if (vkAllocateMemory(context.getDevice(), &allocInfo, nullptr, &memory) != VK_SUCCESS)
    //     {
    //         throw std::runtime_error("failed to allocate image memory!");
    //     }

    //     vkBindImageMemory(context.getDevice(), image, memory, 0);

    //     createSampler();

    //     // Create Image View
    //     VkImageViewCreateInfo viewInfo{};
    //     viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    //     viewInfo.image = image;
    //     viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    //     viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    //     viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    //     viewInfo.subresourceRange.baseMipLevel = 0;
    //     viewInfo.subresourceRange.levelCount = 1;
    //     viewInfo.subresourceRange.baseArrayLayer = 0;
    //     viewInfo.subresourceRange.layerCount = 1;

    //     if (vkCreateImageView(context.getDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS)
    //     {
    //         throw std::runtime_error("failed to create texture image view!");
    //     }

    //     return true;
    // }
    void TextureVideo::transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout)
    {
        // Create command buffer for the transition
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        // Set access masks and pipeline stages based on layouts
        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL)
        {
            // This is the key transition for compute shader access
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else
        {
            // Default case for other transitions
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = 0;

            sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        }

        m_currentLayout = newLayout;

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        endSingleTimeCommands(commandBuffer);
    }

    VkCommandBuffer TextureVideo::beginSingleTimeCommands()
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = context.getCommandPool();
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(context.getDevice(), &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void TextureVideo::endSingleTimeCommands(VkCommandBuffer commandBuffer)
    {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(context.getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(context.getGraphicsQueue());

        vkFreeCommandBuffers(context.getDevice(), context.getCommandPool(), 1, &commandBuffer);
    }

    bool TextureVideo::createFromSize(uint32_t width, uint32_t height)
    {
        texWidth = width;
        texHeight = height;

        std::cout << "Creating texture with size: " << width << "x" << height << '\n';

        // External memory flags for DMA-BUF export
        VkExternalMemoryImageCreateInfo extMemoryImageInfo{};
        extMemoryImageInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
        extMemoryImageInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;

        // Create image with external memory support
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.pNext = &extMemoryImageInfo; // Add external memory support
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = texWidth;
        imageInfo.extent.height = texHeight;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        // CRITICAL: Include all needed usage flags for sharing
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                          VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                          VK_IMAGE_USAGE_SAMPLED_BIT;

        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        std::cout << "Image usage flags: " << imageInfo.usage << '\n';

        if (vkCreateImage(context.getDevice(), &imageInfo, nullptr, &image) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(context.getDevice(), image, &memRequirements);

        std::cout << "Memory size: " << memRequirements.size << '\n';

        // External memory allocation info for DMA-BUF export
        VkExportMemoryAllocateInfo exportAllocInfo{};
        exportAllocInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
        exportAllocInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.pNext = &exportAllocInfo; // Add export support
        allocInfo.allocationSize = memRequirements.size;

        // Find a suitable memory type for DMA-BUF export
        allocInfo.memoryTypeIndex = vst::vulkan_utils::findMemoryType(
            context.getPhysicalDevice(),
            memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        std::cout << "Memory type index: " << allocInfo.memoryTypeIndex << '\n';

        if (vkAllocateMemory(context.getDevice(), &allocInfo, nullptr, &memory) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(context.getDevice(), image, memory, 0);

        std::cout << "Image created and memory bound\n";

        createSampler();

        // Create Image View
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(context.getDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create texture image view!");
        }

        std::cout << "Image view created\n";

        // Initially set to SHADER_READ_ONLY_OPTIMAL for rendering
        transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        // Update tracked layout
        m_currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        std::cout << "Texture creation completed successfully\n";
        return true;
    }

    void TextureVideo::createSampler()
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        if (vkCreateSampler(context.getDevice(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    void TextureVideo::destroy()
    {
        if (sampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(context.getDevice(), sampler, nullptr);
            sampler = VK_NULL_HANDLE;
        }
        if (imageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(context.getDevice(), imageView, nullptr);
            imageView = VK_NULL_HANDLE;
        }
        if (image != VK_NULL_HANDLE)
        {
            vkDestroyImage(context.getDevice(), image, nullptr);
            image = VK_NULL_HANDLE;
        }
        if (memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(context.getDevice(), memory, nullptr);
            memory = VK_NULL_HANDLE;
        }
    }

    void TextureVideo::updateFromFrame(const cv::Mat &frame)
    {
        if (frame.empty())
        {
            return;
        }

        if (frame.cols != static_cast<int>(texWidth) || frame.rows != static_cast<int>(texHeight))
        {
            throw std::runtime_error("Frame size does not match texture size!");
        }

        // Create staging buffer
        VkDeviceSize imageSize = texWidth * texHeight * 4; // RGBA 4 channels

        // Create temporary staging buffer
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        vst::vulkan_utils::createBuffer(context.getDevice(), context.getPhysicalDevice(), imageSize, stagingBuffer, stagingBufferMemory,
                                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        // Copy frame data
        void *data;
        vkMapMemory(context.getDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
        if (frame.channels() == 3)
        {
            // Frame is BGR -> need to expand to RGBA
            cv::Mat rgbaFrame;
            cv::cvtColor(frame, rgbaFrame, cv::COLOR_BGR2RGBA);
            std::memcpy(data, rgbaFrame.data, static_cast<size_t>(imageSize));
        }
        else if (frame.channels() == 4)
        {
            std::memcpy(data, frame.data, static_cast<size_t>(imageSize));
        }
        else
        {
            throw std::runtime_error("Unsupported frame format!");
        }
        vkUnmapMemory(context.getDevice(), stagingBufferMemory);

        // Transition image layout and copy buffer to image
        vst::vulkan_utils::copyBufferToImage(context.getDevice(),
                                             context.getCommandPool(),
                                             context.getGraphicsQueue(),
                                             stagingBuffer,
                                             image,
                                             texWidth,
                                             texHeight);

        // Clean up staging
        vkDestroyBuffer(context.getDevice(), stagingBuffer, nullptr);
        vkFreeMemory(context.getDevice(), stagingBufferMemory, nullptr);
    }
}