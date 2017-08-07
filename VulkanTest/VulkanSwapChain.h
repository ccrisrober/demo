#pragma once

#include "VulkanDevice.h"
#include <assert.h>

#include <algorithm> 
using namespace std;

class VulkanSwapChain
{
public:
  VulkanSwapChain( );
  ~VulkanSwapChain( );
  void rebuild( VulkanDevicePtr device, VkSurfaceKHR& surface, uint32_t w, uint32_t h, bool vsync, 
    VkFormat colorFormat, VkColorSpaceKHR colorSpace )
  {
    _device = device;

    VkResult result;
    VkPhysicalDevice physicalDevice = _device->getPhysical( );


    // Determine swap chain dimensions
    VkSurfaceCapabilitiesKHR surfaceCaps;
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR( physicalDevice, surface,
      &surfaceCaps );
    assert( result == VK_SUCCESS );

    VkExtent2D swapchainExtent;
    if ( surfaceCaps.currentExtent.width != std::numeric_limits<uint32_t>::max( ) )
    {
      swapchainExtent = surfaceCaps.currentExtent;
    }
    else
    {
      VkExtent2D actualExtent = { w, h };

      actualExtent.width = std::max( surfaceCaps.minImageExtent.width, 
        std::min( surfaceCaps.maxImageExtent.width, actualExtent.width ) );
      actualExtent.height = std::max( surfaceCaps.minImageExtent.height, 
        std::min( surfaceCaps.maxImageExtent.height, actualExtent.height ) );

      swapchainExtent = actualExtent;
    }

    _width = swapchainExtent.width;
    _height = swapchainExtent.height;


    // Find present mode
    uint32_t numPresentModes;
    result = vkGetPhysicalDeviceSurfacePresentModesKHR( physicalDevice, 
      surface, &numPresentModes, nullptr );
    assert( result == VK_SUCCESS );
    assert( numPresentModes > 0 );

    std::vector<VkPresentModeKHR> presentModes;
    presentModes.resize( numPresentModes );
    result = vkGetPhysicalDeviceSurfacePresentModesKHR( physicalDevice, surface,
      &numPresentModes, presentModes.data( ) );
    assert( result == VK_SUCCESS );

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    if ( !vsync )
    {
      for ( const auto& pr : presentModes )
      {
        if ( pr == VK_PRESENT_MODE_IMMEDIATE_KHR )
        {
          presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
          break;
        }

        if ( pr == VK_PRESENT_MODE_FIFO_RELAXED_KHR )
        {
          presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
        }
      }
    }
    else
    {
      // Mailbox comes with lower input latency than FIFO, but can waste GPU 
      //    power by rendering frames that are never
      // displayed, especially if the app runs much faster than the refresh 
      //    rate. This is a concern for mobiles.
      for ( const auto& pr : presentModes )
      {
        if ( pr == VK_PRESENT_MODE_MAILBOX_KHR )
        {
          presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
          break;
        }
      }
    }
    presentModes.clear( );


    uint32_t numImages = surfaceCaps.minImageCount;

    VkSurfaceTransformFlagsKHR transform;
    if ( surfaceCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR )
    {
      transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else
    {
      transform = surfaceCaps.currentTransform;
    }

    VkSwapchainCreateInfoKHR swapChainCI;// = { };
    swapChainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCI.pNext = nullptr;
    swapChainCI.flags = 0;
    swapChainCI.surface = surface;

    swapChainCI.minImageCount = numImages;
    swapChainCI.imageFormat = colorFormat;
    swapChainCI.imageColorSpace = colorSpace;
    swapChainCI.imageExtent = { swapchainExtent.width, swapchainExtent.height };
    swapChainCI.imageArrayLayers = 1;
    swapChainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;


    swapChainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapChainCI.queueFamilyIndexCount = 0;
    swapChainCI.pQueueFamilyIndices = nullptr;

    swapChainCI.preTransform = ( VkSurfaceTransformFlagBitsKHR ) transform;
    swapChainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainCI.presentMode = presentMode;
    swapChainCI.clipped = VK_TRUE;

    swapChainCI.oldSwapchain = _swapChain; // At this time, _swapChain as VK_NULL_HANDLE;

    VkDevice logicalDevice = _device->getLogical( );
    result = vkCreateSwapchainKHR( logicalDevice, &swapChainCI, nullptr, &_swapChain );
    assert( result == VK_SUCCESS );


    result = vkGetSwapchainImagesKHR( logicalDevice, _swapChain, &numImages, nullptr );
    assert( result == VK_SUCCESS );

    std::vector<VkImage> images( numImages );
    result = vkGetSwapchainImagesKHR( logicalDevice, _swapChain, &numImages, images.data( ) );
    assert( result == VK_SUCCESS );
  }
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
};

