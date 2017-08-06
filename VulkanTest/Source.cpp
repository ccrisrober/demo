#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>

#include <vector>
#include <assert.h>

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
  void cleanup( void );
protected:
  VkInstance _instance;
  GLFWwindow* _window;
#ifndef NDEBUG
  VkDebugReportCallbackEXT _debugCallback;
  PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = nullptr;
  PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT = nullptr;
#endif
};


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
, _debugCallback( 0 )
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
  const char* layers[ ] =
  {
    "VK_LAYER_LUNARG_standard_validation"
  };
  const char* extensions[ ] =
  {
    nullptr,	// Surface extension
    nullptr,	// OS specific surface extension
    VK_EXT_DEBUG_REPORT_EXTENSION_NAME
  };

  uint32_t numLayers = sizeof( layers ) / sizeof( layers[ 0 ] );
#else
  const char** layers = nullptr;
  const char* extensions[ ] =
  {
    nullptr,	// Surface extension
    nullptr,	// OS specific surface extension
  };
  uint32_t numLayers = 0;
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

  uint32_t numExtensions = sizeof( extensions ) / sizeof( extensions[ 0 ] );

  VkInstanceCreateInfo instanceInfo;
  instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instanceInfo.pNext = nullptr;
  instanceInfo.flags = 0;
  instanceInfo.pApplicationInfo = &appInfo;
  instanceInfo.enabledLayerCount = numLayers;
  instanceInfo.ppEnabledLayerNames = layers;
  instanceInfo.enabledExtensionCount = numExtensions;
  instanceInfo.ppEnabledExtensionNames = extensions;

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
  std::vector<VkPhysicalDevice> physicalDevices( _numDevices );
  result = vkEnumeratePhysicalDevices( _instance, &_numDevices, physicalDevices.data( ) );
}
int main( )
{
  VulkanRenderAPI vr;
  vr.initialize( );
  vr.run( );
  vr.cleanup( );
  //system( "PAUSE" );
  return 0;
}