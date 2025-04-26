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

namespace vst
{
    class ProducerApp
    {
    public:
        ProducerApp();
        // ProducerApp(VulkanContext &context);
        ~ProducerApp();

        void ProducerDMA(GLFWwindow *window, const std::string &imagePath, const std::string &mode, bool isVideo);
        void ProducerSHM(const std::string &imagePath, const std::string &mode, bool isVideo);

        void runFrame();
        void runFrame(const std::string &imagePath);
        void cleanup();

        // video methods
        // bool initialize(const std::string &videoPath);
        // void update();
        // void cleanup();

        // void startVideoStreaming(const std::string &videoPath, Texture texture);
        // void stopVideoStreaming();
        // void updateFrame();

    private:
        FrameQueue frameQueue;
        std::thread decodeThread;
        std::atomic<bool> decodingDone = false;
        bool videoMode = false;
        // VulkanContext context;
        // TextureImage texture;
        Pipeline pipeline;
        std::string mode;
        std::string shmName;
        cv::VideoCapture cap;
        vst::utils::ImageSize imageData;
        std::atomic<bool> running = true;
        
        // video variables
        // VulkanContext *context = nullptr;
        VulkanContext &context;
        std::unique_ptr<VideoLoader> videoLoader;
        std::unique_ptr<TextureVideo> videoTexture;
        bool videoEnded = false;

        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        DescriptorManager descriptorManager;
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    };

}
