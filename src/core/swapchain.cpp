#include "core/swapchain.hpp"
#include "utils/logger.hpp"
#include <stdexcept>

namespace vst
{

    Swapchain::Swapchain() {}

    Swapchain::~Swapchain()
    {
        cleanup();
    }

    void Swapchain::init(VkPhysicalDevice phys, VkDevice dev, GLFWwindow *win, VkSurfaceKHR surf)
    {
        physicalDevice = phys;
        device = dev;
        window = win;
        surface = surf;

        createSwapchain();
        createImageViews();
    }

    void Swapchain::createSwapchain()
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = 2;
        createInfo.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
        createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        createInfo.imageExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create swapchain.");
        }

        uint32_t imageCount = 0;
        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
        images.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images.data());

        imageFormat = createInfo.imageFormat;
        extent = createInfo.imageExtent;

        LOG_INFO("Swapchain created.");
    }

    void Swapchain::createImageViews()
    {
        imageViews.resize(images.size());

        for (size_t i = 0; i < images.size(); i++)
        {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = images[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = imageFormat;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device, &createInfo, nullptr, &imageViews[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create image view.");
            }
        }

        LOG_INFO("Swapchain image views created.");
    }

    void Swapchain::cleanup()
    {
        for (auto view : imageViews)
        {
            vkDestroyImageView(device, view, nullptr);
        }

        if (swapchain)
        {
            vkDestroySwapchainKHR(device, swapchain, nullptr);
        }

        LOG_INFO("Swapchain cleaned up.");
    }

} // namespace vst
