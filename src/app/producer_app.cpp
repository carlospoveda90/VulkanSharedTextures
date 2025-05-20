#include <thread>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/mman.h>
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
#include "memory/shm_video_handler.hpp"
#include "stb_image.h"
#include "utils/file_utils.hpp"

namespace vst
{
    void createDescriptorPool(VkDevice device, VkDescriptorPool &pool);
    static void createVertexBuffer(VkDevice device, VkPhysicalDevice phys, VkBuffer &buffer, VkDeviceMemory &memory, const std::vector<vst::Vertex> &vertices);

    ProducerApp::ProducerApp()
        : context(*(new VulkanContext()))
    {
    }

    // ProducerApp::ProducerApp(VulkanContext &ctx)
    //     : context(ctx)
    // {
    // }

    ProducerApp::ProducerApp(const VulkanContext &context)
        : context(const_cast<VulkanContext &>(context))
    {
    }

    void ProducerApp::update()
    {
        if (this->isVideo && this->mode == "dma" && videoTexture && videoLoader)
        {
            // Add debug logging to verify this method is being called
            static int frameCount = 0;
            if (frameCount % 30 == 0)
            { // Log every 30 frames
                LOG_INFO("Updating video frame: " + std::to_string(frameCount));
            }

            cv::Mat frame;
            if (!videoLoader->grabFrame(frame))
            {
                // End of video reached, loop back to beginning
                LOG_INFO("End of video reached, restarting...");
                videoLoader->getCapture().set(cv::CAP_PROP_POS_FRAMES, 0);
                return;
            }

            // Update the video texture with the new frame
            try
            {
                videoTexture->updateFromFrame(frame);
                frameCount++;
            }
            catch (const std::exception &e)
            {
                LOG_ERR("Failed to update texture from frame: " + std::string(e.what()));
            }
        }
    }

    bool ProducerApp::updateFromComputedTexture()
    {
        if (!videoTexture)
        {
            LOG_ERR("Video texture is null");
            return false;
        }

        // // Force the texture to be in the SHADER_READ_ONLY_OPTIMAL layout
        // // This will ensure it's visible in the producer window
        // videoTexture->transitionImageLayout(
        //     VK_IMAGE_LAYOUT_GENERAL,
        //     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        // Only transition if needed - don't transition during compute operations
        // if (videoTexture->getCurrentLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        // {
        //     videoTexture->transitionImageLayout(
        //         videoTexture->getCurrentLayout(),
        //         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        // }

        return true;
    }

    bool ProducerApp::copyTextureFromImage(VkImage sourceImage)
    {
        if (!videoTexture || sourceImage == VK_NULL_HANDLE)
        {
            return false;
        }

        VkDevice device = context.getDevice();
        VkPhysicalDevice physDevice = context.getPhysicalDevice();
        VkCommandPool cmdPool = context.getCommandPool();
        VkQueue gfxQueue = context.getGraphicsQueue();

        VkCommandBuffer cmdBuffer;

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = cmdPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(device, &allocInfo, &cmdBuffer) != VK_SUCCESS)
        {
            LOG_ERR("Failed to allocate command buffer for texture copy");
            return false;
        }

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        if (vkBeginCommandBuffer(cmdBuffer, &beginInfo) != VK_SUCCESS)
        {
            LOG_ERR("Failed to begin command buffer for texture copy");
            vkFreeCommandBuffers(device, cmdPool, 1, &cmdBuffer);
            return false;
        }

        // Set up image copy
        VkImageCopy region{};
        region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.srcSubresource.mipLevel = 0;
        region.srcSubresource.baseArrayLayer = 0;
        region.srcSubresource.layerCount = 1;
        region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.dstSubresource.mipLevel = 0;
        region.dstSubresource.baseArrayLayer = 0;
        region.dstSubresource.layerCount = 1;
        region.extent.width = videoTexture->getWidth();
        region.extent.height = videoTexture->getHeight();
        region.extent.depth = 1;

