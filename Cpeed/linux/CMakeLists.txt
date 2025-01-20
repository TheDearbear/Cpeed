add_library (CpeedLinux STATIC "linuxmain.h" "linuxmain.c" "linuxwindow.c")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET CpeedLinux PROPERTY C_STANDARD)
endif()
