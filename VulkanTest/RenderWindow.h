#pragma once

#include <vulkan/vulkan.h>

class VulkanRenderAPI;

class RenderWindow
{
public:
  RenderWindow( VulkanRenderAPI& renderAPI );
  ~RenderWindow( void );

protected:
  VulkanRenderAPI& _renderAPI;
  VkSurfaceKHR _surface;
};

