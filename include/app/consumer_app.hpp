#pragma once

#include "core/vulkan_context.hpp"
#include "core/pipeline.hpp"
#include "core/descriptor_manager.hpp"
#include "core/vertex_definitions.hpp"

namespace vst
{

    class ConsumerApp
    {
    public:
        ConsumerApp(GLFWwindow *window, const std::string &mode);
        ~ConsumerApp();

        void runFrame();
        void cleanup();

    private:
        VulkanContext context;

        int dmaBufFd;
        uint32_t imageWidth;
        uint32_t imageHeight;

        VkImage importedImage = VK_NULL_HANDLE;
        VkDeviceMemory importedMemory = VK_NULL_HANDLE;
        VkImageView importedImageView = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        DescriptorManager descriptorManager;
        Pipeline pipeline;

        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    };

}
