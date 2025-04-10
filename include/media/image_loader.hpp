#pragma once

#include <vulkan/vulkan.h>
#include <string>

namespace vst
{

    struct Texture
    {
        VkImage image;
        VkDeviceMemory memory;
        VkImageView view;
        int width;
        int height;
    };

    class ImageLoader
    {
    public:
        static Texture loadTexture(const std::string &path, VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, bool exportMemory = false);
    };

}
