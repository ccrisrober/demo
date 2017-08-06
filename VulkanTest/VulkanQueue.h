#pragma once

#include <vulkan/vulkan.h>

class VulkanDevice;

// Types of GPU queues.
enum GpuQueueType
{
  /**
  * Queue used for rendering. Allows the use of draw commands, but also all commands supported by compute
  * or upload buffers.
  */
  GPUT_GRAPHICS,
  /** Discrete queue used for compute operations. Allows the use of dispatch and upload commands. */
  GPUT_COMPUTE,
  /** Queue used for memory transfer operations only. No rendering or compute dispatch allowed. */
  GPUT_TRANSFER,
  GPUT_COUNT // Keep at end
};

class VulkanQueue
{
public:
  VulkanQueue( VulkanDevice& device, VkQueue queue, GpuQueueType type, 
    uint32_t index );
  ~VulkanQueue( void );

protected:
  VulkanDevice& _device;
  VkQueue _queue;
  GpuQueueType _type;
  uint32_t _index;
};

