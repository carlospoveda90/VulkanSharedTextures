// shm_handler.cpp
#include "memory/shm_handler.hpp"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>
#include <iostream>

namespace vst
{

    int ShmHandler::createShm(const std::string &name, size_t size)
    {
        int fd = shm_open(name.c_str(), O_CREAT | O_RDWR, 0666);
        if (fd == -1)
        {
            throw std::runtime_error("Failed to create shared memory: " + name);
        }

        if (ftruncate(fd, size) == -1)
        {
            close(fd);
            throw std::runtime_error("Failed to set size on shared memory: " + name);
        }

        return fd;
    }

    void *ShmHandler::mapShm(const std::string &name, size_t size)
    {
        int fd = shm_open(name.c_str(), O_RDWR, 0666);
        
        // std::cout << "fd: " << fd << std::endl;
        // std::cout<<"name: " << name << std::endl;
        // std::cout<<"size: " << size << std::endl;
        // std::cout<<"O_RDWR: " << O_RDWR << std::endl;
        // std::cout<<"0666: " << 0666 << std::endl;
        // std::cout<<"shm_open: " << shm_open(name.c_str(), O_RDWR, 0666) << std::endl;
        
        if (fd == -1)
        {
            throw std::runtime_error("Failed to open shared memory: " + name);
        }

        void *addr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        close(fd);

        if (addr == MAP_FAILED)
        {
            throw std::runtime_error("Failed to map shared memory: " + name);
        }

        return addr;
    }

    void ShmHandler::unmapShm(void *addr, size_t size)
    {
        munmap(addr, size);
    }

    void ShmHandler::destroyShm(const std::string &name)
    {
        shm_unlink(name.c_str());
    }

} // namespace vst
