/*
 * Tests for sc_string_builder.h
 */
#include "greatest.h"
#include "sc_string_builder.h"

TEST string_builder_basic(void) {
    sc_string_builder_t sb;
    sc_sb_init(&sb, 0);
    ASSERT_EQ(0, sc_sb_append_cstr(&sb, "Hello"));
    ASSERT_EQ(0, sc_sb_append_cstr(&sb, " "));
    ASSERT_EQ(0, sc_sb_append_cstr(&sb, "World"));
    SC_String *s = sc_sb_finish(&sb);
    ASSERT(s != NULL);
    ASSERT_EQ(11, (int)s->length);
    ASSERT(memcmp(s->data, "Hello World", 11) == 0);
    sc_string_free(s);
    PASS();
}

TEST string_builder_append_variants(void) {
    sc_string_builder_t sb;
    sc_sb_init(&sb, 8);
    ASSERT_EQ(0, sc_sb_append_char(&sb, '['));
    ASSERT_EQ(4, sc_sb_append_printf(&sb, "%s", "core"));
    ASSERT_EQ(0, sc_sb_append_char(&sb, ']'));
    SC_String *tail = sc_string_from_cstr("-ok");
    ASSERT(tail != NULL);
    ASSERT_EQ(0, sc_sb_append_string(&sb, tail));
    sc_string_free(tail);
    SC_String *s = sc_sb_finish(&sb);
    ASSERT(s != NULL);
    ASSERT_EQ(9, (int)s->length);
    ASSERT(memcmp(s->data, "[core]-ok", 9) == 0);
    sc_string_free(s);
    PASS();
}

TEST string_builder_arena_pipeline_no_extra_space_expected(void) {
    sc_arena_t arena;
    ASSERT_EQ(0, sc_arena_init(&arena, 512));
    sc_string_builder_t sb;
    sc_sb_init_arena(&sb, &arena, 128);
    ASSERT_EQ(0, sc_sb_append_cstr(&sb, "arena"));
    ASSERT_EQ(0, sc_sb_append_char(&sb, '-'));
    ASSERT_EQ(2, sc_sb_append_printf(&sb, "%d", 10));
    const size_t used_before_finish = sc_arena_used(&arena);
    SC_String *s = sc_sb_finish(&sb);
    ASSERT(s != NULL);
    ASSERT_EQ(8, (int)s->length);
    ASSERT(memcmp(s->data, "arena-10", 8) == 0);
    ASSERT(sc_arena_used(&arena) >= used_before_finish);
    sc_arena_destroy(&arena);
    PASS();
}

SUITE(sc_string_builder_suite) {
    RUN_TEST(string_builder_basic);
    RUN_TEST(string_builder_append_variants);
    RUN_TEST(string_builder_arena_pipeline_no_extra_space_expected);
}
