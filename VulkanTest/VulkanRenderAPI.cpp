#include "VulkanRenderAPI.h"

#include <assert.h>

#include <functional>
#include <fstream>

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
  VkDevice logicalDevice = getPresentDevice( )->getLogical( );

  vkDestroySemaphore( logicalDevice, renderFinishedSemaphore, nullptr );
  vkDestroySemaphore( logicalDevice, imageAvailableSemaphore, nullptr );

  vkDestroyCommandPool( logicalDevice, commandPool, nullptr );

  for ( size_t i = 0; i < swapChainFramebuffers.size( ); ++i )
  {
    vkDestroyFramebuffer( logicalDevice, swapChainFramebuffers[ i ], nullptr );
  }


  vkDestroyPipeline( logicalDevice, graphicsPipeline, nullptr );
  vkDestroyPipelineLayout( logicalDevice, pipelineLayout, nullptr );
  vkDestroyRenderPass( logicalDevice, renderPass, nullptr );

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

  // TODO: USED??
  VkBool32 supportsPresent = false;
  vkGetPhysicalDeviceSurfaceSupportKHR( physicalDevice, presentQueueFamily, 
    _surface, &supportsPresent );
  // TODO: USED?? \


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



  // TODO: MOVE TO ANOTHER ZONE
  std::function<std::vector<char>( const std::string& )> readFile = [ &] ( const std::string& filename )
  {
    std::ifstream file( filename, std::ios::ate | std::ios::binary );

    if ( !file.is_open( ) )
    {
      throw std::runtime_error( "failed to open file!" );
    }

    size_t fileSize = ( size_t ) file.tellg( );
    std::vector<char> buffer( fileSize );

    file.seekg( 0 );
    file.read( buffer.data( ), fileSize );

    file.close( );

    return buffer;
  };

  std::function<VkShaderModule( const std::vector<char>& code )>
    createShaderModule = [ &] ( const std::vector<char>& code )
  {
    VkShaderModuleCreateInfo createInfo = { };
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size( );
    createInfo.pCode = reinterpret_cast<const uint32_t*>( code.data( ) );

    VkShaderModule shaderModule;
    if ( vkCreateShaderModule( getPresentDevice( )->getLogical( ),
      &createInfo, nullptr, &shaderModule ) != VK_SUCCESS )
    {
      throw std::runtime_error( "failed to create shader module!" );
    }

    return shaderModule;
  };

  auto vertShaderCode = readFile( "C:/Users/maldicion069/Documents/Visual Studio 2013/Projects/VulkanTest/VulkanTest/vert.spv" );
  auto fragShaderCode = readFile( "C:/Users/maldicion069/Documents/Visual Studio 2013/Projects/VulkanTest/VulkanTest/frag.spv" );

  VkShaderModule vertShaderModule = createShaderModule( vertShaderCode );
  VkShaderModule fragShaderModule = createShaderModule( fragShaderCode );

  VkPipelineShaderStageCreateInfo vertShaderStageInfo = { };
  vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo = { };
  fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[ ] = { vertShaderStageInfo, fragShaderStageInfo };



  // RENDER PASSES
  VkDevice logicalDevice = getPresentDevice( )->getLogical( );


  VkAttachmentDescription colorAttachment = { };
  colorAttachment.format = _colorFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef = { };
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = { };
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  VkRenderPassCreateInfo renderPassInfo = { };
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;

  if ( vkCreateRenderPass( logicalDevice, &renderPassInfo,
    nullptr, &renderPass ) != VK_SUCCESS )
  {
    throw std::runtime_error( "failed to create render pass!" );
  }



  // FIXED FUNCTIONS
  VkPipelineVertexInputStateCreateInfo vertexInputInfo = { };
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 0;
  vertexInputInfo.vertexAttributeDescriptionCount = 0;

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = { };
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;


  VkViewport viewport = { };
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = ( float ) _swapChain->getWidth( );
  viewport.height = ( float ) _swapChain->getHeight( );
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = { };
  scissor.offset = { 0, 0 };
  scissor.extent = { _swapChain->getWidth( ), _swapChain->getHeight( ) };

  VkPipelineViewportStateCreateInfo viewportState = { };
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer = { };
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling = { };
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState colorBlendAttachment = { };
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
    | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo colorBlending = { };
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[ 0 ] = 0.0f;
  colorBlending.blendConstants[ 1 ] = 0.0f;
  colorBlending.blendConstants[ 2 ] = 0.0f;
  colorBlending.blendConstants[ 3 ] = 0.0f;

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = { };
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 0;
  pipelineLayoutInfo.pushConstantRangeCount = 0;

  if ( vkCreatePipelineLayout( getPresentDevice( )->getLogical( ),
    &pipelineLayoutInfo, nullptr, &pipelineLayout ) != VK_SUCCESS )
  {
    throw std::runtime_error( "failed to create pipeline layout!" );
  }

  // CONCLUSION (GRAPHICS PIPELINE BASICS)
  VkGraphicsPipelineCreateInfo pipelineInfo = { };
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

  if ( vkCreateGraphicsPipelines( logicalDevice,
    VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline ) != VK_SUCCESS )
  {
    throw std::runtime_error( "failed to create graphics pipeline!" );
  }


  vkDestroyShaderModule( logicalDevice, fragShaderModule, nullptr );
  vkDestroyShaderModule( logicalDevice, vertShaderModule, nullptr );






  // FRAMEBUFFERS
  swapChainFramebuffers.resize( _swapChain->swapChainImageViews.size( ) );

  for ( size_t i = 0; i < _swapChain->swapChainImageViews.size( ); i++ )
  {
    VkImageView attachments[ ] = {
      _swapChain->swapChainImageViews[ i ]
    };

    VkFramebufferCreateInfo framebufferInfo = { };
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = _swapChain->getWidth( );
    framebufferInfo.height = _swapChain->getHeight( );
    framebufferInfo.layers = 1;

    if ( vkCreateFramebuffer( logicalDevice,
      &framebufferInfo, nullptr, &swapChainFramebuffers[ i ] ) != VK_SUCCESS )
    {
      throw std::runtime_error( "failed to create framebuffer!" );
    }
  }

  // COMMAND POOL
  VkCommandPoolCreateInfo poolCI;
  poolCI.pNext = nullptr;
  poolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolCI.queueFamilyIndex = getPresentDevice( )->getQueueFamily( GpuQueueType::GPUT_GRAPHICS );
  if ( vkCreateCommandPool( logicalDevice, &poolCI, nullptr, &commandPool ) != VK_SUCCESS )
  {
    throw std::runtime_error( "failed to create command pool!" );
  }

  // COMMAND BUFFERS
  commandBuffers.resize( swapChainFramebuffers.size( ) );
  VkCommandBufferAllocateInfo allocInfo = { };
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = ( uint32_t ) commandBuffers.size( );

  if ( vkAllocateCommandBuffers( logicalDevice, &allocInfo, commandBuffers.data( ) ) != VK_SUCCESS )
  {
    throw std::runtime_error( "failed to allocate command buffers!" );
  }
  // Starting command buffer recording
  for ( size_t i = 0; i < commandBuffers.size( ); ++i )
  {
    VkCommandBufferBeginInfo beginInfo = { };
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    vkBeginCommandBuffer( commandBuffers[ i ], &beginInfo );

    // Starting a render pass
    VkRenderPassBeginInfo renderPassInfo = { };
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[ i ];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = _swapChain->swapchainExtent;

    VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass( commandBuffers[ i ], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );

    // Basic drawing commands
    vkCmdBindPipeline( commandBuffers[ i ], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline );

    vkCmdDraw( commandBuffers[ i ], 3, 1, 0, 0 );

    vkCmdEndRenderPass( commandBuffers[ i ] );

    if ( vkEndCommandBuffer( commandBuffers[ i ] ) != VK_SUCCESS )
    {
      throw std::runtime_error( "failed to record command buffer!" );
    }
  }

  // SEMAPHORES
  VkSemaphoreCreateInfo semaphoreCI;
  semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  semaphoreCI.pNext = nullptr;
  semaphoreCI.flags = 0;

  if ( vkCreateSemaphore( logicalDevice, &semaphoreCI, nullptr, &imageAvailableSemaphore ) != VK_SUCCESS ||
    vkCreateSemaphore( logicalDevice, &semaphoreCI, nullptr, &renderFinishedSemaphore ) != VK_SUCCESS )
  {

    throw std::runtime_error( "failed to create semaphores!" );
  }
}