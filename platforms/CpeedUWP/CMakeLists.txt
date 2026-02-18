project (CpeedUWP C CXX)

add_library (CpeedUWP STATIC
  "AppView.cpp" "AppView.h"
  "main.c" "main.h"
  "uwpframe.c"
  "uwpinit.c"
  "uwpinput.c"
  "uwplock.c"
  "uwplogging.c"
  "uwpmain.c" "uwpmain.h"
  "uwpqueue.c" "uwpqueue.h"
  "uwpthread.c"
  "uwpwindow.c")

target_sources (Cpeed PRIVATE "mainstub.c")

target_link_libraries (CpeedUWP uuid)
target_include_directories (CpeedUWP PRIVATE "${Cpeed_SOURCE_DIR}")

if (TARGET DearImGui)
  target_link_libraries (CpeedUWP DearImGui)
  target_include_directories (CpeedUWP PRIVATE "${DearBindings_SOURCE_DIR}/imgui")
  target_include_directories (CpeedUWP PRIVATE "${DearBindings_SOURCE_DIR}/dear_bindings/dcimgui")
endif ()

set_property (TARGET CpeedUWP PROPERTY C_STANDARD 99)
set_property (TARGET CpeedUWP PROPERTY CXX_STANDARD 17)
set_property (TARGET CpeedUWP PROPERTY VS_WINRT_COMPONENT)
