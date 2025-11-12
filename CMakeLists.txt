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
  set (CPD_COMPILE_PLATFORM CpeedUWP)
  set (CPD_COMPILE_BACKENDS CpeedDirectX)
elseif (WIN32)
  set (CPD_COMPILE_PLATFORM CpeedWindows)
  set (CPD_COMPILE_BACKENDS CpeedDirectX CpeedVulkan)
elseif (LINUX)
  set (CPD_COMPILE_PLATFORM CpeedLinux)
  set (CPD_COMPILE_BACKENDS CpeedVulkan)
else ()
  message (FATAL_ERROR "Unsupported platform specified")
endif (WINDOWS_STORE)

# Include sub-projects.
add_subdirectory ("deps")

if (TARGET DearImGui)
  add_compile_definitions (CPD_IMGUI_AVAILABLE)
endif ()

add_subdirectory ("Cpeed")

foreach (BACKEND ${CPD_COMPILE_BACKENDS})
  add_subdirectory ("backends/${BACKEND}")
  target_link_libraries (Cpeed PRIVATE ${BACKEND})
endforeach ()

add_subdirectory ("platforms/${CPD_COMPILE_PLATFORM}")
target_link_libraries (Cpeed PRIVATE ${CPD_COMPILE_PLATFORM})

target_include_directories (Cpeed PRIVATE "${Cpeed_SOURCE_DIR}")
