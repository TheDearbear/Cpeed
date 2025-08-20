add_library (CpeedWindows STATIC
    "wininit.c"
    "wininput.c"
    "winlock.c"
    "winmain.c" "winmain.h"
    "winproc.c" "winproc.h"
    "winwindow.c")

find_package(Vulkan REQUIRED)

target_include_directories(CpeedWindows PRIVATE ${Vulkan_INCLUDE_DIRS})

set_property(TARGET CpeedWindows PROPERTY C_STANDARD 99)
