#pragma once

#include "core/vulkan_context.hpp"
#include "core/pipeline.hpp"
#include "core/descriptor_manager.hpp"
#include "core/vertex_definitions.hpp"
#include "memory/shm_video_handler.hpp"
#include <opencv2/opencv.hpp>
#include <atomic>
#include <string>
#include <memory>

namespace vst
{
    class ConsumerApp
    {
    public:
        ConsumerApp();
        ConsumerApp(const std::string &mode);
        ConsumerApp(GLFWwindow *window, const std::string &shmName, const std::string &mode, bool isVideo);
        ~ConsumerApp();

        void runFrame();
        void cleanup();

        // New method for SHM video consumption
        bool consumeShmVideo(const std::string &shmName);

        // Run the video loop for SHM video
        void runVideoLoop();

        // Check if video consumption is still running
        bool isVideoRunning() const { return m_videoRunning; }

        // Integration with demo application
        bool readFrame(cv::Mat &frame);

        VkImage getImportedImage() const;

        // Get the Vulkan Context
        const VulkanContext &getContext() const
        {
            return context;
        }

    private:
        VulkanContext context;

        int dmaBufFd;
        uint32_t imageWidth;
        uint32_t imageHeight;

        VkImage importedImage = VK_NULL_HANDLE;
        VkDeviceMemory importedMemory = VK_NULL_HANDLE;
        VkImageView importedImageView = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        DescriptorManager descriptorManager;
        Pipeline pipeline;
        std::string mode;
        std::string shmName;
        bool isVideo = false;

        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;

        // Video-specific members
        std::shared_ptr<memory::ShmVideoHandler> m_shmVideoHandler;
        std::string m_videoWindowTitle;
        std::atomic<bool> m_videoRunning{false};
        double m_videoFrameRate = 30.0;
    };
}