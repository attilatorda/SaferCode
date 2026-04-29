cmake_minimum_required(VERSION 3.16)

# Omnibus binary that runs all suites
add_executable(test_all test_all.c)
target_include_directories(test_all PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/framework
)
target_link_libraries(test_all PRIVATE safercode)
add_test(NAME all COMMAND test_all)
