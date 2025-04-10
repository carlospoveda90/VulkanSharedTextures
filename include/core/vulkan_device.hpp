#pragma once

#include <vulkan/vulkan.h>
#include <vector>

// extern PFN_vkGetMemoryFdKHR vkGetMemoryFdKHR;

namespace vst
{

    struct QueueFamilyIndices
    {
        uint32_t graphicsFamily = -1;
        bool isComplete() const { return graphicsFamily != -1; }
    };

    class VulkanDevice
    {
    public:
        VulkanDevice();
        ~VulkanDevice();

        void pickPhysicalDevice(VkInstance instance);
        void createLogicalDevice();

        VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }
        VkDevice getDevice() const { return device; }
        QueueFamilyIndices getQueueFamilies() const { return queueFamilies; }
        VkQueue getGraphicsQueue() const { return graphicsQueue; }

    private:
        bool isDeviceSuitable(VkPhysicalDevice device);

        VkInstance instance = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;
        QueueFamilyIndices queueFamilies;
        VkQueue graphicsQueue = VK_NULL_HANDLE;
    };

} // namespace vst
