add_library (CpeedWindows STATIC
  "backend/vulkan.c"

  "wininit.c"
  "wininput.c"
  "winlock.c"
  "winmain.c" "winmain.h"
  "winproc.c" "winproc.h"
  "winwindow.c")

include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/EnableVulkanBackend.cmake")

target_sources(Cpeed PRIVATE "../main.c")

target_link_libraries(CpeedWindows gameinput)

set_property(TARGET CpeedWindows PROPERTY C_STANDARD 99)
