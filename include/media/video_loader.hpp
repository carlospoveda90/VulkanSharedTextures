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
        bool isOpened() const;

        // Add this method to expose the VideoCapture object
        cv::VideoCapture& getCapture() { return capture; }

        // Static method to get video resolution
        static VideoInfo getVideoResolution(const std::string &path);
        
    private:
        cv::VideoCapture capture;
    };
} // namespace vst
