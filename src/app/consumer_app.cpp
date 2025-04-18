#include "app/consumer_app.hpp"
#include "core/vertex_definitions.hpp"
#include "utils/file_utils.hpp"
#include "utils/logger.hpp"
#include <stdexcept>
#include <vulkan/vulkan.h>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include "ipc/fd_passing.hpp"
#include "shm/shm_viewer.hpp"
#include <SDL3/SDL.h>

namespace vst
{
    void createVertexBuffer(VkDevice device, VkPhysicalDevice phys, VkBuffer &buffer, VkDeviceMemory &memory, const std::vector<vst::Vertex> &vertices)
    {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufferInfo.size = bufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);

        VkMemoryRequirements memReq;
        vkGetBufferMemoryRequirements(device, buffer, &memReq);

        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(phys, &memProps);

        VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocInfo.allocationSize = memReq.size;

        for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
        {
            if ((memReq.memoryTypeBits & (1 << i)) &&
                (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
                (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
            {
                allocInfo.memoryTypeIndex = i;
                break;
            }
        }

        vkAllocateMemory(device, &allocInfo, nullptr, &memory);
        vkBindBufferMemory(device, buffer, memory, 0);

        void *data;
        vkMapMemory(device, memory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(device, memory);
    }

    ConsumerApp::ConsumerApp(GLFWwindow *window, const std::string &mode)
    {
        this->mode = mode;
        context.init(window);
        // === Socket: Receive FD from Producer ===
        LOG_INFO("Connecting to producer via socket...");
        
        std::string socketPath = vst::utils::findLatestDmaSocket().value_or("");
        LOG_INFO("Socket path: " + socketPath); 
        int sock_fd = vst::ipc::connect_unix_client_socket(socketPath);
        if (sock_fd < 0)
            throw std::runtime_error("Failed to connect to producer via socket.");
        int fd = -1;
        uint32_t texWidth = 0, texHeight = 0;
        if (vst::ipc::receive_fd_with_info(sock_fd, fd, texWidth, texHeight) < 0)
        {
            throw std::runtime_error("Failed to receive FD from producer.");
        }
        LOG_INFO("Received FD = " + std::to_string(fd) + " with dimensions: " + std::to_string(texWidth) + "x" + std::to_string(texHeight));

        // Create image with external memory support and format Compatibility Check ===
        VkPhysicalDeviceExternalImageFormatInfo externalImageFormatInfo{};
        externalImageFormatInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO;
        externalImageFormatInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;

        VkPhysicalDeviceImageFormatInfo2 formatInfo{};
        formatInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2;
        formatInfo.pNext = &externalImageFormatInfo;
        formatInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        formatInfo.type = VK_IMAGE_TYPE_2D;
        formatInfo.tiling = VK_IMAGE_TILING_LINEAR;
        formatInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
        formatInfo.flags = 0;

        VkExternalImageFormatProperties externalImageFormatProps{};
        externalImageFormatProps.sType = VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES;

        VkImageFormatProperties2 imageFormatProps{};
        imageFormatProps.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2;
        imageFormatProps.pNext = &externalImageFormatProps;

        VkResult fmtResult = vkGetPhysicalDeviceImageFormatProperties2(
            context.getPhysicalDevice(),
            &formatInfo,
            &imageFormatProps);

        if (fmtResult != VK_SUCCESS)
        {
            throw std::runtime_error("Image format not supported for external memory (DMA-BUF).");
        }

        // Create image matching producer's configuration
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL; // Must match producer
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

        // Add external memory flag
        VkExternalMemoryImageCreateInfo extImageInfo{};
        extImageInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
        extImageInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
        imageInfo.pNext = &extImageInfo;

        if (vkCreateImage(context.getDevice(), &imageInfo, nullptr, &importedImage) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create image for imported memory");
        }

        // Query memory requirements and properties
        VkMemoryRequirements memReq{};
        vkGetImageMemoryRequirements(context.getDevice(), importedImage, &memReq);

        VkPhysicalDeviceMemoryProperties memProps{};
        vkGetPhysicalDeviceMemoryProperties(context.getPhysicalDevice(), &memProps);

        std::cout << "Image memoryTypeBits from Consumer App: " << memReq.memoryTypeBits << std::endl;

        LOG_INFO("Image memoryTypeBits: " << memReq.memoryTypeBits);
        for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
        {
            LOG_INFO("Memory Type " << i << ": flags=" << memProps.memoryTypes[i].propertyFlags);
        }

        // Memory Requirements for Image
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(context.getDevice(), importedImage, &memRequirements);

        std::cout << "Image memoryTypeBits from Consumer App: " << memRequirements.memoryTypeBits << std::endl;

        // External Memory Import Info
        VkImportMemoryFdInfoKHR importInfo{};
        importInfo.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR;
        importInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
        importInfo.fd = fd; // this must be valid and not already closed

        // Memory Allocation Info
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.pNext = &importInfo;

        // Choose Compatible Memory Type
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(context.getPhysicalDevice(), &memProperties);

        bool found = false;
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((memRequirements.memoryTypeBits & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
            {
                allocInfo.memoryTypeIndex = i;
                found = true;
                break;
            }
        }

        if (!found)
        {
            throw std::runtime_error("Failed to find suitable memory type for imported DMA-BUF image.");
        }

        // Allocate the memory
        if (vkAllocateMemory(context.getDevice(), &allocInfo, nullptr, &importedMemory) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate memory for imported DMA-BUF image.");
        }

        // Bind the memory to the image
        if (vkBindImageMemory(context.getDevice(), importedImage, importedMemory, 0) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to bind imported memory to image.");
        }

        // Create image view
        VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = importedImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = imageInfo.format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(context.getDevice(), &viewInfo, nullptr, &importedImageView) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create image view for imported DMA-BUF.");
        }

        // Init descriptor and pipeline
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = 1;

        VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = 1;

        vkCreateDescriptorPool(context.getDevice(), &poolInfo, nullptr, &descriptorPool);

        Texture importedTex{};
        importedTex.view = importedImageView;
        importedTex.width = imageWidth;
        importedTex.height = imageHeight;

        descriptorManager.init(context.getDevice(), descriptorPool, importedTex);

        pipeline.create(
            context.getDevice(),
            context.getSwapchainExtent(),
            context.getRenderPass(),
            descriptorManager.getLayout());

        createVertexBuffer(
            context.getDevice(),
            context.getPhysicalDevice(),
            vertexBuffer,
            vertexBufferMemory,
            vst::FULLSCREEN_QUAD);

        LOG_INFO("DMA-BUF imported and image view created successfully.");
    }

    ConsumerApp::ConsumerApp(const std::string &mode)
    {
        this->mode = mode;
        std::string mediaPath = vst::utils::find_shared_image_file().value_or("");
        if (mediaPath.empty())
            throw std::runtime_error("Failed to find shared image file.");

        std::string fileName = "/" + std::string(vst::utils::getFileName(mediaPath).c_str());
        this->shmName = fileName;
        LOG_INFO("Running shm_open consumer with media path: " + mediaPath);
        vst::shm::run_viewer(fileName.c_str(), "Consumer");
        return;
    }

    void ConsumerApp::runFrame()
    {
        context.drawFrame(
            pipeline.get(),
            pipeline.getLayout(),
            descriptorManager.getDescriptorSet(),
            vertexBuffer);
    }

    void ConsumerApp::cleanup()
    {
        if (this->mode == "dma")
        {
            if (importedImageView)
            {
                vkDestroyImageView(context.getDevice(), importedImageView, nullptr);
            }
            if (importedImage)
            {
                vkDestroyImage(context.getDevice(), importedImage, nullptr);
            }
            if (importedMemory)
            {
                vkFreeMemory(context.getDevice(), importedMemory, nullptr);
            }
            if (vertexBuffer)
            {
                vkDestroyBuffer(context.getDevice(), vertexBuffer, nullptr);
                vkFreeMemory(context.getDevice(), vertexBufferMemory, nullptr);
            }
            if (descriptorPool)
            {
                descriptorManager.cleanup(context.getDevice());
                vkDestroyDescriptorPool(context.getDevice(), descriptorPool, nullptr);
            }
            pipeline.cleanup(context.getDevice());
            context.cleanup();
        }
        else if (this->mode == "shm")
        {
            // Cleanup for shm mode
            LOG_INFO("Cleaning up SHM viewer...");
            vst::shm::destroy_viewer(this->shmName);
        }
    }

    ConsumerApp::~ConsumerApp()
    {
        cleanup();
    }

} // namespace vst
