message ("-- Enabling Vulkan backend support")

find_package(Vulkan REQUIRED)

add_compile_definitions(CPD_VULKAN_ENABLED VK_NO_PROTOTYPES)
target_compile_definitions(Cpeed PRIVATE CPD_VULKAN_ENABLED VK_NO_PROTOTYPES)

include_directories(${Vulkan_INCLUDE_DIRS})
target_include_directories(Cpeed PRIVATE ${Vulkan_INCLUDE_DIRS})

target_sources(Cpeed PRIVATE
  "../common/backend/vulkan/rendering/rendering.c" "../common/backend/vulkan/rendering/rendering.h"
  
  "../common/backend/vulkan/backend.c"   "../common/backend/vulkan/backend.h"
  "../common/backend/vulkan/device.c"    "../common/backend/vulkan/device.h"
  "../common/backend/vulkan/device.queue_init.c" "../common/backend/vulkan/device.queue_init.h"
  "../common/backend/vulkan/image.c"     "../common/backend/vulkan/image.h"
  "../common/backend/vulkan/renderer.c"  "../common/backend/vulkan/renderer.h"
  "../common/backend/vulkan/shortcuts.c" "../common/backend/vulkan/shortcuts.h"
  "../common/backend/vulkan/surface.c"   "../common/backend/vulkan/surface.h"
  "../common/backend/vulkan/swapchain.c" "../common/backend/vulkan/swapchain.h"
  "../common/backend/vulkan/vulkan.c"    "../common/backend/vulkan/vulkan.h"

  "../platform/backend/vulkan.h")
