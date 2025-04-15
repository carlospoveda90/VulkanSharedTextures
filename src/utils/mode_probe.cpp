#include "utils/mode_probe.hpp"
#include <sys/stat.h>
#include <stdexcept>

namespace vst::utils
{

    std::string detect_producer_mode()
    {
        struct stat sb;

        if (stat("/tmp/vulkan_shared.sock", &sb) == 0 && S_ISSOCK(sb.st_mode))
        {
            return "dma";
        }

        if (stat("/dev/shm/vst_shared_texture", &sb) == 0)
        {
            return "shm";
        }

        throw std::runtime_error("Failed to detect producer mode: no known resources found.");
    }

} // namespace vst::utils
