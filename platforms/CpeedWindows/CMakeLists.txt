add_library (CpeedWindows STATIC
  "winframe.c"
  "wininit.c"
  "wininput.c"
  "winlock.c"
  "winlogging.c"
  "winmain.c" "winmain.h"
  "winproc.c" "winproc.h"
  "winthread.c"
  "winwindow.c")

target_sources (CpeedVulkan PRIVATE "backend/vulkan.c")
target_sources (Cpeed PRIVATE
  "${Cpeed_SOURCE_DIR}/Cpeed/main.c"

  "Cpeed.rc"
  "icon.ico")

target_link_libraries (CpeedWindows gameinput)
target_include_directories (CpeedWindows PRIVATE "${Cpeed_SOURCE_DIR}")

if (TARGET DearImGui)
  target_link_libraries (CpeedWindows DearImGui)
  target_include_directories (CpeedWindows PRIVATE "${DearBindings_SOURCE_DIR}/imgui")
  target_include_directories (CpeedWindows PRIVATE "${DearBindings_SOURCE_DIR}/dear_bindings/dcimgui")
endif ()

set_property(TARGET CpeedWindows PROPERTY C_STANDARD 99)
