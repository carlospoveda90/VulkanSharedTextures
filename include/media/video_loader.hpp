// #pragma once

// #include <functional>
// #include <string>
// #include <vulkan/vulkan.h>
// #include "media/texture.hpp"
// #include "media/video_info.hpp"
// #include "media/video_frame.hpp"

// #include <opencv2/core.hpp>
// #include <opencv2/highgui.hpp>
// #include <opencv2/imgproc.hpp>
// #include <opencv2/videoio.hpp>

// namespace vst
// {

//     class VideoLoader
//     {
//     public:
//         static Texture loadTexture(const uint8_t *frameData, int width, int height,
//                                    VkDevice device,
//                                    VkPhysicalDevice physicalDevice,
//                                    VkCommandPool commandPool,
//                                    VkQueue graphicsQueue,
//                                    bool exportMemory = false);

//         // static bool extractFrames(const std::string &path, std::function<void(uint8_t *data, int width, int height)> callback);
//         static bool extractFrames(const std::string &path, std::function<void(VideoFrame)> callback);
//         static VideoInfo getVideoResolution(const std::string &path);
//     };

// } // namespace vst

#pragma once

#include "media/video_info.hpp"
#include <opencv2/opencv.hpp>
#include <string>

namespace vst
{
    class VideoLoader
    {
    public:
        VideoLoader();
        ~VideoLoader();

        bool open(const std::string &filepath);
        bool grabFrame(cv::Mat &outputFrame);
        void close();
        static VideoInfo getVideoResolution(const std::string &path);
        bool isOpened() const;

    private:
        cv::VideoCapture capture;
    };
} // namespace vst
