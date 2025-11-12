# CMakeList.txt : CMake project for Cpeed, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (Cpeed
  "backend.c" "backend.h"
  "input.h"
  "platform.h"

  "common/frame_layers/test_layer.c" "common/frame_layers/test_layer.h"

  "common/frame.c" "common/frame.h"
  "common/init.c" "common/init.h"
  "common/math.c" "common/math.h"

  "input/clipboard.h"
  "input/gamepad.h"
  "input/keyboard.h"
  "input/mouse.h"
  
  "platform/input/gamepad.h"
  "platform/input/mode.h"
  "platform/input/queue.h"

  "platform/init.h"
  "platform/lock.h"
  "platform/logging.h"
  "platform/window.h")

if (TARGET DearImGui)
  target_link_libraries (Cpeed PRIVATE DearImGui)
  target_include_directories (Cpeed PUBLIC "${DearBindings_SOURCE_DIR}/imgui")
  target_include_directories (Cpeed PUBLIC "${DearBindings_SOURCE_DIR}/dear_bindings/dcimgui")

  target_sources (Cpeed PRIVATE
    "common/imgui/imgui_impl_cpeed.c" "common/imgui/imgui_impl_cpeed.h")
endif ()

set_property(TARGET Cpeed PROPERTY C_STANDARD 99)
