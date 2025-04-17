#pragma once

#include <string>
#include <memory>
#include "core/pipeline.hpp"
#include "core/descriptor_manager.hpp"
#include "core/vulkan_context.hpp"
#include "media/image_loader.hpp"
#include "window/iapp_window.hpp"

namespace vst
{
    class ProducerApp
    {
    public:
        ProducerApp();
        ~ProducerApp();
        ProducerApp(GLFWwindow *window, const std::string &imagePath, const std::string &mode);
        ProducerApp(const std::string &imagePath, const std::string &mode);

        void runFrame();
        void runFrame(const std::string &imagePath);
        void cleanup();

    private:
        VulkanContext context;
        Texture texture;
        Pipeline pipeline;
        std::string mode;

        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        DescriptorManager descriptorManager;
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    };

}
