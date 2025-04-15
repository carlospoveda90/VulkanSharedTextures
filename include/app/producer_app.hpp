#pragma once

#include <string>
#include "core/vulkan_context.hpp"
#include "media/image_loader.hpp"
#include "core/descriptor_manager.hpp"
#include "core/pipeline.hpp"

namespace vst
{

    class ProducerApp
    {
    public:
        ProducerApp(GLFWwindow *window, const std::string &imagePath, const std::string &mode);
        ~ProducerApp();

        void runFrame(const std::string &mode);
        void cleanup();

    private:
        VulkanContext context;
        Texture texture;
        Pipeline pipeline;

        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        DescriptorManager descriptorManager;
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    };

}
