add_library (linuxmain STATIC "linuxmain.h" "linuxmain.c" "../platform.h")

target_include_directories(linuxmain PUBLIC "$ENV{VULKAN_SDK}/Include")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET linuxmain PROPERTY C_STANDARD)
endif()
