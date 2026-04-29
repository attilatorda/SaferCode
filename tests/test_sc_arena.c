/*
 * Tests for sc_arena.h
 * Build:  cc -I../../include -o test_sc_arena \
 *             test_sc_arena.c
 */
#include "greatest.h"
#include "sc_arena.h"

TEST arena_alloc_and_reset(void) {
    sc_arena_t a;
    ASSERT_EQ(0, sc_arena_init(&a, 128));
    void *p1 = sc_arena_alloc(&a, 16);
    void *p2 = sc_arena_alloc(&a, 32);
    ASSERT(p1 != NULL);
    ASSERT(p2 != NULL);
    ASSERT(sc_arena_used(&a) > 0);
    sc_arena_reset(&a);
    ASSERT_EQ(0, (int)sc_arena_used(&a));
    sc_arena_destroy(&a);
    PASS();
}

SUITE(sc_arena_suite) {
    RUN_TEST(arena_alloc_and_reset);
}
