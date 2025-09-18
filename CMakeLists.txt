# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.16)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

cmake_policy(SET CMP0069 NEW)
cmake_policy(SET CMP0079 NEW)

include(CheckIPOSupported)

project ("Cpeed" C)

# Include sub-projects.
add_subdirectory ("Cpeed")

check_ipo_supported(RESULT ipo_supported)
if (ipo_supported)
  set_property(GLOBAL PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  message("-- IPO is enabled")
else()
  message("-- IPO is NOT enabled")
endif()

if (WIN32)
  add_subdirectory ("Cpeed/directx")
  add_subdirectory ("Cpeed/vulkan")
  add_subdirectory ("Cpeed/windows")
  target_link_libraries (Cpeed LINK_PRIVATE CpeedWindows CpeedDirectX CpeedVulkan)
elseif (LINUX)
  add_subdirectory ("Cpeed/vulkan")
  add_subdirectory ("Cpeed/linux")
  target_link_libraries (Cpeed LINK_PRIVATE CpeedLinux CpeedVulkan)
endif (WIN32)
