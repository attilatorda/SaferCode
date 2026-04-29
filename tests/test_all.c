/*
 * test_all.c — runs every test suite in one binary.
 * Build:  cc -I../include -o test_all test_all.c
 */
#include "greatest.h"

/* Keep omnibus tests stable: run test logic without debug sentinel hooks. */
#ifdef SC_DEBUG_SENTINEL
#undef SC_DEBUG_SENTINEL
#endif

#include "test_sc_version.c"
#include "test_sc_panic.c"
#include "test_sc_sentinel.c"
#include "test_sc_raii.c"
#include "test_sc_arena.c"
#include "test_sc_string.c"
#include "test_sc_string_builder.c"
#include "test_sc_log.c"
#include "test_sc_reftracker.c"
#include "test_sc_memfile.c"

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    puts("RUN sc_version_suite"); fflush(stdout);
    RUN_SUITE(sc_version_suite);
    puts("RUN sc_panic_suite"); fflush(stdout);
    RUN_SUITE(sc_panic_suite);
    puts("RUN sc_sentinel_suite"); fflush(stdout);
    RUN_SUITE(sc_sentinel_suite);
    puts("RUN sc_raii_suite"); fflush(stdout);
    RUN_SUITE(sc_raii_suite);
    puts("RUN sc_arena_suite"); fflush(stdout);
    RUN_SUITE(sc_arena_suite);
    puts("RUN sc_string_suite"); fflush(stdout);
    RUN_SUITE(sc_string_suite);
    puts("RUN sc_string_builder_suite"); fflush(stdout);
    RUN_SUITE(sc_string_builder_suite);
    puts("RUN sc_log_suite"); fflush(stdout);
    RUN_SUITE(sc_log_suite);
    puts("RUN sc_reftracker_suite"); fflush(stdout);
    RUN_SUITE(sc_reftracker_suite);
    puts("RUN sc_memfile_suite"); fflush(stdout);
    RUN_SUITE(sc_memfile_suite);
    GREATEST_MAIN_END();
}
