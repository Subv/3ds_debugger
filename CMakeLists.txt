cmake_minimum_required(VERSION 2.8)
project(RemoteDebugger C CXX ASM)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(Tools3DS)

find_package(CTRULIB REQUIRED)

option(BUILD_DUMMY_PROC "Builds the dummy process to debug" ON)
option(BUILD_DEBUGGER_POC "Builds the Proof of Concept debugger" ON)
option(BUILD_REMOTE_DEBUGGER "Builds the remote debugger daemon" ON)

# Set the output directory to 'bin'
# TODO
# set(OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Set the C and C++ build flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${DKA_SUGGESTED_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${DKA_SUGGESTED_CXX_FLAGS}")

if (NOT BANNERTOOL)
    # Bannertool workaround
    find_program(BANNERTOOL bannertool ${CMAKE_SOURCE_DIR}/tools/bannertool)
    if(BANNERTOOL)
        message(STATUS "bannertool: ${BANNERTOOL} - found")
    else()
        message(WARNING "bannertool - not found")
    endif()
endif()

# Add the libfmt library
set(HAVE_OPEN OFF) # Disable the POSIX layer in fmtlib
add_subdirectory(externals/fmtlib)

if (BUILD_DUMMY_PROC)
    add_subdirectory(dummy_process)
endif()

if (BUILD_DEBUGGER_POC)
    add_subdirectory(debugger_poc)
endif()

if (BUILD_REMOTE_DEBUGGER)
    add_subdirectory(debugger_daemon)
endif()