#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include "media/texture_image.hpp"

namespace vst
{

    class ImageLoader
    {
    public:
        static TextureImage loadTexture(const std::string &path,
                                        VkDevice device,
                                        VkPhysicalDevice physicalDevice,
                                        VkCommandPool commandPool,
                                        VkQueue graphicsQueue,
                                        bool exportMemory = false);
    };

} // namespace vst
