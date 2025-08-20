# CMakeList.txt : CMake project for Cpeed, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (Cpeed
  "input.h"
  "main.c" "main.h"
  "platform.h"
  "vulkan.c" "vulkan.h"

  "common/rendering/rendering.c" "common/rendering/rendering.h"
  
  "common/device.c" "common/device.h"
  "common/device.queue_init.c" "common/device.queue_init.h"
  "common/general.c" "common/general.h"
  "common/image.c" "common/image.h"
  "common/renderer.c" "common/renderer.h"
  "common/shortcuts.c" "common/shortcuts.h"
  "common/surface.c" "common/surface.h"
  "common/swapchain.c" "common/swapchain.h"

  "input/clipboard.h"
  "input/keyboard.h"
  "input/mouse.h"

  "platform/init.h"
  "platform/input.h"
  "platform/lock.h"
  "platform/window.h")

find_package(Vulkan REQUIRED)

target_include_directories(Cpeed PRIVATE ${Vulkan_INCLUDE_DIRS})

set_property(TARGET Cpeed PROPERTY C_STANDARD 99)
