// #include "media/video_loader.hpp"
// #include "media/video_info.hpp"
// #include "core/vulkan_utils.hpp"
// #include "media/video_frame.hpp"

// #include <opencv2/core.hpp>
// #include <opencv2/highgui.hpp>
// #include <opencv2/imgproc.hpp>
// #include <opencv2/videoio.hpp>
// #include <iostream>
// #include <cstring>
// #include <stdexcept>
// #include <thread>

// using namespace vst::vulkan_utils;

// namespace vst
// {
//     vst::VideoInfo vst::VideoLoader::getVideoResolution(const std::string &path)
//     {
//         vst::VideoInfo info;
//         cv::VideoCapture cap(path);

//         if (!cap.isOpened())
//             return info;

//         info.width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
//         info.height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
//         info.valid = (info.width > 0 && info.height > 0);

//         return info;
//     }

//     // bool VideoLoader::extractFrames(const std::string &path, std::function<void(VideoFrame)> callback) {
//     //     cv::VideoCapture cap(path);
//     //     if (!cap.isOpened()) {
//     //         std::cerr << "[ERROR] Failed to open video file: " << path << std::endl;
//     //         return false;
//     //     }

//     //     cv::Mat frame;
//     //     while (cap.read(frame)) {
//     //         if (frame.empty()) continue;

//     //         VideoFrame videoFrame;
//     //         videoFrame.width = frame.cols;
//     //         videoFrame.height = frame.rows;
//     //         videoFrame.pixels.assign(frame.data, frame.data + frame.cols * frame.rows * frame.channels());

//     //         callback(videoFrame);
//     //         std::this_thread::sleep_for(std::chrono::milliseconds(33)); // simulate ~30 FPS
//     //     }

//     //     return true;
//     // }

//     bool VideoLoader::extractFrames(const std::string &path, std::function<void(VideoFrame)> callback)
//     {
//         cv::VideoCapture cap(path);
//         if (!cap.isOpened())
//         {
//             std::cerr << "[ERROR] Failed to open video: " << path << std::endl;
//             return false;
//         }

//         cv::Mat frame;
//         while (cap.read(frame))
//         {
//             if (frame.empty())
//                 continue;

//             // Convert BGR to RGB
//             cv::Mat rgb;
//             cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);

//             VideoFrame videoFrame;
//             videoFrame.width = rgb.cols;
//             videoFrame.height = rgb.rows;
//             videoFrame.pixels.assign(rgb.data, rgb.data + rgb.total() * rgb.elemSize());

//             callback(std::move(videoFrame));
//             std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30 FPS
//         }

//         cap.release();
//         return true;
//     }

//     Texture VideoLoader::loadTexture(const uint8_t *frameData, int width, int height,
//                                      VkDevice device,
//                                      VkPhysicalDevice physicalDevice,
//                                      VkCommandPool commandPool,
//                                      VkQueue graphicsQueue,
//                                      bool exportMemory)
//     {
//         VkDeviceSize imageSize = width * height * 4;

//         VkBuffer stagingBuffer;
//         VkDeviceMemory stagingMemory;

//         createBuffer(device, physicalDevice, imageSize, stagingBuffer, stagingMemory,
//                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
//                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

//         void *mappedData;
//         vkMapMemory(device, stagingMemory, 0, imageSize, 0, &mappedData);
//         std::memcpy(mappedData, frameData, static_cast<size_t>(imageSize));
//         vkUnmapMemory(device, stagingMemory);

//         Texture texture;
//         texture.width = width;
//         texture.height = height;

//         createImage(device, physicalDevice, width, height, texture.image, texture.memory, exportMemory);

//         transitionImageLayout(device, commandPool, graphicsQueue,
//                               texture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

//         copyBufferToImage(device, commandPool, graphicsQueue,
//                           stagingBuffer, texture.image, width, height);

//         transitionImageLayout(device, commandPool, graphicsQueue,
//                               texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

//         texture.view = createImageView(device, texture.image);

//         texture.sampler = createSampler(device);

//         vkDestroyBuffer(device, stagingBuffer, nullptr);
//         vkFreeMemory(device, stagingMemory, nullptr);

//         return texture;
//     }

// } // namespace vst

#include "media/video_loader.hpp"
#include "media/video_info.hpp"

namespace vst
{

    VideoLoader::VideoLoader() {}
    VideoLoader::~VideoLoader() { close(); }

    bool VideoLoader::open(const std::string &filepath)
    {
        close();
        capture.open(filepath);
        return capture.isOpened();
    }

    bool VideoLoader::grabFrame(cv::Mat &outputFrame)
    {
        if (!capture.isOpened())
        {
            return false;
        }
        return capture.read(outputFrame);
    }

    void VideoLoader::close()
    {
        if (capture.isOpened())
        {
            capture.release();
        }
    }

    bool VideoLoader::isOpened() const
    {
        return capture.isOpened();
    }

    vst::VideoInfo vst::VideoLoader::getVideoResolution(const std::string &path)
    {
        vst::VideoInfo info;
        cv::VideoCapture cap(path);

        if (!cap.isOpened())
            return info;

        info.width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
        info.height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
        info.valid = (info.width > 0 && info.height > 0);

        return info;
    }

} // namespace vst
