add_library (CpeedWindows STATIC "winmain.h" "winmain.c" "../platform.h")

target_include_directories(CpeedWindows PUBLIC "$ENV{VULKAN_SDK}/Include")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET CpeedWindows PROPERTY C_STANDARD)
endif()
