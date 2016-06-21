set(SOURCE_FILES
    source/logger.cpp
    source/main.cpp
    source/network_manager.cpp
)

set(HEADER_FILES
    source/logger.h
    source/network_manager.h
    source/utils.h
)

add_executable(debugger_daemon ${SOURCE_FILES} ${HEADER_FILES})
target_link_libraries(debugger_daemon 3ds::ctrulib fmt)

add_cia_target(debugger_daemon debugger_daemon.rsf meta/banner.cgfx meta/audio.wav "Network Debugger" "Network Debugger Daemon" "Subv" meta/icon.png)