set(SOURCE_FILES
    source/main.cpp
)

add_executable(dummy_process ${SOURCE_FILES})
target_link_libraries(dummy_process 3ds::ctrulib)

add_cia_target(dummy_process dummy_process.rsf meta/banner.cgfx meta/audio.wav "DummyProc" "Dummy debuggable process" "Subv" meta/icon.png)