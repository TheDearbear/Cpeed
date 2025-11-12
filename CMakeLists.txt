# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.16)

cmake_policy (SET CMP0069 NEW)
cmake_policy (SET CMP0079 NEW)

option (CPD_IMGUI_ENABLED "Compiles and enables support of DearImGui in code" ON)

project ("Cpeed" C)

include (CheckIPOSupported)

message (CHECK_START "Checking for IPO support")
check_ipo_supported (RESULT ipo_supported OUTPUT ipo_error)
if (ipo_supported)
  message (CHECK_PASS "available")
  set (CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else ()
  message (CHECK_FAIL "not available: ${ipo_error}")
endif ()

if (WINDOWS_STORE)
  set (CPD_PLATFORM_TARGET CpeedUWP)
  set (CPD_PLATFORM_SUBDIRECTORY "Cpeed/uwp")

  set (CPD_BACKENDS_TARGET CpeedDirectX)
  set (CPD_BACKENDS_SUBDIRECTORY "Cpeed/directx")
elseif (WIN32)
  set (CPD_PLATFORM_TARGET CpeedWindows)
  set (CPD_PLATFORM_SUBDIRECTORY "Cpeed/windows")

  set (CPD_BACKENDS_TARGET CpeedDirectX CpeedVulkan)
  set (CPD_BACKENDS_SUBDIRECTORY "Cpeed/directx" "Cpeed/vulkan")
elseif (LINUX)
  set (CPD_PLATFORM_TARGET CpeedLinux)
  set (CPD_PLATFORM_SUBDIRECTORY "Cpeed/linux")

  set (CPD_BACKENDS_TARGET CpeedVulkan)
  set (CPD_BACKENDS_SUBDIRECTORY "Cpeed/vulkan")
else ()
  message (FATAL_ERROR "Unsupported platform specified")
endif (WINDOWS_STORE)

# Include sub-projects.
add_subdirectory ("deps")

if (TARGET DearImGui)
  add_compile_definitions (CPD_IMGUI_AVAILABLE)
endif ()

add_subdirectory ("Cpeed")

foreach (SUBDIR ${CPD_BACKENDS_SUBDIRECTORY})
  add_subdirectory (${SUBDIR})
endforeach ()

add_subdirectory (${CPD_PLATFORM_SUBDIRECTORY})

target_link_libraries (Cpeed PRIVATE ${CPD_PLATFORM_TARGET} ${CPD_BACKENDS_TARGET})
