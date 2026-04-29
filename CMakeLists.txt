cmake_minimum_required(VERSION 3.15)
project(SaferCode VERSION 0.9.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(SC_DEBUG_SENTINEL "Enable sentinel memory checking" ON)
if(SC_DEBUG_SENTINEL)
    add_compile_definitions(SC_DEBUG_SENTINEL)
endif()

# SaferCode is header-only; expose its include/ directory as an INTERFACE library.
add_library(safercode INTERFACE)
target_include_directories(safercode INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link pthreads on POSIX (needed by sc_sentinel.h background thread)
if(UNIX)
    target_link_libraries(safercode INTERFACE pthread)
endif()

# Link Winsock on Windows (needed by sc_log.h network target)
if(WIN32)
    target_link_libraries(safercode INTERFACE ws2_32)
endif()

# Test suite
enable_testing()
add_subdirectory(tests)
