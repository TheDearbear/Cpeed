add_library (CpeedVulkan STATIC
  "rendering/rendering.c" "rendering/rendering.h"
  
  "backend.c"   "backend.h"
  "device.c"    "device.h"
  "device.queue_init.c" "device.queue_init.h"
  "image.c"     "image.h"
  "renderer.c"  "renderer.h"
  "shortcuts.c" "shortcuts.h"
  "surface.c"   "surface.h"
  "swapchain.c" "swapchain.h"
  "vulkan.c"    "vulkan.h")

message ("-- Enabling Vulkan backend support")

find_package(Vulkan REQUIRED)

target_compile_definitions(Cpeed PRIVATE CPD_VULKAN_ENABLED)
target_compile_definitions(CpeedVulkan PRIVATE CPD_VULKAN_ENABLED VK_NO_PROTOTYPES)

target_include_directories(CpeedVulkan PRIVATE ${Vulkan_INCLUDE_DIRS})

target_sources(Cpeed PRIVATE "../platform/backend/vulkan.h")

if (TARGET DearImGui)
  target_link_libraries (CpeedVulkan DearImGui)
  target_include_directories (CpeedVulkan PRIVATE "${DearBindings_SOURCE_DIR}/imgui")
  target_include_directories (CpeedVulkan PRIVATE "${DearBindings_SOURCE_DIR}/dear_bindings/dcimgui")
endif ()

set_property(TARGET CpeedVulkan PROPERTY C_STANDARD 99)
