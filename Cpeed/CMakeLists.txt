# CMakeList.txt : CMake project for Cpeed, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (Cpeed
  "main.c" "main.h"
  "platform.h"
  "vulkan.c" "vulkan.h"
  
  "common/device.c" "common/device.h"
  "common/device.queue_init.c" "common/device.queue_init.h"
  "common/general.c" "common/general.h"
  "common/image.c" "common/image.h"
  "common/renderer.c" "common/renderer.h"
  "common/shortcuts.c" "common/shortcuts.h"
  "common/surface.c" "common/surface.h"
  "common/swapchain.c" "common/swapchain.h"
  "common/window.h")

if (WIN32)
  target_include_directories(Cpeed PUBLIC "$ENV{VULKAN_SDK}/include")
  target_link_libraries(Cpeed PUBLIC "$ENV{VULKAN_SDK}/lib/vulkan-1.lib")
endif()

set_property(TARGET Cpeed PROPERTY C_STANDARD 99)
