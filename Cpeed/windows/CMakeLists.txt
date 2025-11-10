add_library (CpeedWindows STATIC
  "winframe.c"
  "wininit.c"
  "wininput.c"
  "winlock.c"
  "winlogging.c"
  "winmain.c" "winmain.h"
  "winproc.c" "winproc.h"
  "winwindow.c")

target_sources(CpeedVulkan PRIVATE "backend/vulkan.c")
target_sources(Cpeed PRIVATE
  "../main.c"

  "Cpeed.rc"
  "icon.ico")

target_link_libraries(CpeedWindows gameinput)

set_property(TARGET CpeedWindows PROPERTY C_STANDARD 99)
