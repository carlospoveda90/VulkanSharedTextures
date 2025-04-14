#pragma once
#include <string>

namespace vst::shm
{
    bool write_to_shm(const std::string &shmName, const void *data, size_t size);
}
