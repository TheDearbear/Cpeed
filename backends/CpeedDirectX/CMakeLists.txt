add_library (CpeedDirectX STATIC
  "backend.c" "backend.h"
  "directx.h")

message ("-- Enabling DirectX backend support")

target_compile_definitions (Cpeed PRIVATE CPD_DIRECTX_ENABLED)
target_compile_definitions (CpeedDirectX PRIVATE CPD_DIRECTX_ENABLED)

target_link_libraries (CpeedDirectX d3d11 dxgi dxguid)
target_include_directories (CpeedDirectX PRIVATE "${Cpeed_SOURCE_DIR}")

target_sources (Cpeed PRIVATE "${Cpeed_SOURCE_DIR}/Cpeed/platform/backend/directx.h")

if (TARGET DearImGui)
  target_link_libraries (CpeedDirectX DearImGui)
  target_include_directories (CpeedDirectX PRIVATE "${DearBindings_SOURCE_DIR}/imgui")
  target_include_directories (CpeedDirectX PRIVATE "${DearBindings_SOURCE_DIR}/dear_bindings/dcimgui")
endif ()

set_property (TARGET CpeedDirectX PROPERTY C_STANDARD 99)
