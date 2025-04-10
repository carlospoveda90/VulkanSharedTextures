#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <GLFW/glfw3.h>

namespace vst
{

    class Swapchain
    {
    public:
        Swapchain();
        ~Swapchain();

        void init(VkPhysicalDevice physicalDevice, VkDevice device, GLFWwindow *window, VkSurfaceKHR surface);
        void cleanup();

        VkSwapchainKHR getSwapchain() const { return swapchain; }
        VkFormat getImageFormat() const { return imageFormat; }
        VkExtent2D getExtent() const { return extent; }
        const std::vector<VkImageView> &getImageViews() const { return imageViews; }

    private:
        void createSwapchain();
        void createImageViews();

        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        GLFWwindow *window = nullptr;

        VkSwapchainKHR swapchain = VK_NULL_HANDLE;
        VkFormat imageFormat;
        VkExtent2D extent;
        std::vector<VkImage> images;
        std::vector<VkImageView> imageViews;
    };

} // namespace vst
