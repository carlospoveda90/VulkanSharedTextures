#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include "core/vulkan_device.hpp"
#include "core/swapchain.hpp"

namespace vst
{
    class VulkanContext
    {
    public:
        VulkanContext();
        ~VulkanContext();

        void init(GLFWwindow *window);
        void setupRenderPass();
        void createFramebuffers();
        void cleanup();

        void drawFrame(VkPipeline pipeline, VkPipelineLayout layout, VkDescriptorSet descriptorSet, VkBuffer vertexBuffer);
        VkInstance getInstance() const { return instance; }
        VkDevice getDevice() const { return device.getDevice(); }
        VkPhysicalDevice getPhysicalDevice() const { return device.getPhysicalDevice(); }
        VkCommandPool getCommandPool() const { return commandPool; }
        VkQueue getGraphicsQueue() const { return device.getGraphicsQueue(); }
        VkExtent2D getSwapchainExtent() const { return swapchain.getExtent(); }
        VkRenderPass getRenderPass() const { return renderPass; }

    private:
        void createInstance();
        void createSurface(GLFWwindow *window);
        void pickDeviceAndCreateLogical();
        void setupSwapchain(GLFWwindow *window);

        VkInstance instance = VK_NULL_HANDLE;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkRenderPass renderPass = VK_NULL_HANDLE;
        std::vector<VkFramebuffer> framebuffers;

        VkCommandPool commandPool = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> commandBuffers;

        VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
        VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
        VkFence inFlightFence = VK_NULL_HANDLE;

        VulkanDevice device;
        Swapchain swapchain;

        void createCommandPool();
        void createCommandBuffers();
        void createSyncObjects();
    };

} // namespace vst
