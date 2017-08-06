#include "RenderWindow.h"

#include "VulkanRenderAPI.h"

RenderWindow::RenderWindow( VulkanRenderAPI& renderAPI  )
  : _renderAPI( renderAPI )
{
  if ( glfwCreateWindowSurface( _renderAPI.getInstance( ), _renderAPI.getWindow( ), 
    nullptr, &_surface ) != VK_SUCCESS )
  {
    throw std::runtime_error( "failed to create window surface!" );
  }
}


RenderWindow::~RenderWindow( void )
{
  
  vkDestroySurfaceKHR( _renderAPI.getInstance( ), _surface, nullptr );
}
