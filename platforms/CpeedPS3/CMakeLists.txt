add_library (CpeedPS3 STATIC
  "ps3frame.c"
  "ps3init.c"
  "ps3input.c"
  "ps3logging.c"
  "ps3main.c" "ps3main.h"
  "ps3window.c")

target_sources (Cpeed PRIVATE
  "${Cpeed_SOURCE_DIR}/Cpeed/main.c"
)

message (CHECK_START "Checking for compatible Python")
find_package (Python3 "3.0...3.12" COMPONENTS Interpreter)

if (${Python3_FOUND})
  message (CHECK_PASS "found")
else ()
  message (CHECK_FAIL "not found. Project packaging will not be available")
endif ()

target_link_libraries (CpeedPS3 io lv2 rt)
target_include_directories (CpeedPS3 PRIVATE "${Cpeed_SOURCE_DIR}")

if (TARGET DearImGui)
  target_compile_definitions (DearImGui PUBLIC IMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS)

  target_link_libraries (CpeedPS3 DearImGui)
  target_include_directories (CpeedPS3 PRIVATE "${DearBindings_SOURCE_DIR}/imgui")
  target_include_directories (CpeedPS3 PRIVATE "${DearBindings_SOURCE_DIR}/dear_bindings/dcimgui")
endif ()

set (CPD_PS3_APP_ID "CPEED0001" CACHE STRING "Sets application id for PARAM.SFO file")
set (CPD_PS3_CONTENT_ID "UP0001-${CPD_PS3_APP_ID}_00-0000000000000000" CACHE STRING "Sets content id for PS3 executable and package file")

if (${Python3_FOUND})
  add_custom_target (Package ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/pkg"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/pkg/USRDIR"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/ICON0.PNG" "${CMAKE_BINARY_DIR}/pkg/ICON0.PNG"
    COMMAND "${CPD_PS3_DEV}/bin/sprxlinker" "${CMAKE_BINARY_DIR}/Cpeed/Cpeed${CMAKE_EXECUTABLE_SUFFIX_C}"
    COMMAND "${CPD_PS3_DEV}/bin/make_self" "${CMAKE_BINARY_DIR}/Cpeed/Cpeed${CMAKE_EXECUTABLE_SUFFIX_C}" "${CMAKE_BINARY_DIR}/Cpeed/Cpeed.self"
    COMMAND "${CPD_PS3_DEV}/bin/make_self_npdrm" "${CMAKE_BINARY_DIR}/Cpeed/Cpeed${CMAKE_EXECUTABLE_SUFFIX_C}" "${CMAKE_BINARY_DIR}/pkg/USRDIR/EBOOT.BIN" "${CPD_PS3_CONTENT_ID}"
    COMMAND ${Python3_EXECUTABLE} "${CPD_PS3_DEV}/bin/sfo.py" "--appid" "${CPD_PS3_APP_ID}" "-f" "${CMAKE_CURRENT_SOURCE_DIR}/sfo.xml" "${CMAKE_BINARY_DIR}/pkg/PARAM.SFO"
    COMMAND ${Python3_EXECUTABLE} "${CPD_PS3_DEV}/bin/pkg.py" "--contentid" "${CPD_PS3_CONTENT_ID}" "${CMAKE_BINARY_DIR}/pkg/"
    COMMAND "${CPD_PS3_DEV}/bin/package_finalize" "${CMAKE_BINARY_DIR}/${CPD_PS3_CONTENT_ID}.pkg"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    USES_TERMINAL)
endif ()

set_property(TARGET CpeedPS3 PROPERTY C_STANDARD 99)
