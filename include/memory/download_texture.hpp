#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace vst
{

    struct CpuTextureData
    {
        void *mappedData = nullptr;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceSize size = 0;
    };

    class TextureDownloader
    {
    public:
        static CpuTextureData downloadImageToCpu(
            VkDevice device,
            VkPhysicalDevice physicalDevice,
            VkCommandPool commandPool,
            VkQueue graphicsQueue,
            VkImage srcImage,
            uint32_t width,
            uint32_t height);
    };

}
