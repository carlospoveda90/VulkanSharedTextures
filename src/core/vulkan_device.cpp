#include "core/vulkan_device.hpp"
#include "utils/logger.hpp"
#include <stdexcept>
#include <vector>

// PFN_vkGetMemoryFdKHR vkGetMemoryFdKHR = nullptr;

namespace vst
{
    const std::vector<const char *> requiredDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
        VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME};

    VulkanDevice::VulkanDevice() {}

    VulkanDevice::~VulkanDevice()
    {
        if (device != VK_NULL_HANDLE)
        {
            vkDestroyDevice(device, nullptr);
        }
    }

    void VulkanDevice::pickPhysicalDevice(VkInstance inst)
    {
        instance = inst;
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0)
            throw std::runtime_error("Failed to find GPUs with Vulkan support.");

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        for (auto &dev : devices)
        {
            if (isDeviceSuitable(dev))
            {
                physicalDevice = dev;
                LOG_INFO("Selected suitable physical device.");
                return;
            }
        }

        throw std::runtime_error("No suitable Vulkan GPU found.");
    }

    bool VulkanDevice::isDeviceSuitable(VkPhysicalDevice device)
    {
        // Just pick first queue with graphics support
        uint32_t queueCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, nullptr);

        std::vector<VkQueueFamilyProperties> props(queueCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, props.data());

        for (uint32_t i = 0; i < props.size(); i++)
        {
            if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                queueFamilies.graphicsFamily = i;
                return true;
            }
        }

        return false;
    }

    void VulkanDevice::createLogicalDevice()
    {
        if (!queueFamilies.isComplete())
            throw std::runtime_error("Queue families not selected.");

        float priority = 1.0f;
        VkDeviceQueueCreateInfo queueCreate{};
        queueCreate.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreate.queueFamilyIndex = queueFamilies.graphicsFamily;
        queueCreate.queueCount = 1;
        queueCreate.pQueuePriorities = &priority;

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = 1;
        createInfo.pQueueCreateInfos = &queueCreate;

        createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size());
        createInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();

        // vkGetMemoryFdKHR = reinterpret_cast<PFN_vkGetMemoryFdKHR>(vkGetDeviceProcAddr(device, "vkGetMemoryFdKHR"));

        // if (!vkGetMemoryFdKHR)
        // {
        //     throw std::runtime_error("vkGetMemoryFdKHR not available. Extension VK_KHR_external_memory_fd may not be supported.");
        // }

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create logical device.");
        }

        vkGetDeviceQueue(device, queueFamilies.graphicsFamily, 0, &graphicsQueue);
        LOG_INFO("Logical device successfully created.");
    }

} // namespace vst
