#include "media/video_loader.hpp"
#include "media/video_info.hpp"
#include "utils/logger.hpp"

namespace vst
{
    VideoLoader::VideoLoader() {}

    VideoLoader::~VideoLoader() { close(); }

    bool VideoLoader::open(const std::string &filepath)
    {
        close();

        // Open with explicit backend to avoid codec issues
        capture.open(filepath, cv::CAP_FFMPEG);

        if (!capture.isOpened())
        {
            LOG_ERR("Failed to open video file: " + filepath);
            return false;
        }

        // Try to read the first frame to verify the video opened correctly
        cv::Mat testFrame;
        bool success = capture.read(testFrame);

        if (!success || testFrame.empty())
        {
            LOG_ERR("Failed to read first frame from video: " + filepath);
            capture.release();
            return false;
        }

        // Reset to the beginning
        capture.set(cv::CAP_PROP_POS_FRAMES, 0);

        LOG_INFO("Successfully opened video: " + filepath +
                 " (" + std::to_string(capture.get(cv::CAP_PROP_FRAME_WIDTH)) + "x" +
                 std::to_string(capture.get(cv::CAP_PROP_FRAME_HEIGHT)) + " @ " +
                 std::to_string(capture.get(cv::CAP_PROP_FPS)) + " fps)");

        return true;
    }

    bool VideoLoader::grabFrame(cv::Mat &outputFrame)
    {
        if (!capture.isOpened())
        {
            LOG_ERR("VideoLoader: Capture is not open");
            return false;
        }

        bool success = capture.read(outputFrame);

        if (!success || outputFrame.empty())
        {
            // Log only when we reach the end of the video, not for every failed frame
            if (capture.get(cv::CAP_PROP_POS_FRAMES) >= capture.get(cv::CAP_PROP_FRAME_COUNT) - 1)
            {
                LOG_INFO("Reached end of video");
            }
            else
            {
                LOG_ERR("Failed to read frame at position " +
                        std::to_string(capture.get(cv::CAP_PROP_POS_FRAMES)));
            }
            return false;
        }

        return true;
    }

    void VideoLoader::close()
    {
        if (capture.isOpened())
        {
            capture.release();
            LOG_INFO("Video capture closed");
        }
    }

    bool VideoLoader::isOpened() const
    {
        return capture.isOpened();
    }

    vst::VideoInfo vst::VideoLoader::getVideoResolution(const std::string &path)
    {
        vst::VideoInfo info;
        cv::VideoCapture cap(path, cv::CAP_FFMPEG);

        if (!cap.isOpened())
        {
            LOG_ERR("Unable to open video to get resolution: " + path);
            return info;
        }

        info.width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
        info.height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
        info.valid = (info.width > 0 && info.height > 0);

        cap.release();
        return info;
    }
} // namespace vst