#pragma once
#include <string>
#include "utils/file_utils.hpp"

namespace vst::shm
{
    bool write_to_shm(const std::string &shmName, vst::utils::ImageSize imageData, const void *data, size_t size);
}
