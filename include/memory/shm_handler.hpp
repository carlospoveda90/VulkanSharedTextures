// shm_handler.hpp
#pragma once

#include <string>
#include <cstddef>

namespace vst
{

    class ShmHandler
    {
    public:
        static int createShm(const std::string &name, size_t size);
        static void *mapShm(const std::string &name, size_t size);
        static void unmapShm(void *addr, size_t size);
        static void destroyShm(const std::string &name);
    };

} // namespace vst
