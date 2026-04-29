cmake_minimum_required(VERSION 3.16)

foreach(EXAMPLE
    01_hello_arena
    02_string_builder
    03_reftracker
    04_logging
    05_arena_scratch_pattern
    06_url_parser
    07_log_formatter
    08_word_wrapper
    09_csv_to_json
)
    add_executable(${EXAMPLE} ${EXAMPLE}/main.c)
    target_include_directories(${EXAMPLE} PRIVATE ${CMAKE_SOURCE_DIR}/include)
endforeach()
