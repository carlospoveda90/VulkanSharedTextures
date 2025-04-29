#pragma once
#include <string>
#include <optional>

namespace vst::utils
{
    struct ImageSize
    {
        int width;
        int height;
        int channels;
    };
    struct SharedResource
    {
        std::string path;
        std::string mode;
        std::string type;
        vst::utils::ImageSize dimensions; 
    };

    ImageSize getImageSize(const std::string &imagePath);
    ImageSize parseImageDimensions(const std::string &filename);
    std::string getFileName(const std::string &path);
    std::string getFileExtension(const std::string &path);
    std::optional<std::string> findLatestShmFile();
    std::optional<std::string> find_shared_image_file();
    std::optional<std::string> findLatestVideoShmFile();
    std::optional<std::string> findLatestVideoDmaSocket();
    std::optional<SharedResource> findSharedResource();
}
