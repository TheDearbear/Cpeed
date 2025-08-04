add_library (CpeedWindows STATIC "winmain.h" "winmain.c" "winwindow.c")

find_package(Vulkan REQUIRED)

target_include_directories(CpeedWindows PRIVATE ${Vulkan_INCLUDE_DIRS})

set_property(TARGET CpeedWindows PROPERTY C_STANDARD 99)
