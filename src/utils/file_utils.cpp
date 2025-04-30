#include "utils/file_utils.hpp"
#include <string>
#include <tuple>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <cstdint>
#include <stb_image.h>
#include <optional>
#include <regex>
#include <filesystem>

namespace fs = std::filesystem;

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

        return {static_cast<int>(width), static_cast<int>(height), 0};
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

    std::optional<std::string> findLatestShmFile()
    {
        std::string prefix = "/dev/shm/vst_shared_texture-";
        std::regex pattern("vst_shared_texture-(\\d+)x(\\d+)");

        for (const auto &entry : std::filesystem::directory_iterator("/dev/shm"))
        {
            const std::string name = entry.path().filename().string();
            if (std::regex_match(name, pattern))
            {
                return prefix + name.substr(std::string("vst_shared_texture-").length());
            }
        }
        return std::nullopt; // not found
    }

    std::optional<std::string> findLatestVideoDmaSocket()
    {
        std::string prefix = "/tmp/vulkan_shared_";
        std::regex pattern("vulkan_shared_video-(\\d+)x(\\d+).sock");

        for (const auto &entry : std::filesystem::directory_iterator("/tmp"))
        {
            const std::string name = entry.path().filename().string();
            if (std::regex_match(name, pattern))
            {
                return prefix + name.substr(std::string("vulkan_shared-").length());
            }
        }
        return std::nullopt;
    }

    std::optional<std::string> findLatestVideoShmFile()
    {
        std::string basePath = "/dev/shm";
        std::regex videoPattern("vst_shared_video-(\\d+)x(\\d+)");
        std::vector<fs::directory_entry> matches;

        try
        {
            for (const auto &entry : fs::directory_iterator(basePath))
            {
                std::string filename = entry.path().filename().string();
                if (std::regex_match(filename, videoPattern))
                {
                    matches.push_back(entry);
                }
            }
        }
        catch (const fs::filesystem_error &e)
        {
            std::cout << "[INFO] Filesystem error: " << e.what() << "\n";
            return std::nullopt;
        }

        // Sort by modification time (newest first)
        std::sort(matches.begin(), matches.end(),
                  [](const fs::directory_entry &a, const fs::directory_entry &b)
                  {
                      return fs::last_write_time(a) > fs::last_write_time(b);
                  });

        if (!matches.empty())
        {
            std::string path = matches[0].path().filename().string();
            std::cout << "[INFO] Found video shared memory: " << path << "\n";
            return path;
        }

        return std::nullopt;
    }

    std::optional<std::string> find_shared_image_file()
    {
        const std::regex shmPattern("vst_shared_texture-(\\d+)x(\\d+)");
        const std::regex dmaPattern("vulkan_shared_image-(\\d+)x(\\d+).sock");

        // Check SHM in /dev/shm
        for (const auto &entry : std::filesystem::directory_iterator("/dev/shm"))
        {
            const std::string name = entry.path().filename().string();
            if (std::regex_match(name, shmPattern))
            {
                return std::string("/dev/shm/") + name;
            }
        }

        // Check DMA socket in /tmp
        for (const auto &entry : std::filesystem::directory_iterator("/tmp"))
        {
            const std::string name = entry.path().filename().string();
            // std::cout << "[DMA] File : " << name << '\n';
            if (std::regex_match(name, dmaPattern))
            {
                return std::string("/tmp/") + name;
            }
        }

        return std::nullopt;
    }

    std::optional<SharedResource> findSharedResource()
    {
        SharedResource resource;

        // Check for SHM video first
        const std::regex shmVideoPattern("vst_shared_video-(\\d+)x(\\d+)");
        for (const auto &entry : std::filesystem::directory_iterator("/dev/shm"))
        {
            const std::string name = entry.path().filename().string();
            if (std::regex_match(name, shmVideoPattern))
            {
                resource.path = entry.path().string();
                resource.mode = "shm";
                resource.type = "video";

                // Extract dimensions from filename
                try
                {
                    resource.dimensions = parseImageDimensions(name);
                    std::cout << "Found shared video via SHM: " << name + " (" + std::to_string(resource.dimensions.width) + "x" + 
                                        std::to_string(resource.dimensions.height) + ")" << "\n";
                    return resource;
                }
                catch (const std::exception &e)
                {
                    std::cout << "Error parsing dimensions: " + std::string(e.what()) << "\n";
                }
            }
        }

        // Check for SHM image
        const std::regex shmImagePattern("vst_shared_texture-(\\d+)x(\\d+)");
        for (const auto &entry : std::filesystem::directory_iterator("/dev/shm"))
        {
            const std::string name = entry.path().filename().string();
            if (std::regex_match(name, shmImagePattern))
            {
                resource.path = entry.path().string();
                resource.mode = "shm";
                resource.type = "image";

                // Extract dimensions from filename
                try
                {
                    resource.dimensions = parseImageDimensions(name);
                    std::cout << "Found shared image via SHM: " + name +
                                     " (" + std::to_string(resource.dimensions.width) + "x" +
                                     std::to_string(resource.dimensions.height) + ")"
                              << "\n";
                    return resource;
                }
                catch (const std::exception &e)
                {
                    std::cout << "Error parsing dimensions: " + std::string(e.what()) << "\n";
                }
            }
        }

        // Check for DMA-BUF socket (image only, as video is not implemented for DMA)
        const std::regex dmaPattern("vulkan_shared_image-(\\d+)x(\\d+).sock");
        for (const auto &entry : std::filesystem::directory_iterator("/tmp"))
        {
            const std::string name = entry.path().filename().string();
            if (std::regex_match(name, dmaPattern))
            {
                resource.path = "/tmp/" + name;
                resource.mode = "dma";
                resource.type = "image";

                // Extract dimensions from filename
                try
                {
                    resource.dimensions = parseImageDimensions(name);
                    std::cout << "Found shared image via DMA-BUF: " + name +
                                     " (" + std::to_string(resource.dimensions.width) + "x" +
                                     std::to_string(resource.dimensions.height) + ")"
                              << "\n";
                    return resource;
                }
                catch (const std::exception &e)
                {
                    std::cout << "Error parsing dimensions: " + std::string(e.what()) << "\n";
                }
            }
        }

        // Check for DMA-BUF video socket (same pattern as image but will be updated by the producer)
        const std::regex dmaVideoPattern("vulkan_shared_video-(\\d+)x(\\d+).sock");
        for (const auto &entry : std::filesystem::directory_iterator("/tmp"))
        {
            const std::string name = entry.path().filename().string();
            if (std::regex_match(name, dmaVideoPattern))
            {
                resource.path = "/tmp/" + name;
                resource.mode = "dma";
                resource.type = "video";

                // Extract dimensions from filename
                try
                {
                    resource.dimensions = parseImageDimensions(name);
                    std::cout << "Found shared video via DMA-BUF: " + name +
                             " (" + std::to_string(resource.dimensions.width) + "x" +
                             std::to_string(resource.dimensions.height) + ")" << "\n";
                    return resource;
                }
                catch (const std::exception &e)
                {
                    std::cout << "Error parsing dimensions: " + std::string(e.what()) << "\n";
                }
            }
        }

        // Nothing found
        return std::nullopt;
    }

} // namespace vst::utils