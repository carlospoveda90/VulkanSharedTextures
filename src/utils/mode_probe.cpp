#include "utils/mode_probe.hpp"
#include "utils/file_utils.hpp"
#include <sys/stat.h>
#include <stdexcept>

namespace vst::utils
{

    std::string detect_producer_mode()
    {
        std::optional<SharedResource> pathOpt = findSharedResource();

        // if (!pathOpt.has_value())
        if (!pathOpt->path.empty())
        {
            throw std::runtime_error("Failed to detect producer mode: no known resources found.");
        }

        const std::string &path = pathOpt->path;

        if (path.find("/dev/shm/") == 0)
        {
            return "shm";
        }
        else if (path.find("/tmp/") == 0)
        {
            return "dma";
        }

        throw std::runtime_error("Unknown producer resource path: " + path);
    }

} // namespace vst::utils
