# CMakeList.txt : CMake project for Cpeed, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (Cpeed "main.c" "main.h" "platform.h" "common/window.h" "common/renderer.h" "common/renderer.c")

target_include_directories(Cpeed PUBLIC "$ENV{VULKAN_SDK}/include")

if (WIN32)
  target_link_libraries(Cpeed PUBLIC "$ENV{VULKAN_SDK}/lib/vulkan-1.lib")
endif()

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Cpeed PROPERTY C_STANDARD)
endif()
