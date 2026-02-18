function (add_stable_wayland_protocol protocol_name protocol_target)
  file (MAKE_DIRECTORY ${protocol_name})

  add_custom_command (OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/${protocol_name}/client.h" "${CMAKE_CURRENT_SOURCE_DIR}/${protocol_name}/private.c"
    COMMAND ${WaylandScanner_EXECUTABLE} ARGS "client-header" "${WaylandProtocols_DATADIR}/stable/${protocol_name}/${protocol_name}.xml" "${CMAKE_CURRENT_SOURCE_DIR}/${protocol_name}/client.h"
    COMMAND ${WaylandScanner_EXECUTABLE} ARGS "private-code" "${WaylandProtocols_DATADIR}/stable/${protocol_name}/${protocol_name}.xml" "${CMAKE_CURRENT_SOURCE_DIR}/${protocol_name}/private.c"
    DEPENDS "${WaylandProtocols_DATADIR}/stable/${protocol_name}/${protocol_name}.xml"
    VERBATIM
  )

  target_sources (${protocol_target} PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/${protocol_name}/client.h" "${CMAKE_CURRENT_SOURCE_DIR}/${protocol_name}/private.c"
  )
endfunction ()

function (add_staging_wayland_protocol protocol_name protocol_version protocol_target)
  file (MAKE_DIRECTORY ${protocol_name})

  add_custom_command (OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/${protocol_name}/client.h" "${CMAKE_CURRENT_SOURCE_DIR}/${protocol_name}/private.c"
    COMMAND ${WaylandScanner_EXECUTABLE} ARGS "client-header" "${WaylandProtocols_DATADIR}/staging/${protocol_name}/${protocol_name}-v${protocol_version}.xml" "${CMAKE_CURRENT_SOURCE_DIR}/${protocol_name}/client.h"
    COMMAND ${WaylandScanner_EXECUTABLE} ARGS "private-code" "${WaylandProtocols_DATADIR}/staging/${protocol_name}/${protocol_name}-v${protocol_version}.xml" "${CMAKE_CURRENT_SOURCE_DIR}/${protocol_name}/private.c"
    DEPENDS "${WaylandProtocols_DATADIR}/staging/${protocol_name}/${protocol_name}-v${protocol_version}.xml"
    VERBATIM
  )

  target_sources (${protocol_target} PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/${protocol_name}/client.h" "${CMAKE_CURRENT_SOURCE_DIR}/${protocol_name}/private.c"
  )
endfunction ()

function (add_unstable_wayland_protocol protocol_name protocol_version protocol_target)
  file (MAKE_DIRECTORY ${protocol_name})

  add_custom_command (OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/${protocol_name}/client.h" "${CMAKE_CURRENT_SOURCE_DIR}/${protocol_name}/private.c"
    COMMAND ${WaylandScanner_EXECUTABLE} ARGS "client-header" "${WaylandProtocols_DATADIR}/unstable/${protocol_name}/${protocol_name}-unstable-v${protocol_version}.xml" "${CMAKE_CURRENT_SOURCE_DIR}/${protocol_name}/client.h"
    COMMAND ${WaylandScanner_EXECUTABLE} ARGS "private-code" "${WaylandProtocols_DATADIR}/unstable/${protocol_name}/${protocol_name}-unstable-v${protocol_version}.xml" "${CMAKE_CURRENT_SOURCE_DIR}/${protocol_name}/private.c"
    DEPENDS "${WaylandProtocols_DATADIR}/unstable/${protocol_name}/${protocol_name}-unstable-v${protocol_version}.xml"
    VERBATIM
  )

  target_sources (${protocol_target} PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/${protocol_name}/client.h" "${CMAKE_CURRENT_SOURCE_DIR}/${protocol_name}/private.c"
  )
endfunction ()
