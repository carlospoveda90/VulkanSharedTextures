#pragma once
#include <string>
namespace vst::shm
{
    void run_viewer(const char *shmName, const std::string &owner);
    void destroy_viewer(const std::string &shmName);
}
