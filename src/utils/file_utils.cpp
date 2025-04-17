#include "utils/file_utils.hpp"
#include <string>
#include <tuple>
#include <stdexcept>
#include <sstream>
#include <cstdint>
#include <stb_image.h>

namespace vst::utils
{
    ImageSize getImageSize(const std::string &imagePath)
    {
        int w, h, c;
        stbi_info(imagePath.c_str(), &w, &h, &c);
        return {w, h, c};
    }

    ImageSize parseImageDimensions(const std::string &filename)
    {
        size_t dashPos = filename.find_last_of('-');
        size_t xPos = filename.find('x', dashPos);

        if (dashPos == std::string::npos || xPos == std::string::npos)
        {
            throw std::runtime_error("Filename format invalid: " + filename);
        }

        std::string widthStr = filename.substr(dashPos + 1, xPos - dashPos - 1);
        std::string heightStr = filename.substr(xPos + 1);

        uint32_t width = static_cast<uint32_t>(std::stoi(widthStr));
        uint32_t height = static_cast<uint32_t>(std::stoi(heightStr));

        return {static_cast<int>(width), static_cast<int>(height), 0}; // Assuming channels is not needed here
    }

    std::string getFileName(const std::string &path)
    {
        size_t lastSlash = path.find_last_of("/\\");
        if (lastSlash == std::string::npos)
            return path;
        return path.substr(lastSlash + 1);
    }

    std::string getFileExtension(const std::string &path)
    {
        size_t lastDot = path.find_last_of('.');
        if (lastDot == std::string::npos)
            return "";
        return path.substr(lastDot + 1);
    }

} // namespace vst::utils