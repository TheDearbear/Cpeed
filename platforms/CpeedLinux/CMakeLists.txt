add_library (CpeedLinux STATIC
  "linuxevent.c" "linuxevent.h"
  "linuxframe.c"
  "linuxinit.c"
  "linuxinput.c"
  "linuxlock.c"
  "linuxlogging.c"
  "linuxmain.c" "linuxmain.h"
  "linuxwayland.c" "linuxwayland.h"
  "linuxwindow.c"
  "linuxwindowlist.c" "linuxwindowlist.h")

find_package (ECM REQUIRED NO_MODULE)
set (CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

find_package (Wayland REQUIRED Client)
find_package (WaylandProtocols REQUIRED)
find_package (WaylandScanner REQUIRED)

find_package (X11 REQUIRED COMPONENTS xkbcommon)

find_package (PkgConfig REQUIRED)
pkg_check_modules (EVDEV REQUIRED "libevdev")
pkg_check_modules (SYSTEMD REQUIRED "libsystemd")

target_sources (CpeedVulkan PRIVATE "backend/vulkan.c")
target_sources (Cpeed PRIVATE "${Cpeed_SOURCE_DIR}/Cpeed/main.c")

file(MAKE_DIRECTORY "xdg-decoration" "xdg-shell")

add_custom_command (OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/xdg-decoration/client.h" "${CMAKE_CURRENT_SOURCE_DIR}/xdg-decoration/private.c"
  COMMAND ${WaylandScanner_EXECUTABLE} ARGS "client-header" "${WaylandProtocols_DATADIR}/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml" "${CMAKE_CURRENT_SOURCE_DIR}/xdg-decoration/client.h"
  COMMAND ${WaylandScanner_EXECUTABLE} ARGS "private-code" "${WaylandProtocols_DATADIR}/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml" "${CMAKE_CURRENT_SOURCE_DIR}/xdg-decoration/private.c"
  DEPENDS "${WaylandProtocols_DATADIR}/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml"
  VERBATIM
)

add_custom_command (OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/xdg-shell/client.h" "${CMAKE_CURRENT_SOURCE_DIR}/xdg-shell/private.c"
  COMMAND ${WaylandScanner_EXECUTABLE} ARGS "client-header" "${WaylandProtocols_DATADIR}/stable/xdg-shell/xdg-shell.xml" "${CMAKE_CURRENT_SOURCE_DIR}/xdg-shell/client.h"
  COMMAND ${WaylandScanner_EXECUTABLE} ARGS "private-code" "${WaylandProtocols_DATADIR}/stable/xdg-shell/xdg-shell.xml" "${CMAKE_CURRENT_SOURCE_DIR}/xdg-shell/private.c"
  DEPENDS "${WaylandProtocols_DATADIR}/stable/xdg-shell/xdg-shell.xml"
  VERBATIM
)

add_library (WaylandExtensions OBJECT)
target_sources (WaylandExtensions PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/xdg-decoration/client.h" "${CMAKE_CURRENT_SOURCE_DIR}/xdg-decoration/private.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/xdg-shell/client.h" "${CMAKE_CURRENT_SOURCE_DIR}/xdg-shell/private.c")

target_include_directories (CpeedLinux
  PRIVATE ${Wayland_INCLUDE_DIRS}
  PRIVATE ${X11_xkbcommon_INCLUDE_PATH}
  PRIVATE ${EVDEV_INCLUDE_DIRS}
  PRIVATE ${SYSTEMD_INCLUDE_DIRS}
  PRIVATE ${Cpeed_SOURCE_DIR}
)

target_link_libraries (CpeedLinux
  ${Wayland_LIBRARIES}
  ${X11_xkbcommon_LIB}
  ${EVDEV_LIBRARIES}
  ${SYSTEMD_LIBRARIES}
  WaylandExtensions
)

if (TARGET DearImGui)
  target_link_libraries (CpeedLinux DearImGui)
  target_include_directories (CpeedLinux PRIVATE "${DearBindings_SOURCE_DIR}/imgui")
  target_include_directories (CpeedLinux PRIVATE "${DearBindings_SOURCE_DIR}/dear_bindings/dcimgui")
endif ()

set_property (TARGET CpeedLinux PROPERTY C_STANDARD 99)
