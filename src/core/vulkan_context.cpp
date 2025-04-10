#include "core/vulkan_context.hpp"
#include "utils/logger.hpp"
#include <stdexcept>
#include <set>

namespace vst
{

    VulkanContext::VulkanContext() {}

    VulkanContext::~VulkanContext()
    {
        cleanup();
    }

    void VulkanContext::init(GLFWwindow *window)
    {
        createInstance();
        createSurface(window);
        pickDeviceAndCreateLogical();
        setupSwapchain(window);
        setupRenderPass();
        createFramebuffers();
        createCommandPool();
        createCommandBuffers();
        createSyncObjects();
    }

    void VulkanContext::createInstance()
    {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "VulkanSharedTextures";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create Vulkan instance.");
        }

        LOG_INFO("Vulkan instance created.");
    }

    void VulkanContext::createSurface(GLFWwindow *window)
    {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create window surface.");
        }
        LOG_INFO("Window surface created.");
    }

    void VulkanContext::pickDeviceAndCreateLogical()
    {
        device.pickPhysicalDevice(instance);
        device.createLogicalDevice();
    }

    void VulkanContext::setupSwapchain(GLFWwindow *window)
    {
        swapchain.init(device.getPhysicalDevice(), device.getDevice(), window, surface);
    }

    void VulkanContext::setupRenderPass()
    {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapchain.getImageFormat();
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorRef{};
        colorRef.attachment = 0;
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorRef;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        if (vkCreateRenderPass(device.getDevice(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create render pass.");
        }

        LOG_INFO("Render pass created.");
    }

    void VulkanContext::createFramebuffers()
    {
        auto imageViews = swapchain.getImageViews();
        framebuffers.resize(imageViews.size());

        for (size_t i = 0; i < imageViews.size(); ++i)
        {
            VkImageView attachments[] = {imageViews[i]};

            VkFramebufferCreateInfo fbInfo{};
            fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fbInfo.renderPass = renderPass;
            fbInfo.attachmentCount = 1;
            fbInfo.pAttachments = attachments;
            fbInfo.width = swapchain.getExtent().width;
            fbInfo.height = swapchain.getExtent().height;
            fbInfo.layers = 1;

            if (vkCreateFramebuffer(device.getDevice(), &fbInfo, nullptr, &framebuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create framebuffer.");
            }
        }

        LOG_INFO("Framebuffers created.");
    }

    void VulkanContext::drawFrame(VkPipeline pipeline, VkPipelineLayout layout, VkDescriptorSet descriptorSet, VkBuffer vertexBuffer)
    {
        vkWaitForFences(device.getDevice(), 1, &inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(device.getDevice(), 1, &inFlightFence);

        uint32_t imageIndex;
        vkAcquireNextImageKHR(device.getDevice(), swapchain.getSwapchain(), UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

        VkCommandBuffer cmd = commandBuffers[imageIndex];

        VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        vkBeginCommandBuffer(cmd, &beginInfo);

        VkClearValue clearColor = {{0.1f, 0.1f, 0.1f, 1.0f}};

        VkRenderPassBeginInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = framebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapchain.getExtent();
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Bind and draw!
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSet, 0, nullptr);
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, offsets);
        vkCmdDraw(cmd, 6, 1, 0, 0); // fullscreen quad (to be added soon)

        vkCmdEndRenderPass(cmd);
        vkEndCommandBuffer(cmd);

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
        VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

        VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        auto queue = device.getGraphicsQueue();
        vkQueueSubmit(queue, 1, &submitInfo, inFlightFence);

        VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.swapchainCount = 1;
        VkSwapchainKHR swapchainHandle = swapchain.getSwapchain();
        presentInfo.pSwapchains = &swapchainHandle;
        presentInfo.pImageIndices = &imageIndex;

        vkQueuePresentKHR(queue, &presentInfo);
    }

    void VulkanContext::createCommandPool()
    {
        auto queueFamilyIndex = device.getQueueFamilies().graphicsFamily;

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndex;

        if (vkCreateCommandPool(device.getDevice(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create command pool.");
        }

        LOG_INFO("Command pool created.");
    }

    void VulkanContext::createCommandBuffers()
    {
        commandBuffers.resize(framebuffers.size());

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

        if (vkAllocateCommandBuffers(device.getDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate command buffers.");
        }

        LOG_INFO("Command buffers allocated.");
    }

    void VulkanContext::createSyncObjects()
    {
        VkSemaphoreCreateInfo semInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateSemaphore(device.getDevice(), &semInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(device.getDevice(), &semInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
            vkCreateFence(device.getDevice(), &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create sync objects.");
        }

        LOG_INFO("Sync objects created.");
    }

    void VulkanContext::cleanup()
    {
        swapchain.cleanup();

        if (surface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(instance, surface, nullptr);
        }

        device.~VulkanDevice(); // OR move this into the VulkanDevice destructor

        if (instance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(instance, nullptr);
        }

        LOG_INFO("Vulkan context cleaned up.");
    }

} // namespace vst
