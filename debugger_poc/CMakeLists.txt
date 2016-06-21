set(SOURCE_FILES
    source/main.cpp
)

add_executable(debugger_poc ${SOURCE_FILES})
target_link_libraries(debugger_poc 3ds::ctrulib)

add_3dsx_target(debugger_poc)
