add_library (CpeedRSX STATIC
  "backend.c" "backend.h"
  "device.h"
  
  "ppu-lv2-game.h")

message ("-- Enabling RSX backend support")

target_compile_definitions (Cpeed PRIVATE CPD_RSX_ENABLED)
target_compile_definitions (CpeedRSX PRIVATE CPD_RSX_ENABLED)

target_link_libraries (CpeedRSX rsx gcm_sys sysutil lv2)
target_include_directories (CpeedRSX PRIVATE "${Cpeed_SOURCE_DIR}")

if (TARGET DearImGui)
  target_link_libraries (CpeedRSX DearImGui)
  target_include_directories (CpeedRSX PRIVATE "${DearBindings_SOURCE_DIR}/imgui")
  target_include_directories (CpeedRSX PRIVATE "${DearBindings_SOURCE_DIR}/dear_bindings/dcimgui")

  target_sources (CpeedRSX PRIVATE
    "imgui/dcimgui_impl_rsx.c" "imgui/dcimgui_impl_rsx.h")
endif ()

set_property (TARGET CpeedRSX PROPERTY C_STANDARD 99)
