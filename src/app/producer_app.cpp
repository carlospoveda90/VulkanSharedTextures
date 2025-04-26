#include <thread>
#include <unistd.h>
#include <sys/socket.h>
#include <cstring>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include "app/producer_app.hpp"
#include "core/vulkan_context.hpp"
#include "core/vertex_definitions.hpp"
#include "media/texture_image.hpp"
#include "media/image_loader.hpp"
#include "media/video_loader.hpp"
#include "ipc/fd_passing.hpp"
#include "utils/logger.hpp"
#include "shm/shm_writer.hpp"
#include "shm/shm_viewer.hpp"
#include "memory/shm_handler.hpp"
#include "stb_image.h"
#include "utils/file_utils.hpp"

namespace vst
{
    void createDescriptorPool(VkDevice device, VkDescriptorPool &pool);
    void createVertexBuffer(VkDevice device, VkPhysicalDevice phys, VkBuffer &buffer, VkDeviceMemory &memory, const std::vector<vst::Vertex> &vertices);

    ProducerApp::ProducerApp()
        : context(*(new VulkanContext()))
    {
    }

    // ProducerApp::ProducerApp(VulkanContext &ctx)
    //     : context(ctx)
    // {
    // }

    void ProducerApp::ProducerDMA(GLFWwindow *window, const std::string &filePath, const std::string &mode, bool isVideo)
    {
        this->mode = mode;
        context.init(window);

        if (isVideo)
        {
            LOG_INFO("Running video producer, loading: " + filePath);
            LOG_INFO("Mode: DMA-BUF with Vulkan");
        }
        else
        {
            TextureImage texture;
            texture = ImageLoader::loadTexture(filePath, context.getDevice(), context.getPhysicalDevice(),
                                               context.getCommandPool(), context.getGraphicsQueue(), mode == "dma");

            createDescriptorPool(context.getDevice(), descriptorPool);
            descriptorManager.init(context.getDevice(), descriptorPool, texture);
            pipeline.create(context.getDevice(), context.getSwapchainExtent(), context.getRenderPass(), descriptorManager.getLayout());

            createVertexBuffer(
                context.getDevice(),
                context.getPhysicalDevice(),
                vertexBuffer,
                vertexBufferMemory,
                FULLSCREEN_QUAD);

            VkMemoryGetFdInfoKHR getFdInfo{VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR};
            getFdInfo.memory = texture.memory;
            getFdInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;

            auto vkGetMemoryFdKHR = reinterpret_cast<PFN_vkGetMemoryFdKHR>(vkGetDeviceProcAddr(context.getDevice(), "vkGetMemoryFdKHR"));

            if (!vkGetMemoryFdKHR)
            {
                throw std::runtime_error("vkGetMemoryFdKHR is not available on this device.");
            }

            int fd = -1;
            if (vkGetMemoryFdKHR(context.getDevice(), &getFdInfo, &fd) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to export DMA-BUF.");
            }

            std::string shmName = "/tmp/vulkan_shared-" + std::to_string(texture.width) + "x" + std::to_string(texture.height) + ".sock";
            LOG_INFO("Creating shared memory segment: " + shmName);
            this->shmName = shmName;

            std::thread([fd, this, shmName, texture]()
                        {
                LOG_INFO("Waiting for consumer connection on socket...");
                int server_fd = ipc::setup_unix_server_socket(shmName);
                int client_fd = accept(server_fd, nullptr, nullptr);
                if (client_fd >= 0)
                {
                    vst::ipc::send_fd_with_info(client_fd, fd, texture.width, texture.height);
                    LOG_INFO("Sent FD to consumer via Unix socket.");
                    close(client_fd);
                }
                close(server_fd); })
                .detach();
        }
    }
    
