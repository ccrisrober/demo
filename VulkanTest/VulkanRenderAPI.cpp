#include "VulkanRenderAPI.h"

#include <assert.h>

#include <sstream>
VkResult CreateDebugReportCallbackEXT( VkInstance instance,
  const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
  const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback )
{
  auto func = ( PFN_vkCreateDebugReportCallbackEXT ) vkGetInstanceProcAddr(
    instance, "vkCreateDebugReportCallbackEXT" );
  if ( func != nullptr )
  {
    return func( instance, pCreateInfo, pAllocator, pCallback );
  }
  else
  {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyDebugReportCallbackEXT( VkInstance instance,
  VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator )
{
  auto func = ( PFN_vkDestroyDebugReportCallbackEXT ) vkGetInstanceProcAddr(
    instance, "vkDestroyDebugReportCallbackEXT" );
  if ( func != nullptr )
  {
    func( instance, callback, pAllocator );
  }
}
VkBool32 debugMsgCallback( VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject,
  size_t location, int32_t msgCode, const char* pLayerPrefix, const char* pMsg, void* pUserData )
{
  std::stringstream message;

  // Determine prefix
  if ( flags & VK_DEBUG_REPORT_ERROR_BIT_EXT )
    message << "ERROR";

  if ( flags & VK_DEBUG_REPORT_WARNING_BIT_EXT )
    message << "WARNING";

  if ( flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT )
    message << "PERFORMANCE";

  if ( flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT )
    message << "INFO";

  if ( flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT )
    message << "DEBUG";

  message << ": [" << pLayerPrefix << "] Code " << msgCode << ": " << pMsg << std::endl;

  if ( flags & VK_DEBUG_REPORT_ERROR_BIT_EXT )
    std::cerr << message.str( ) << std::endl;
  else if ( flags & VK_DEBUG_REPORT_WARNING_BIT_EXT || flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT )
    std::cerr << message.str( ) << std::endl;
  else
    std::cerr << message.str( ) << std::endl;

  // Don't abort calls that caused a validation message
  return VK_FALSE;
}

VulkanRenderAPI::VulkanRenderAPI( )
: _instance( nullptr )
#ifndef NDEBUG
, _debugCallback( VK_NULL_HANDLE )
#endif
{
  glfwInit( );

  glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
  glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

  _window = glfwCreateWindow( WIDTH, HEIGHT, "Vulkan", nullptr, nullptr );
}

VulkanRenderAPI::~VulkanRenderAPI( )
{
}

void VulkanRenderAPI::cleanup( void )
{
  _swapChain.reset( );
  vkDestroySurfaceKHR( _instance, _surface, nullptr );
  _primaryDevices.clear( );
  _devices.clear( );
#ifndef NDEBUG
  if ( _debugCallback != 0 )
  {
    DestroyDebugReportCallbackEXT( _instance, _debugCallback, nullptr );
  }
#endif
  vkDestroyInstance( _instance, nullptr );

  glfwDestroyWindow( _window );

  glfwTerminate( );
}

void VulkanRenderAPI::initialize( void )
{
  // Create instance
  VkApplicationInfo appInfo;
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pNext = nullptr;
  appInfo.pApplicationName = "App Name";
  appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
  appInfo.pEngineName = "FooEngine";
  appInfo.engineVersion = VK_MAKE_VERSION( 1, 0, 0 );
  appInfo.apiVersion = VK_API_VERSION_1_0;

#ifndef NDEBUG
  std::vector<const char*> layers =
  {
    "VK_LAYER_LUNARG_standard_validation"
  };
  checkValidationLayerSupport( layers );
  std::vector<const char*> extensions =
  {
    nullptr,	// Surface extension
    nullptr,	// OS specific surface extension
    VK_EXT_DEBUG_REPORT_EXTENSION_NAME
  };
#else
  std::vector<const char*> layers;
  std::vector<const char*> extensions =
  {
    nullptr,	// Surface extension
    nullptr,	// OS specific surface extension
  };
#endif
  std::vector<const char*> exts;

  uint32_t extensionCount = 0;
  const char** glfwExtensions;

  glfwExtensions = glfwGetRequiredInstanceExtensions( &extensionCount );

  for ( uint32_t i = 0; i < extensionCount; ++i )
  {
    exts.push_back( glfwExtensions[ i ] );
  }

  extensions[ 0 ] = exts[ 0 ];
  extensions[ 1 ] = exts[ 1 ];

  VkInstanceCreateInfo instanceInfo;
  instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instanceInfo.pNext = nullptr;
  instanceInfo.flags = 0;
  instanceInfo.pApplicationInfo = &appInfo;
  instanceInfo.enabledLayerCount = static_cast<uint32_t>( layers.size( ) );
  instanceInfo.ppEnabledLayerNames = layers.data( );
  instanceInfo.enabledExtensionCount = static_cast<uint32_t>( extensions.size( ) );
  instanceInfo.ppEnabledExtensionNames = extensions.data( );

  VkResult result = vkCreateInstance( &instanceInfo, nullptr, &_instance );
  assert( result == VK_SUCCESS );

#ifndef NDEBUG
  // Set debug callback
  VkDebugReportFlagsEXT debugFlags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT |
    VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;

  VkDebugReportCallbackCreateInfoEXT debugInfo;
  debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
  debugInfo.pNext = nullptr;
  debugInfo.pfnCallback = ( PFN_vkDebugReportCallbackEXT ) debugMsgCallback;
  debugInfo.flags = debugFlags;

  if ( CreateDebugReportCallbackEXT( _instance, &debugInfo, nullptr, &_debugCallback ) != VK_SUCCESS )
  {
    throw std::runtime_error( "failed to set up debug callback!" );
  }
#endif


  uint32_t _numDevices = 0;

  // Enumerate all devices
  result = vkEnumeratePhysicalDevices( _instance, &_numDevices, nullptr );
  assert( result == VK_SUCCESS );

  if ( _numDevices == 0 )
  {
    throw std::runtime_error( "failed to find GPUs with Vulkan support!" );
  }

  std::vector<VkPhysicalDevice> physicalDevices( _numDevices );
  result = vkEnumeratePhysicalDevices( _instance, &_numDevices, physicalDevices.data( ) );

  _devices.resize( _numDevices );
  for ( uint32_t i = 0; i < _numDevices; ++i )
  {
    _devices[ i ] = std::make_shared<VulkanDevice>( physicalDevices[ i ], i );
  }


  // Find primary device
  // Note: MULTIGPU - Detect multiple similar devices here if supporting multi-GPU
  for ( uint32_t i = 0; i < _numDevices; ++i )
  {
    bool isPrimary = _devices[ i ]->getDeviceProperties( ).deviceType 
      == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

    if ( isPrimary )
    {
      _devices[ i ]->setIsPrimary( );
      _primaryDevices.push_back( _devices[ i ] );
      break;
    }
  }

  if ( _primaryDevices.empty( ) )
  {
    _primaryDevices.push_back( _devices.front( ) );
  }

  // Surface KHR
  if ( glfwCreateWindowSurface( this->getInstance( ), this->getWindow( ),
    nullptr, &_surface ) != VK_SUCCESS )
  {
    throw std::runtime_error( "failed to create window surface!" );
  }

  std::shared_ptr<VulkanDevice> presentDevice = this->getPresentDevice( );
  VkPhysicalDevice physicalDevice = presentDevice->getPhysical( );

  uint32_t presentQueueFamily = presentDevice->getQueueFamily( GPUT_GRAPHICS );

  VkBool32 supportsPresent = false;
  vkGetPhysicalDeviceSurfaceSupportKHR( physicalDevice, presentQueueFamily, 
    _surface, &supportsPresent );

  // Create swap chain
  /*mSwapChain = bs_shared_ptr_new<VulkanSwapChain>( );
  mSwapChain->rebuild( presentDevice, mSurface, props.mWidth, props.mHeight, props.mVSync, mColorFormat, mColorSpace,
    mDesc.depthBuffer, mDepthFormat );*/



  uint32_t numFormats;
  result = vkGetPhysicalDeviceSurfaceFormatsKHR( physicalDevice, _surface, &numFormats, nullptr );
  assert( result == VK_SUCCESS );
  assert( numFormats > 0 );

  std::vector<VkSurfaceFormatKHR> surfaceFormats( numFormats );
  result = vkGetPhysicalDeviceSurfaceFormatsKHR( physicalDevice, _surface, &numFormats, surfaceFormats.data( ) );
  assert( result == VK_SUCCESS );

  bool gamma = false;
  // If there is no preferred format, use standard RGBA
  if ( ( numFormats == 1 ) && ( surfaceFormats[ 0 ].format == VK_FORMAT_UNDEFINED ) )
  {
    if ( gamma )
    {
      _colorFormat = VK_FORMAT_R8G8B8A8_SRGB;
    }
    else
    {
      _colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
    }

    _colorSpace = surfaceFormats[ 0 ].colorSpace;
  }
  else
  {
    bool foundFormat = false;

    std::vector<VkFormat> wantedFormatsUNORM =
    {
      VK_FORMAT_R8G8B8A8_UNORM,
      VK_FORMAT_B8G8R8A8_UNORM,
      VK_FORMAT_A8B8G8R8_UNORM_PACK32,
      VK_FORMAT_A8B8G8R8_UNORM_PACK32,
      VK_FORMAT_R8G8B8_UNORM,
      VK_FORMAT_B8G8R8_UNORM
    };

    std::vector<VkFormat> wantedFormatsSRGB =
    {
      VK_FORMAT_R8G8B8A8_SRGB,
      VK_FORMAT_B8G8R8A8_SRGB,
      VK_FORMAT_A8B8G8R8_SRGB_PACK32,
      VK_FORMAT_A8B8G8R8_SRGB_PACK32,
      VK_FORMAT_R8G8B8_SRGB,
      VK_FORMAT_B8G8R8_SRGB
    };

    uint32_t numWantedFormats;
    std::vector<VkFormat> wantedFormats;
    if ( gamma )
    {
      wantedFormats = wantedFormatsSRGB;
    }
    else
    {
      wantedFormats = wantedFormatsUNORM;
    }

    for ( const auto& wantedFormat : wantedFormats )
    {
      for ( const auto& surfFormat : surfaceFormats )
      {
        if ( surfFormat.format == wantedFormat )
        {
          _colorFormat = surfFormat.format;
          _colorSpace = surfFormat.colorSpace;

          foundFormat = true;
          break;
        }
      }
      if ( foundFormat )
        break;
    }

    wantedFormatsSRGB.clear( );
    wantedFormatsUNORM.clear( );
    wantedFormats.clear( );

    // If we haven't found anything, fall back to first available
    if ( !foundFormat )
    {
      _colorFormat = surfaceFormats[ 0 ].format;
      _colorSpace = surfaceFormats[ 0 ].colorSpace;

      if ( gamma )
        throw new std::exception( "Cannot find a valid sRGB format for a render window surface, falling back to a default format." );
    }
  }

  _swapChain = std::make_shared<VulkanSwapChain>( );
  _swapChain->rebuild( getPresentDevice( ), _surface, WIDTH, HEIGHT, true, _colorFormat, _colorSpace );
}