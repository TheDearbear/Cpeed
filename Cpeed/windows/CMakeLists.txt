add_library (winmain STATIC "winmain.h" "winmain.c" "../platform.h")

target_include_directories(winmain PUBLIC "$ENV{VULKAN_SDK}/Include")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET winmain PROPERTY C_STANDARD)
endif()