    void ProducerApp::ProducerSHM(const std::string &filePath, const std::string &mode, bool isVideo)
    {
        this->mode = mode;
        LOG_INFO("ProducerApp constructor called with filePath: " + filePath + " and mode: " + mode);

        vst::utils::ImageSize imageData = vst::utils::getImageSize(filePath);
        // Create shared memory segment
        LOG_INFO("Running in shm_open mode...");

        int texWidth, texHeight, texChannels;
        stbi_uc *pixels = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

        LOG_INFO("width: " + std::to_string(texWidth) + ", height: " + std::to_string(texHeight) + ", channels: " + std::to_string(texChannels));

        if (!pixels)
            throw std::runtime_error("Failed to load image for shm.");

        std::string shmNameSize = "/vst_shared_texture-" + std::to_string(imageData.width) + "x" + std::to_string(imageData.height);
        LOG_INFO("Creating shared memory segment: " + shmNameSize);

        size_t imageSize = texWidth * texHeight * 4;
        bool success = vst::shm::write_to_shm(shmNameSize, imageData, pixels, imageSize);
        stbi_image_free(pixels); // Feel free the loaded image data

        if (!success)
            throw std::runtime_error("Failed to write image to shared memory.");

        LOG_INFO("Image written to shared memory: /dev/shm" + shmNameSize);

        return;
    }

    void createDescriptorPool(VkDevice device, VkDescriptorPool &pool)
    {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = 1;

        VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = 1;

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create descriptor pool.");
        }
    }

    void createVertexBuffer(VkDevice device, VkPhysicalDevice phys, VkBuffer &buffer, VkDeviceMemory &memory, const std::vector<vst::Vertex> &vertices)
    {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufferInfo.size = bufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
            throw std::runtime_error("Failed to create vertex buffer");

        VkMemoryRequirements memReq;
        vkGetBufferMemoryRequirements(device, buffer, &memReq);

        VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex = 0;

        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(phys, &memProps);
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

        if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate vertex buffer memory");

        vkBindBufferMemory(device, buffer, memory, 0);

        void *data;
        vkMapMemory(device, memory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(device, memory);
    }

    void ProducerApp::runFrame()
    {
        if (!pipeline.get())
            // LOG_ERR("Pipeline is null");
            throw std::runtime_error("drawFrame: pipeline is null");
        if (!pipeline.getLayout())
            // LOG_ERR("Pipeline layout is null");
            throw std::runtime_error("drawFrame: pipeline layout is null");
        if (!descriptorManager.getDescriptorSet())
            // LOG_ERR("Descriptor set is null");
            throw std::runtime_error("drawFrame: descriptorSet is null");
        if (!vertexBuffer)
            // LOG_ERR("Vertex buffer is null");
            throw std::runtime_error("drawFrame: vertexBuffer is null");

        context.drawFrame(
            pipeline.get(),
            pipeline.getLayout(),
            descriptorManager.getDescriptorSet(),
            vertexBuffer);
    }

    void ProducerApp::runFrame(const std::string &filePath)
    {
        LOG_INFO("Running shm_open producer");
        vst::utils::ImageSize imageData = vst::utils::getImageSize(filePath);
        LOG_INFO("Image size: " + std::to_string(imageData.width) + "x" + std::to_string(imageData.height));
        std::string shmNameSize = "/vst_shared_texture-" + std::to_string(imageData.width) + "x" + std::to_string(imageData.height);
        this->shmName = shmNameSize;
        vst::shm::run_viewer(shmNameSize.c_str(), "Producer");
    }

    void ProducerApp::cleanup()
    {
        if (this->mode == "dma")
        {
            vkDestroyBuffer(context.getDevice(), vertexBuffer, nullptr);
            vkFreeMemory(context.getDevice(), vertexBufferMemory, nullptr);
            descriptorManager.cleanup(context.getDevice());
            pipeline.cleanup(context.getDevice());
            vkDestroyDescriptorPool(context.getDevice(), descriptorPool, nullptr);
            context.cleanup();
            LOG_INFO("Socker Name: " + this->shmName);
            ipc::cleanup_unix_socket(this->shmName);
        }
        else if (this->mode == "shm")
        {
            // Cleanup shared memory resources if needed
            LOG_INFO("Cleaning up SHM resources...");
            vst::shm::destroy_viewer(this->shmName);
        }
        else
        {
            throw std::runtime_error("Invalid cleanup mode. Use 'dma' or 'shm'.");
        }
    }

    ProducerApp::~ProducerApp()
    {
        // stopVideoStreaming();
        cleanup();
    }

} // namespace vst
