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

  operator VkSwapchainKHR( )
  {
    return _swapChain;
  }

  VkSwapchainKHR getSwapChain( void ) const
  {
    return _swapChain;
  }

  /*void acquireBackBuffer( )
  {
    uint32_t imageIndex;

    VkResult result = vkAcquireNextImageKHR( _device->getLogical( ), _swapChain, UINT64_MAX,
      _surfaces[ mCurrentSemaphoreIdx ].sync->getHandle( ), VK_NULL_HANDLE, &imageIndex );
    assert( result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR );

    // In case surfaces aren't being distributed in round-robin fashion the image and semaphore indices might not match,
    // in which case just move the semaphores
    if ( imageIndex != mCurrentSemaphoreIdx )
      std::swap( mSurfaces[ mCurrentSemaphoreIdx ].sync, mSurfaces[ imageIndex ].sync );

    mCurrentSemaphoreIdx = ( mCurrentSemaphoreIdx + 1 ) % mSurfaces.size( );

    assert( !mSurfaces[ imageIndex ].acquired && "Same swap chain surface being acquired twice in a row without present()." );
    mSurfaces[ imageIndex ].acquired = true;
    mSurfaces[ imageIndex ].needsWait = true;

    mCurrentBackBufferIdx = imageIndex;
  }*/


protected:
  VulkanDevicePtr _device;
  uint32_t _width = 0;
  uint32_t _height = 0;
  VkSwapchainKHR _swapChain = VK_NULL_HANDLE;
public:
  VkExtent2D swapchainExtent;
  std::vector<VkImageView> swapChainImageViews;
};

