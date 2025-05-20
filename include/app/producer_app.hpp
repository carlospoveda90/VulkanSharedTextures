#pragma once

#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include "core/pipeline.hpp"
#include "core/descriptor_manager.hpp"
#include "core/vulkan_context.hpp"
#include "media/image_loader.hpp"
#include "media/texture_video.hpp"
#include "window/iapp_window.hpp"
#include "utils/file_utils.hpp"
#include "media/frame_queue.hpp"
#include "media/video_loader.hpp"
#include "memory/shm_video_handler.hpp"

namespace vst
{
    class ProducerApp
    {
    public:
        ProducerApp();
        // ProducerApp(VulkanContext &context);
        ProducerApp(const VulkanContext &context);
        ~ProducerApp();

        void ProducerDMA(GLFWwindow *window, const std::string &imagePath, const std::string &mode, bool isVideo);
        void ProducerSHM(const std::string &imagePath, const std::string &mode, bool isVideo);

        void runFrame();
        void runFrame(const std::string &imagePath);
        void cleanup();

        // Video methods
        void update();
        VideoLoader *getVideoLoader() { return videoLoader.get(); }
        std::shared_ptr<memory::ShmVideoHandler> getSharedMemoryHandler() { return shmVideoHandler; }
        std::string getWindowTitle() { return windowTitle; }

        bool isDecodingDone() const { return decodingDone; }
        bool isRunning() const { return running; }

        // Integration with demo application
        bool initSharedMemory(const std::string &name, int width, int height);
        bool writeFrame(const cv::Mat &frame);

        // validate socket connections
        bool setupDmaSocket(const std::string &socketPath, int fd, uint32_t width, uint32_t height);
        void checkForConnections();

        // Add to producer_app.hpp
        std::shared_ptr<vst::memory::ShmVideoHandler> getShmVideoHandler() const
        {
            return shmVideoHandler;
        }

        // Set the shared memory video handler
        void setShmVideoHandler(std::shared_ptr<vst::memory::ShmVideoHandler> handler)
        {
            shmVideoHandler = handler;
        }

        // Get the Vulkan pipeline
        TextureVideo *getVideoTexture() const;

        // Get the Vulkan Context
        const VulkanContext &getContext() const
        {
            return context;
        }

        // initialize the DMA buffer producer
        bool initializeDmaBufProducer(GLFWwindow *window, const std::string &dummyPath,
                                      uint32_t width, uint32_t height);

        // update the texture from the computed frame
        bool updateFromComputedTexture();

        // Copy texture from image
        bool copyTextureFromImage(VkImage sourceImage);

    private:
        FrameQueue frameQueue;
        std::thread decodeThread;
        std::atomic<bool> decodingDone = false;
        bool isVideo = false;
        Pipeline pipeline;
        std::string mode;
        std::string shmName;
        std::string windowTitle;
        cv::VideoCapture cap;
        vst::utils::ImageSize imageData;
        std::atomic<bool> running = true;

        // dma socket connections validation variables
        bool m_socketServerRunning = false;
        int m_serverSocketFd = -1;
        int m_dmaFd = -1; // Store the DMA-BUF file descriptor
        uint32_t m_texWidth = 0;
        uint32_t m_texHeight = 0;

        // Video variables
        VulkanContext &context;
        std::unique_ptr<VideoLoader> videoLoader;
        std::unique_ptr<TextureVideo> videoTexture;
        std::shared_ptr<memory::ShmVideoHandler> shmVideoHandler;
        bool videoEnded = false;

        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        DescriptorManager descriptorManager;
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    };
}