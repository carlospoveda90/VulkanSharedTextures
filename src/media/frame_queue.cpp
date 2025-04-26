#include "media/frame_queue.hpp"

namespace vst
{

    void FrameQueue::push(const VideoFrame &frame)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        latest_ = frame;
    }

    std::optional<VideoFrame> FrameQueue::pop()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto result = latest_;
        latest_.reset(); // mark as consumed
        return result;
    }

} // namespace vst
