# This is a custom script to easily include Microsoft GDK
#
# This script uses 'GDK_VERSION' variable to select which
# version of GDK script should include into CMake

set (GDK_VERSION "" CACHE STRING "Version of Microsoft GDK to use")

if ("${GDK_VERSION}" STREQUAL "")
  if (NOT DEFINED ENV{GameDKCoreLatest})
    message (FATAL_ERROR "Unable to find installation of GDK. Please set 'GameDKCoreLatest' environment variable.")
  endif ()

  set (GDK_PATH "$ENV{GameDKCoreLatest}")
else ()
  if (NOT DEFINED ENV{GameDK})
    message (FATAL_ERROR "Unable to find installation of GDK. Please set 'GameDK' environment variable.")
  endif ()

  set (GDK_PATH "")
  cmake_path (APPEND GDK_PATH "$ENV{GameDK}" "${GDK_VERSION}")
endif ()

cmake_path (APPEND GDK_PATH "${GDK_PATH}" "windows")

if ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "AMD64")
  set (GDK_ARCH "x64")
elseif ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "ARM64")
  set (GDK_ARCH "arm64")
else ()
  message (FATAL_ERROR "Processor architecture '${CMAKE_SYSTEM_PROCESSOR}' is not supported by GDK")
endif ()

set (GDK_LIBRARY_DIR "" PARENT_SCOPE)
cmake_path (APPEND GDK_LIBRARY_DIR "${GDK_PATH}" "lib" "${GDK_ARCH}")

link_directories ("${GDK_LIBRARY_DIR}")

set (GDK_INCLUDE_DIR "" PARENT_SCOPE)
cmake_path (APPEND GDK_INCLUDE_DIR "${GDK_PATH}" "include")
