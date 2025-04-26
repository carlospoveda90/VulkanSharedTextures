#pragma once
#include <cstdint>
#include <vector>

namespace vst
{

    struct VideoFrame
    {
        std::vector<uint8_t> pixels; // raw RGB24 or RGBA
        int width = 0;
        int height = 0;
    };

} // namespace vst
