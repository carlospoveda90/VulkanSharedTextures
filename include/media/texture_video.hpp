#pragma once

#include <vulkan/vulkan.h>
#include <opencv2/opencv.hpp>
#include <memory>

namespace vst
{
    class VulkanContext;

    class TextureVideo
    {
    public:
        TextureVideo(VulkanContext &ctx);
        ~TextureVideo();

        bool createFromSize(uint32_t width, uint32_t height);
        void updateFromFrame(const cv::Mat &frame);
        void destroy();

        // Accessor methods for internal members
        VkImage getImage() const { return image; }
        VkDeviceMemory getMemory() const { return memory; }
        VkImageView getImageView() const { return imageView; }
        VkSampler getSampler() const { return sampler; }
        uint32_t getWidth() const { return texWidth; }
        uint32_t getHeight() const { return texHeight; }
        void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
        VkCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(VkCommandBuffer commandBuffer);

    private:
        void createSampler();

        VulkanContext &context;
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;
        uint32_t texWidth = 0;
        uint32_t texHeight = 0;
    };
}