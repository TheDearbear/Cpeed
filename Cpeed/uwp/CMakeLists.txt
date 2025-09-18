project (CpeedUWP C CXX)

add_library (CpeedUWP STATIC
  "AppView.cpp" "AppView.h"
  "main.cpp" "main.h"
  "uwpinit.c"
  "uwpinput.c"
  "uwpmain.c" "uwpmain.h"
  "uwpqueue.c"
  "uwpwindow.c")

target_sources(Cpeed PRIVATE "mainstub.c")

set_property(TARGET CpeedUWP PROPERTY C_STANDARD 99)
set_property(TARGET CpeedUWP PROPERTY CXX_STANDARD 17)
set_property(TARGET CpeedUWP PROPERTY VS_WINRT_COMPONENT)
