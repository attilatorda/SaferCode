# cmake/CompilerWarnings.cmake
# Apply consistent warning flags to any target.
# Usage: target_apply_warnings(my_target)

function(target_apply_warnings TARGET)
    target_compile_options(${TARGET} PRIVATE
        -Wall -Wextra -Wshadow -Wconversion
        -Wpedantic -Wno-unused-parameter
    )
endfunction()
