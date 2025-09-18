add_library (CpeedDirectX STATIC
  "backend.c" "backend.h"
  "directx.h")

message ("-- Enabling DirectX backend support")

target_compile_definitions(Cpeed PRIVATE CPD_DIRECTX_ENABLED)
target_compile_definitions(CpeedDirectX PRIVATE CPD_DIRECTX_ENABLED)

target_link_libraries(CpeedDirectX PRIVATE d3d11 dxgi dxguid)

target_sources(Cpeed PRIVATE "../platform/backend/directx.h")

set_property(TARGET CpeedDirectX PROPERTY C_STANDARD 99)
