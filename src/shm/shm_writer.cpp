#include "shm/shm_writer.hpp"
#include "memory/shm_handler.hpp"
#include "utils/file_utils.hpp"
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <cstdint>

namespace vst::shm
{

    bool write_to_shm(const std::string &shmName, vst::utils::ImageSize imageData, const void *data, size_t size)
    {
        int fd = shm_open(shmName.c_str(), O_CREAT | O_RDWR, 0666);
        if (fd < 0)
        {
            perror("shm_open");
            return false;
        }

        if (ftruncate(fd, size) == -1)
        {
            perror("ftruncate");
            close(fd);
            return false;
        }

        void *mapped = mmap(nullptr, size, PROT_WRITE, MAP_SHARED, fd, 0);
        if (mapped == MAP_FAILED)
        {
            perror("mmap");
            close(fd);
            return false;
        }

        std::memcpy(mapped, data, size);
        munmap(mapped, size);
        close(fd);
        return true;
    }

} // namespace vst::shm
