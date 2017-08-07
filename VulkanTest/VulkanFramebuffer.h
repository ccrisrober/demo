#pragma once
#include "VulkanResource.h"
class VulkanFramebuffer :
  public VulkanResource
{
public:
  VulkanFramebuffer( VulkanDevicePtr device );
  ~VulkanFramebuffer( void );
};