        // Transition source image to transfer source layout
        VkImageMemoryBarrier srcBarrier{};
        srcBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        srcBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        srcBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        srcBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        srcBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        srcBarrier.image = sourceImage;
        srcBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        srcBarrier.subresourceRange.baseMipLevel = 0;
        srcBarrier.subresourceRange.levelCount = 1;
        srcBarrier.subresourceRange.baseArrayLayer = 0;
        srcBarrier.subresourceRange.layerCount = 1;
        srcBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(cmdBuffer,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &srcBarrier);

        // Transition destination image to transfer destination layout
        VkImageMemoryBarrier dstBarrier{};
        dstBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        dstBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        dstBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        dstBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        dstBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        dstBarrier.image = videoTexture->getImage();
        dstBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        dstBarrier.subresourceRange.baseMipLevel = 0;
        dstBarrier.subresourceRange.levelCount = 1;
        dstBarrier.subresourceRange.baseArrayLayer = 0;
        dstBarrier.subresourceRange.layerCount = 1;
        dstBarrier.srcAccessMask = 0;
        dstBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(cmdBuffer,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &dstBarrier);

        // Copy the image
        vkCmdCopyImage(cmdBuffer,
                       sourceImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       videoTexture->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1, &region);

        // Transition destination image back to shader read layout
        dstBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        dstBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dstBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        dstBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        vkCmdPipelineBarrier(cmdBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &dstBarrier);

