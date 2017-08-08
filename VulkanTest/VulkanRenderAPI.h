#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>

#include <vector>
#include "VulkanDevice.h"
//#include "RenderWindow.h"
#include "VulkanSwapChain.h"

const int WIDTH = 800;
const int HEIGHT = 600;

class VulkanRenderAPI
{
public:
  VulkanRenderAPI( void );
  ~VulkanRenderAPI( void );
  void initialize( void );
  void run( void )
  {
    while ( !glfwWindowShouldClose( _window ) )
    {
      glfwPollEvents( );
    }
  }


  // Returns the internal Vulkan instance object.
  VkInstance getInstance( void ) const
  {
    return _instance;
  }

  GLFWwindow* getWindow( void ) const
  {
    return _window;
  }
  
  std::shared_ptr<VulkanDevice> getPresentDevice( void ) const
  {
    return _primaryDevices.front( );
  }

  void cleanup( void );
protected:
#ifndef NDEBUG
  VkDebugReportCallbackEXT _debugCallback;
  PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = nullptr;
  PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT = nullptr;
#endif
  std::vector<VulkanDevicePtr> _devices;
  std::vector<VulkanDevicePtr> _primaryDevices;


  bool checkValidationLayerSupport( const std::vector<const char*>& validationLayers )
  {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties( &layerCount, nullptr );

    std::vector<VkLayerProperties> availableLayers( layerCount );
    vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data( ) );

    for ( const char* layerName : validationLayers )
    {
      bool layerFound = false;

      for ( const auto& layerProperties : availableLayers )
      {
        if ( strcmp( layerName, layerProperties.layerName ) == 0 )
        {
          layerFound = true;
          //break;
        }
        std::cout << layerProperties.layerName << std::endl;
      }

      if ( !layerFound )
      {
        return false;
      }
    }

    return true;
  }


  // MOVE TO ANOTHER CLASS
  //std::shared_ptr<RenderWindow> _renderWindow;
  std::vector<VkFramebuffer> swapChainFramebuffers;

  VkRenderPass renderPass;
  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;
  VkSurfaceKHR _surface;
  VkColorSpaceKHR _colorSpace;
  VkFormat _colorFormat;
  //VkFormat _depthFormat;
  std::shared_ptr<VulkanSwapChain> _swapChain;

  VkCommandPool commandPool;
  std::vector<VkCommandBuffer> commandBuffers;
  // <MOVE TO ANOTHER CLASS \\



  VkInstance _instance;
  GLFWwindow* _window;
};
