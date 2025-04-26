#pragma once
#include "media/video_frame.hpp"
#include <mutex>
#include <optional>

namespace vst
{

    class FrameQueue
    {
    public:
        void push(const VideoFrame &frame);
        std::optional<VideoFrame> pop();

    private:
        std::mutex mutex_;
        std::optional<VideoFrame> latest_;
    };

} // namespace vst