        // End and submit command buffer
        if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS)
        {
            LOG_ERR("Failed to end command buffer for texture copy");
            vkFreeCommandBuffers(device, cmdPool, 1, &cmdBuffer);
            return false;
        }

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuffer;

        if (vkQueueSubmit(gfxQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
        {
            LOG_ERR("Failed to submit command buffer for texture copy");
            vkFreeCommandBuffers(device, cmdPool, 1, &cmdBuffer);
            return false;
        }

        vkQueueWaitIdle(gfxQueue);
        vkFreeCommandBuffers(device, cmdPool, 1, &cmdBuffer);

        LOG_INFO("Texture copied successfully");
        return true;
    }

    void ProducerApp::ProducerDMA(GLFWwindow *window, const std::string &filePath, const std::string &mode, bool isVideo)
    {
        this->isVideo = isVideo;
        this->mode = mode;
        context.init(window);

        if (isVideo)
        {
            LOG_INFO("Running video producer in DMA-BUF mode, loading: " + filePath);

            // Initialize video loader
            videoLoader = std::make_unique<VideoLoader>();
            if (!videoLoader->open(filePath))
            {
                throw std::runtime_error("Failed to open video file: " + filePath);
            }

            // Get video properties
            cv::VideoCapture &cap = videoLoader->getCapture();
            int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
            int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
            double fps = cap.get(cv::CAP_PROP_FPS);
            int totalFrames = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));

            LOG_INFO("Video properties: " + std::to_string(width) + "x" + std::to_string(height) +
                     " @ " + std::to_string(fps) + " fps, " + std::to_string(totalFrames) + " frames");

            // Create a video texture that will be shared via DMA-BUF
            // First, grab a frame to initialize with
            cv::Mat firstFrame;
            if (!videoLoader->grabFrame(firstFrame))
            {
                throw std::runtime_error("Failed to read first frame from video");
            }

            // Create a texture for the video
            videoTexture = std::make_unique<TextureVideo>(context);
            if (!videoTexture->createFromSize(firstFrame.cols, firstFrame.rows))
            {
                throw std::runtime_error("Failed to create video texture");
            }

            // Reset the video to the beginning
            videoLoader->getCapture().set(cv::CAP_PROP_POS_FRAMES, 0);

            // Update texture with first frame
            videoTexture->updateFromFrame(firstFrame);

            // Setup descriptors and pipeline for rendering
            createDescriptorPool(context.getDevice(), descriptorPool);

            // Initialize descriptor manager with video texture
            TextureImage texture;
            texture.image = videoTexture->getImage();
            texture.memory = videoTexture->getMemory();
            texture.view = videoTexture->getImageView();
            texture.sampler = videoTexture->getSampler();
            texture.width = width;
            texture.height = height;

            descriptorManager.init(context.getDevice(), descriptorPool, texture);
            pipeline.create(context.getDevice(), context.getSwapchainExtent(),
                            context.getRenderPass(), descriptorManager.getLayout());

            createVertexBuffer(
                context.getDevice(),
                context.getPhysicalDevice(),
                vertexBuffer,
                vertexBufferMemory,
                FULLSCREEN_QUAD);

            // Export the texture via DMA-BUF
            VkMemoryGetFdInfoKHR getFdInfo{VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR};
            getFdInfo.memory = videoTexture->getMemory();
            getFdInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;

            auto vkGetMemoryFdKHR = reinterpret_cast<PFN_vkGetMemoryFdKHR>(
                vkGetDeviceProcAddr(context.getDevice(), "vkGetMemoryFdKHR"));

            if (!vkGetMemoryFdKHR)
            {
                throw std::runtime_error("vkGetMemoryFdKHR is not available on this device.");
            }

            int fd = -1;
            if (vkGetMemoryFdKHR(context.getDevice(), &getFdInfo, &fd) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to export DMA-BUF for video texture.");
            }

            std::string shmName = "/tmp/vulkan_shared_video-" + std::to_string(width) + "x" + std::to_string(height) + ".sock";
            LOG_INFO("Creating DMA-BUF socket for video: " + shmName);
            this->shmName = shmName;

            setupDmaSocket(shmName, fd, width, height);

            // std::thread([fd, this, shmName, width, height]()
            //             {
            // LOG_INFO("Waiting for consumer connection on socket...");
            // int server_fd = ipc::setup_unix_server_socket(shmName);
            // int client_fd = accept(server_fd, nullptr, nullptr);
            // if (client_fd >= 0)
            // {
            //     vst::ipc::send_fd_with_info(client_fd, fd, width, height);
            //     LOG_INFO("Sent FD to consumer via Unix socket.");
            //     close(client_fd);
            // }
            // close(server_fd); })
            //     .detach();

            // LOG_INFO("DMA-BUF video producer initialized");
        }
        else
        {
            // Existing image handling code
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

            std::string shmName = "/tmp/vulkan_shared_image-" + std::to_string(texture.width) + "x" + std::to_string(texture.height) + ".sock";
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
        this->isVideo = isVideo;
        this->mode = mode;
        LOG_INFO("ProducerApp constructor called with filePath: " + filePath + " and mode: " + mode);

        if (isVideo)
        {
            LOG_INFO("Running in shm_open mode with video...");

            // Initialize video loader
            videoLoader = std::make_unique<VideoLoader>();
            if (!videoLoader->open(filePath))
            {
                throw std::runtime_error("Failed to open video file: " + filePath);
            }

            // Get video properties
            cv::VideoCapture &cap = videoLoader->getCapture();
            int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
            int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
            double fps = cap.get(cv::CAP_PROP_FPS);
            int totalFrames = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));

            LOG_INFO("Video properties: " + std::to_string(width) + "x" + std::to_string(height) +
                     " @ " + std::to_string(fps) + " fps, " + std::to_string(totalFrames) + " frames");

            // Create shared memory segment name
            std::string shmName = "/vst_shared_video-" + std::to_string(width) + "x" + std::to_string(height);
            this->shmName = shmName;

            // Make sure any previous instances are cleaned up
            shm_unlink(shmName.c_str());

            // Create shared memory handler for video
            auto shmHandler = std::make_shared<vst::memory::ShmVideoHandler>();
            if (!shmHandler->createSharedMemory(shmName, width, height, 4)) // RGBA format
            {
                throw std::runtime_error("Failed to create shared memory for video");
            }

            // Create OpenCV window for display
            std::string windowName = "Producer - SHM Video " + std::to_string(width) + "x" + std::to_string(height);
            cv::namedWindow(windowName, cv::WINDOW_NORMAL | cv::WINDOW_GUI_NORMAL);
            cv::resizeWindow(windowName, width, height);

            // Store the shmHandler for later use
            this->shmVideoHandler = shmHandler;
            this->windowTitle = windowName;

            // Signal that we're ready for the main loop
            this->running = true;

            LOG_INFO("Video streaming initialized in shared memory: " + shmName);
            LOG_INFO("Video will be played in the main loop.");
        }
        else
        {
            // Your existing image handling code for SHM mode
            vst::utils::ImageSize imageData = vst::utils::getImageSize(filePath);
            LOG_INFO("Running in shm_open mode...");

            int texWidth, texHeight, texChannels;
            stbi_uc *pixels = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

            LOG_INFO("width: " + std::to_string(texWidth) + ", height: " + std::to_string(texHeight) +
                     ", channels: " + std::to_string(texChannels));

            if (!pixels)
                throw std::runtime_error("Failed to load image for shm.");

            std::string shmNameSize = "/vst_shared_texture-" + std::to_string(imageData.width) + "x" +
                                      std::to_string(imageData.height);
            LOG_INFO("Creating shared memory segment: " + shmNameSize);
            this->shmName = shmNameSize;

            size_t imageSize = texWidth * texHeight * 4;
            bool success = vst::shm::write_to_shm(shmNameSize, imageData, pixels, imageSize);
            stbi_image_free(pixels);

            if (!success)
                throw std::runtime_error("Failed to write image to shared memory.");

            LOG_INFO("Image written to shared memory: /dev/shm" + shmNameSize);
        }
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

    static void createVertexBuffer(VkDevice device, VkPhysicalDevice phys, VkBuffer &buffer, VkDeviceMemory &memory, const std::vector<vst::Vertex> &vertices)
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

    bool ProducerApp::setupDmaSocket(const std::string &socketPath, int fd, uint32_t width, uint32_t height)
    {
        LOG_INFO("=== Setting up DMA socket ===");
        LOG_INFO("Socket path: " + socketPath);
        LOG_INFO("FD: " + std::to_string(fd));
        LOG_INFO("Dimensions: " + std::to_string(width) + "x" + std::to_string(height));

        // Check if socket already exists and remove it
        if (access(socketPath.c_str(), F_OK) == 0)
        {
            LOG_INFO("Socket already exists, removing it");
            unlink(socketPath.c_str());
        }

        // Store for future connections
        m_dmaFd = fd;
        m_texWidth = width;
        m_texHeight = height;

        // Create server socket
        m_serverSocketFd = ipc::setup_unix_server_socket(socketPath);
        if (m_serverSocketFd < 0)
        {
            LOG_ERR("Failed to create socket server: " +
                    std::string(strerror(errno)));
            return false;
        }

        // Set socket to non-blocking mode
        int flags = fcntl(m_serverSocketFd, F_GETFL, 0);
        fcntl(m_serverSocketFd, F_SETFL, flags | O_NONBLOCK);

        m_socketServerRunning = true;
        LOG_INFO("DMA-BUF socket server started on: " + socketPath);
        return true;
    }

    void ProducerApp::checkForConnections()
    {
        if (!m_socketServerRunning || m_serverSocketFd < 0 || m_dmaFd < 0)
        {
            return;
        }

        // Check for a new connection
        int client_fd = accept(m_serverSocketFd, nullptr, nullptr);
        if (client_fd >= 0)
        {
            LOG_INFO("New consumer connected");
            vst::ipc::send_fd_with_info(client_fd, m_dmaFd, m_texWidth, m_texHeight);
            LOG_INFO("Sent FD to consumer");
            close(client_fd);
        }
    }

    void ProducerApp::runFrame()
    {
        // Make sure these checks are in place
        if (this->mode == "dma")
        {
            checkForConnections();
        }
        if (!pipeline.get())
        {
            LOG_ERR("Pipeline is null");
            return;
        }
        if (!pipeline.getLayout())
        {
            LOG_ERR("Pipeline layout is null");
            return;
        }
        if (!descriptorManager.getDescriptorSet())
        {
            LOG_ERR("Descriptor set is null");
            return;
        }
        if (!vertexBuffer)
        {
            LOG_ERR("Vertex buffer is null");
            return;
        }

        try
        {
            context.drawFrame(
                pipeline.get(),
                pipeline.getLayout(),
                descriptorManager.getDescriptorSet(),
                vertexBuffer);
        }
        catch (const std::exception &e)
        {
            LOG_ERR("Error in drawFrame: " + std::string(e.what()));
        }
    }

    void ProducerApp::runFrame(const std::string &filePath)
    {
        if (this->isVideo)
        {
            LOG_INFO("Running shm_open producer for video");
            // For video in SHM mode, we don't need to do anything here
            // as the separate decoding thread is handling everything
        }
        else
        {
            LOG_INFO("Running shm_open producer");
            vst::utils::ImageSize imageData = vst::utils::getImageSize(filePath);
            LOG_INFO("Image size: " + std::to_string(imageData.width) + "x" + std::to_string(imageData.height));
            std::string shmNameSize = "/vst_shared_texture-" + std::to_string(imageData.width) + "x" + std::to_string(imageData.height);
            this->shmName = shmNameSize;
            vst::shm::run_viewer(shmNameSize.c_str(), "Producer");
        }
    }

    bool vst::ProducerApp::initSharedMemory(const std::string &name, int width, int height)
    {
        this->mode = "shm";
        this->isVideo = true;

        // Create shared memory segment name
        std::string shmName = name;
        if (shmName.find("/") != 0)
        {
            shmName = "/" + shmName;
        }
        this->shmName = shmName;

        // Make sure any previous instances are cleaned up
        shm_unlink(shmName.c_str());

        // Create shared memory handler for video
        auto shmHandler = std::make_shared<vst::memory::ShmVideoHandler>();
        if (!shmHandler->createSharedMemory(shmName, width, height, 4))
        { // RGBA format
            LOG_ERR("Failed to create shared memory for video");
            return false;
        }

        // Store the shmHandler for later use
        this->shmVideoHandler = shmHandler;
        this->windowTitle = "Producer - SHM Video " + std::to_string(width) + "x" + std::to_string(height);

        // Signal that we're ready
        this->running = true;

        LOG_INFO("Video streaming initialized in shared memory: " + shmName);
        return true;
    }

    bool vst::ProducerApp::writeFrame(const cv::Mat &frame)
    {
        if (!this->running || !this->shmVideoHandler)
        {
            LOG_ERR("Cannot write frame - producer not initialized");
            return false;
        }

        // Calculate timestamp
        static auto startTime = std::chrono::steady_clock::now();
        static int frameCount = 0;

        auto now = std::chrono::steady_clock::now();
        uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 now - startTime)
                                 .count();

        // Convert to RGBA regardless of input format
        cv::Mat rgbaFrame;
        if (frame.channels() == 1)
        {
            // Grayscale to RGBA
            cv::cvtColor(frame, rgbaFrame, cv::COLOR_GRAY2RGBA);
        }
        else if (frame.channels() == 3)
        {
            // BGR to RGBA
            cv::cvtColor(frame, rgbaFrame, cv::COLOR_BGR2RGBA);
        }
        else if (frame.channels() == 4)
        {
            // Already RGBA
            rgbaFrame = frame.clone();
        }
        else
        {
            LOG_ERR("Unsupported number of channels: " + std::to_string(frame.channels()));
            return false;
        }

        // Write frame to shared memory
        bool success = this->shmVideoHandler->writeFrame(
            rgbaFrame,
            frameCount++,
            0,    // Total frames (0 for streaming)
            30.0, // FPS
            timestamp);

        return success;
    }

    TextureVideo *ProducerApp::getVideoTexture() const
    {
        return videoTexture.get();
    }

    // bool ProducerApp::initializeDmaBufProducer(GLFWwindow *window, const std::string &dummyPath,
    //                                            uint32_t width, uint32_t height)
    // {
    //     this->isVideo = true;
    //     this->mode = "dma";
    //     context.init(window);

    //     LOG_INFO("Initializing DMA-BUF producer with dimensions: " +
    //              std::to_string(width) + "x" + std::to_string(height));

    //     // LOG_INFO("Creating texture with size: " + std::to_string(width) + "x" + std::to_string(height));
    //     LOG_INFO("Texture usage flags: " + std::to_string(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));

    //     // Create a texture for the video
    //     videoTexture = std::make_unique<TextureVideo>(context);
    //     if (!videoTexture->createFromSize(width, height))
    //     {
    //         throw std::runtime_error("Failed to create video texture");
    //     }

    //     // Setup descriptors and pipeline for rendering
    //     createDescriptorPool(context.getDevice(), descriptorPool);

    //     // Initialize descriptor manager with video texture
    //     TextureImage texture;
    //     texture.image = videoTexture->getImage();
    //     texture.memory = videoTexture->getMemory();
    //     texture.view = videoTexture->getImageView();
    //     texture.sampler = videoTexture->getSampler();
    //     texture.width = width;
    //     texture.height = height;

    //     descriptorManager.init(context.getDevice(), descriptorPool, texture);
    //     pipeline.create(context.getDevice(), context.getSwapchainExtent(),
    //                     context.getRenderPass(), descriptorManager.getLayout());

    //     createVertexBuffer(
    //         context.getDevice(),
    //         context.getPhysicalDevice(),
    //         vertexBuffer,
    //         vertexBufferMemory,
    //         FULLSCREEN_QUAD);

    //     LOG_INFO("DMA-BUF producer initialized with dimensions: " +
    //              std::to_string(width) + "x" + std::to_string(height));
    //     return true;
    // }

    bool ProducerApp::initializeDmaBufProducer(GLFWwindow *window, const std::string &dummyPath,
                                               uint32_t width, uint32_t height)
    {
        this->isVideo = true;
        this->mode = "dma";

        // Initialize Vulkan context with the window
        try
        {
            LOG_INFO("Initializing Vulkan context with window");
            context.init(window);
        }
        catch (const std::exception &e)
        {
            LOG_ERR("Failed to initialize Vulkan context: " + std::string(e.what()));
            return false;
        }

        LOG_INFO("=== Initializing DMA-BUF producer ===");
        LOG_INFO("Dimensions: " + std::to_string(width) + "x" + std::to_string(height));

        // Create a texture for the video
        videoTexture = std::make_unique<TextureVideo>(context);

        LOG_INFO("Creating video texture with size: " + std::to_string(width) + "x" + std::to_string(height));
        if (!videoTexture->createFromSize(width, height))
        {
            LOG_ERR("Failed to create video texture");
            return false;
        }

        LOG_INFO("Video texture created successfully");
        LOG_INFO("Image handle: " + std::to_string(reinterpret_cast<uint64_t>(videoTexture->getImage())));
        LOG_INFO("Memory handle: " + std::to_string(reinterpret_cast<uint64_t>(videoTexture->getMemory())));

        // Setup descriptors and pipeline for rendering
        LOG_INFO("Creating descriptor pool");
        createDescriptorPool(context.getDevice(), descriptorPool);

        // Initialize descriptor manager with video texture
        LOG_INFO("Initializing descriptor manager");
        TextureImage texture;
        texture.image = videoTexture->getImage();
        texture.memory = videoTexture->getMemory();
        texture.view = videoTexture->getImageView();
        texture.sampler = videoTexture->getSampler();
        texture.width = width;
        texture.height = height;

        descriptorManager.init(context.getDevice(), descriptorPool, texture);

        LOG_INFO("Creating pipeline");
        pipeline.create(context.getDevice(), context.getSwapchainExtent(),
                        context.getRenderPass(), descriptorManager.getLayout());

        LOG_INFO("Creating vertex buffer");
        createVertexBuffer(
            context.getDevice(),
            context.getPhysicalDevice(),
            vertexBuffer,
            vertexBufferMemory,
            FULLSCREEN_QUAD);

        LOG_INFO("=== DMA-BUF producer initialized successfully ===");
        return true;
    }

    void ProducerApp::cleanup()
    {
        LOG_INFO("Starting cleanup process...");

        // First, stop any running activity
        running = false;

        if (this->mode == "dma")
        {
            if (this->isVideo)
            {
                LOG_INFO("Cleaning up video resources shared in dma mode...");

                // close dma socket
                if (this->mode == "dma")
                {
                    m_socketServerRunning = false;
                    if (m_serverSocketFd >= 0)
                    {
                        close(m_serverSocketFd);
                        m_serverSocketFd = -1;
                    }
                }

                // Close the video loader
                if (videoLoader)
                {
                    videoLoader->close();
                    videoLoader.reset();
                }

                // Clean up texture resources
                if (videoTexture)
                {
                    videoTexture->destroy();
                    videoTexture.reset();
                }

                // Clean up Vulkan resources
                if (vertexBuffer != VK_NULL_HANDLE)
                {
                    vkDestroyBuffer(context.getDevice(), vertexBuffer, nullptr);
                    vertexBuffer = VK_NULL_HANDLE;
                }

                if (vertexBufferMemory != VK_NULL_HANDLE)
                {
                    vkFreeMemory(context.getDevice(), vertexBufferMemory, nullptr);
                    vertexBufferMemory = VK_NULL_HANDLE;
                }

                // Clean up descriptors
                descriptorManager.cleanup(context.getDevice());

                if (descriptorPool != VK_NULL_HANDLE)
                {
                    vkDestroyDescriptorPool(context.getDevice(), descriptorPool, nullptr);
                    descriptorPool = VK_NULL_HANDLE;
                }

                // Clean up pipeline
                pipeline.cleanup(context.getDevice());

                // Clean up socket
                if (!this->shmName.empty())
                {
                    ipc::cleanup_unix_socket(this->shmName);
                    LOG_INFO("Cleaned up socket: " + this->shmName);
                }

                // Clean up Vulkan context
                context.cleanup();
            }
            else
            {
                // Existing image cleanup code
                LOG_INFO("Cleaning up image resources shared in dma mode...");
                vkDestroyBuffer(context.getDevice(), vertexBuffer, nullptr);
                vkFreeMemory(context.getDevice(), vertexBufferMemory, nullptr);
                descriptorManager.cleanup(context.getDevice());
                pipeline.cleanup(context.getDevice());
                vkDestroyDescriptorPool(context.getDevice(), descriptorPool, nullptr);
                context.cleanup();
                LOG_INFO("Socket Name: " + this->shmName);
                ipc::cleanup_unix_socket(this->shmName);
            }
        }
        else if (this->mode == "shm")
        {
            if (this->isVideo)
            {
                LOG_INFO("Cleaning up video resources shared in shm mode...");

                // Close any OpenCV windows
                try
                {
                    cv::destroyAllWindows();
                    cv::waitKey(1); // Process the destroy window event
                }
                catch (...)
                {
                    LOG_ERR("Error destroying OpenCV windows");
                }

                // Close the video loader
                if (videoLoader)
                {
                    try
                    {
                        videoLoader->close();
                    }
                    catch (const std::exception &e)
                    {
                        LOG_ERR("Error closing video loader: " + std::string(e.what()));
                    }
                    videoLoader.reset();
                }

                // Close the shared memory handler
                if (shmVideoHandler)
                {
                    try
                    {
                        shmVideoHandler->closeSharedMemory();
                    }
                    catch (const std::exception &e)
                    {
                        LOG_ERR("Error closing shared memory handler: " + std::string(e.what()));
                    }
                    shmVideoHandler.reset();
                }

                // Clean up shared memory
                if (!this->shmName.empty())
                {
                    LOG_INFO("Removing shared memory segment: " + this->shmName);
                    try
                    {
                        shm_unlink(this->shmName.c_str());
                    }
                    catch (...)
                    {
                        LOG_ERR("Error unlinking shared memory");
                    }
                }
            }
            else
            {
                // Your existing SHM image cleanup code
                LOG_INFO("Cleaning up image resources shared in shm mode...");
                try
                {
                    vst::shm::destroy_viewer(this->shmName);
                }
                catch (const std::exception &e)
                {
                    LOG_ERR("Error in SHM viewer cleanup: " + std::string(e.what()));
                }
            }
        }
        else
        {
            LOG_ERR("Invalid cleanup mode: " + this->mode);
        }

        LOG_INFO("Cleanup completed");
    }

    ProducerApp::~ProducerApp()
    {
        cleanup();
    }

} // namespace vst
