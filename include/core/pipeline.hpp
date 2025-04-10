#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace vst
{

    class Pipeline
    {
    public:
        Pipeline();
        ~Pipeline();

        void create(VkDevice device, VkExtent2D extent, VkRenderPass renderPass, VkDescriptorSetLayout descriptorSetLayout);
        void cleanup(VkDevice device);

        VkPipeline get() const { return graphicsPipeline; }
        VkPipelineLayout getLayout() const { return pipelineLayout; }

    private:
        VkPipeline graphicsPipeline = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    };

} // namespace vst
