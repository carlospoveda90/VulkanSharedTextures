#pragma once

#include <vulkan/vulkan.h>
#include <opencv2/opencv.hpp>
#include <memory>

namespace vst
{
    class VulkanContext; // forward declare

    class TextureVideo
    {
    public:
        // Texture();
        TextureVideo(VulkanContext &context);
        ~TextureVideo();

        bool createFromSize(uint32_t width, uint32_t height);
        void destroy();

        void updateFromFrame(const cv::Mat &frame);

        VkImage getImage() const { return image; }
        VkImageView getImageView() const { return imageView; }
        VkSampler getSampler() const { return sampler; }
        VkDeviceMemory getMemory() const { return memory; }
        uint32_t getWidth() const { return texWidth; }
        uint32_t getHeight() const { return texHeight; }

        // bool isInitialized() const { return image != VK_NULL_HANDLE; }

    private:
        VulkanContext &context;

        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;

        uint32_t texWidth = 0;
        uint32_t texHeight = 0;

        void createSampler();
    };

} // namespace vst
