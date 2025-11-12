cmake_minimum_required (VERSION 3.19)

project (DearBindings CXX)

find_package (Python3 3.10 REQUIRED COMPONENTS Interpreter)

message (CHECK_START "Installing requirements for dear_bindings")
execute_process (COMMAND "${Python3_EXECUTABLE}" "-m" "pip" "install" "--no-input" "--disable-pip-version-check" "-r" "${CMAKE_CURRENT_SOURCE_DIR}/dear_bindings/requirements.txt" RESULT_VARIABLE CPD_PIP_RESULT)
if (${CPD_PIP_RESULT} EQUAL 0)
  message (CHECK_PASS "ok")
else ()
  message (CHECK_FAIL "failed, make sure that required packages are installed")
endif ()

file (MAKE_DIRECTORY "dear_bindings/dcimgui")

add_custom_command (
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/dear_bindings/dcimgui/dcimgui.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/dear_bindings/dcimgui/dcimgui.h"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/dear_bindings"
  COMMAND "${Python3_EXECUTABLE}" ARGS "dear_bindings.py" "-o" "dcimgui/dcimgui" "../imgui/imgui.h"
  VERBATIM
)

add_library (DearImGui STATIC
  "${CMAKE_CURRENT_SOURCE_DIR}/dear_bindings/dcimgui/dcimgui.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/dear_bindings/dcimgui/dcimgui.h"

  "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_demo.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  string(APPEND CMAKE_CXX_FLAGS " -fno-threadsafe-statics")
  target_link_libraries (DearImGui "stdc++")
endif ()

target_include_directories (DearImGui PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/imgui")
target_include_directories (DearImGui PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends")

find_library (LIB_M m)
if (LIB_M)
  target_link_libraries (DearImGui ${LIB_M})
endif ()

if (CpeedDirectX IN_LIST CPD_BACKENDS_TARGET)
  add_custom_command (
    OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/dear_bindings/dcimgui/dcimgui_impl_dx11.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/dear_bindings/dcimgui/dcimgui_impl_dx11.h"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/dear_bindings"
    COMMAND "${Python3_EXECUTABLE}" ARGS "dear_bindings.py" "--backend" "-o" "dcimgui/dcimgui_impl_dx11" "--include" "../imgui/imgui.h" "../imgui/backends/imgui_impl_dx11.h"
    VERBATIM
  )

  target_sources (DearImGui PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/dear_bindings/dcimgui/dcimgui_impl_dx11.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/dear_bindings/dcimgui/dcimgui_impl_dx11.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_dx11.cpp"
  )
endif ()

if (CpeedVulkan IN_LIST CPD_BACKENDS_TARGET)
  find_package (Vulkan REQUIRED)
  
  target_compile_definitions (DearImGui PRIVATE VK_NO_PROTOTYPES)
  target_include_directories (DearImGui PRIVATE ${Vulkan_INCLUDE_DIRS})

  add_custom_command (
    OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/dear_bindings/dcimgui/dcimgui_impl_vulkan.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/dear_bindings/dcimgui/dcimgui_impl_vulkan.h"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/dear_bindings"
    COMMAND "${Python3_EXECUTABLE}" ARGS "dear_bindings.py" "--backend" "-o" "dcimgui/dcimgui_impl_vulkan" "--include" "../imgui/imgui.h" "../imgui/backends/imgui_impl_vulkan.h"
    VERBATIM
  )

  target_sources (DearImGui PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/dear_bindings/dcimgui/dcimgui_impl_vulkan.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/dear_bindings/dcimgui/dcimgui_impl_vulkan.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_vulkan.cpp"
  )
endif ()
