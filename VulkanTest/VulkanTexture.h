#pragma once

#include <vulkan/vulkan.h>

#include "VulkanResource.h"

class VulkanImage : public VulkanResource
{
public:
  VulkanImage( VulkanDevicePtr device, VkImage image, VkDeviceMemory mem,
    VkImageLayout layout );
};

class VulkanTexture : public VulkanResource
{
public:
  VulkanTexture( VulkanDevicePtr device );
};
