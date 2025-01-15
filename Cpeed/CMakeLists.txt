# CMakeList.txt : CMake project for Cpeed, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (Cpeed "main.c" "main.h" "platform.h")

target_include_directories(Cpeed PUBLIC "$ENV{VULKAN_SDK}/Include")
target_link_libraries(Cpeed PUBLIC "$ENV{VULKAN_SDK}/Lib/vulkan-1.lib")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Cpeed PROPERTY C_STANDARD)
endif()
