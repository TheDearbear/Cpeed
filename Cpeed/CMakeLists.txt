# CMakeList.txt : CMake project for Cpeed, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (Cpeed
  "backend.c" "backend.h"
  "input.h"
  "platform.h"

  "common/general.c" "common/general.h"

  "input/clipboard.h"
  "input/gamepad.h"
  "input/keyboard.h"
  "input/mouse.h"

  "platform/init.h"
  "platform/input.h"
  "platform/lock.h"
  "platform/window.h")

set_property(TARGET Cpeed PROPERTY C_STANDARD 99)
