/*
 * Tests for sc_version.h
 * Build:  cc -I../../include -o test_sc_version \
 *             test_sc_version.c
 */
#include "greatest.h"
#include "sc_version.h"

TEST version_macros_exist(void) {
    ASSERT_EQ(0, SC_VERSION_MAJOR);
    ASSERT_EQ(9, SC_VERSION_MINOR);
    ASSERT_EQ(1, SC_VERSION_PATCH);
    ASSERT_EQ(SC_MAKE_VERSION(0, 9, 1), SC_VERSION);
    PASS();
}

SUITE(sc_version_suite) {
    RUN_TEST(version_macros_exist);
}
