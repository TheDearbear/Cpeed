add_library (CpeedLinux STATIC "linuxmain.h" "linuxmain.c" "../platform.h")

target_include_directories(CpeedLinux PUBLIC "$ENV{VULKAN_SDK}/Include")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET CpeedLinux PROPERTY C_STANDARD)
endif()
