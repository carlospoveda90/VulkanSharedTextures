#pragma once

#include <vulkan/vulkan.h>
#include "media/image_loader.hpp"

namespace vst
{

    class DescriptorManager
    {
    public:
        void init(VkDevice device, VkDescriptorPool descriptorPool, Texture &texture);
        void cleanup(VkDevice device);

        VkDescriptorSetLayout getLayout() const { return descriptorSetLayout; }
        VkDescriptorSet getDescriptorSet() const { return descriptorSet; }
        VkSampler getSampler() const { return sampler; }

    private:
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;
    };

}
