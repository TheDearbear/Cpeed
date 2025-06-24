add_library (CpeedWindows STATIC "winmain.h" "winmain.c" "winwindow.c")

target_include_directories(CpeedWindows PUBLIC "$ENV{VULKAN_SDK}/Include")

set_property(TARGET CpeedWindows PROPERTY C_STANDARD)
