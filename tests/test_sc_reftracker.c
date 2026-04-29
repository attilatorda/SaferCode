/*
 * Tests for sc_reftracker.h
 * Build:  cc -I../../include -o test_sc_reftracker \
 *             test_sc_reftracker.c
 */
#include "greatest.h"
#include "sc_reftracker.h"

TEST reftracker_smoke(void) {
    PASS();
}

SUITE(sc_reftracker_suite) {
    RUN_TEST(reftracker_smoke);
}
