#pragma once

#include "VulkanDevice.h"
#include "VulkanTexture.h"
#include <assert.h>

#include <algorithm> 
using namespace std;

class VulkanSwapChain
{
public:
  VulkanSwapChain( );
  ~VulkanSwapChain( );
  void rebuild( VulkanDevicePtr device, VkSurfaceKHR& surface, uint32_t w, 
    uint32_t h, bool vsync, VkFormat colorFormat, VkColorSpaceKHR colorSpace );
  /**
  * Returns the actual width of the swap chain, in pixels. This might differ from the requested size in case it
  * wasn't supported.
  */
  uint32_t getWidth( void ) const
  {
    return _width;
  }

  /**
  * Returns the actual height of the swap chain, in pixels. This might differ from the requested size in case it
  * wasn't supported.
  */
  uint32_t getHeight( void ) const
  {
    return _height;
  }
protected:
  VulkanDevicePtr _device;
  uint32_t _width = 0;
  uint32_t _height = 0;
  VkSwapchainKHR _swapChain = VK_NULL_HANDLE;
public:
  std::vector<VkImageView> swapChainImageViews;
};

