add_library (CpeedLinux STATIC
    "linuxinit.c"
    "linuxinput.c" 
    "linuxlock.c"
    "linuxmain.c" "linuxmain.h"
    "linuxwayland.c" "linuxwayland.h"
    "linuxwindow.c"
    "linuxwindowlist.c" "linuxwindowlist.h")

find_package (ECM REQUIRED NO_MODULE)
set (CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

find_package (Wayland REQUIRED Client)
find_package (WaylandProtocols REQUIRED)
find_package (WaylandScanner REQUIRED)

find_package(Vulkan REQUIRED)
find_package(X11 REQUIRED COMPONENTS xkbcommon)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/xdg-shell")

message("-- Generating client header for xdg-shell")

execute_process (
    COMMAND ${WaylandScanner_EXECUTABLE} "client-header"
        INPUT_FILE "${WaylandProtocols_DATADIR}/stable/xdg-shell/xdg-shell.xml"
        OUTPUT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/xdg-shell/client.h"
)

message("-- Generating code for xdg-shell")

execute_process (
    COMMAND ${WaylandScanner_EXECUTABLE} "private-code"
        INPUT_FILE "${WaylandProtocols_DATADIR}/stable/xdg-shell/xdg-shell.xml"
        OUTPUT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/xdg-shell/private.c"
)

target_sources (CpeedLinux PRIVATE "xdg-shell/private.c")

target_include_directories (CpeedLinux
    PRIVATE ${Vulkan_INCLUDE_DIRS}
    PRIVATE ${Wayland_INCLUDE_DIRS}
    PRIVATE ${X11_xkbcommon_INCLUDE_PATH}
)
target_link_libraries (CpeedLinux ${Wayland_LIBRARIES} ${X11_xkbcommon_LIB})

set_property (TARGET CpeedLinux PROPERTY C_STANDARD 99)
