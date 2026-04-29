/*
 * Tests for sc_panic.h
 * Build:  cc -I../../include -o test_sc_panic test_sc_panic.c
 */
#include "greatest.h"
#include "sc_panic.h"
#include <string.h>

/* Flag to track if custom handler was called */
static int custom_handler_called = 0;
static char last_panic_message[256] = {0};
static int panic_log_hook_called = 0;

/* Custom panic handler for testing */
static void test_panic_handler(const char *msg) {
    custom_handler_called = 1;
    if (msg) {
        strncpy(last_panic_message, msg, sizeof(last_panic_message) - 1);
        last_panic_message[sizeof(last_panic_message) - 1] = '\0';
    }
    /* Don't actually exit/abort in tests */
}

static void test_panic_log_hook(const char *msg) {
    (void)msg;
    panic_log_hook_called = 1;
}

/* Test 1: Default panic handler exists */
TEST panic_default_handler_exists(void) {
    sc_panic_handler_t handler = sc_get_panic_handler();
    ASSERT(handler != NULL);
    PASS();
}

/* Test 2: Can set a custom panic handler */
TEST panic_set_custom_handler(void) {
    custom_handler_called = 0;
    memset(last_panic_message, 0, sizeof(last_panic_message));

    /* Install custom handler */
    sc_set_panic_handler(test_panic_handler);
    sc_panic_handler_t handler = sc_get_panic_handler();
    ASSERT_EQ(handler, test_panic_handler);

    PASS();
}

/* Test 3: Custom handler is called with message */
TEST panic_handler_receives_message(void) {
    custom_handler_called = 0;
    memset(last_panic_message, 0, sizeof(last_panic_message));

    sc_set_panic_handler(test_panic_handler);
    sc_panic("test error message");

    ASSERT(custom_handler_called);
    ASSERT(strcmp(last_panic_message, "test error message") == 0);

    PASS();
}

/* Test 4: Can restore default handler */
TEST panic_restore_default_handler(void) {
    sc_panic_handler_t original = sc_get_panic_handler();

    /* Set to custom */
    sc_set_panic_handler(test_panic_handler);
    ASSERT_EQ(sc_get_panic_handler(), test_panic_handler);

    /* Restore (NULL means default) */
    sc_set_panic_handler(NULL);
    sc_panic_handler_t restored = sc_get_panic_handler();
    ASSERT_NEQ(restored, test_panic_handler);

    PASS();
}

/* Test 5: SC_PANIC macro works */
TEST panic_macro(void) {
    custom_handler_called = 0;
    memset(last_panic_message, 0, sizeof(last_panic_message));

    sc_set_panic_handler(test_panic_handler);
    SC_PANIC("macro test");

    ASSERT(custom_handler_called);
    ASSERT(strcmp(last_panic_message, "macro test") == 0);

    PASS();
}

/* Test 6: Panic with NULL message doesn't crash */
TEST panic_null_message(void) {
    custom_handler_called = 0;
    sc_set_panic_handler(test_panic_handler);
    sc_panic(NULL);
    /* If we reach here, the test passed (no crash) */
    PASS();
}

/* Test 7: Can set/get panic log hook */
TEST panic_log_hook_set_get(void) {
    sc_set_panic_log_hook(test_panic_log_hook);
    ASSERT_EQ(sc_get_panic_log_hook(), test_panic_log_hook);

    sc_set_panic_log_hook(NULL);
    ASSERT_EQ(sc_get_panic_log_hook(), NULL);
    PASS();
}

/* Test 8: Log hook installation does not affect custom panic handler flow */
TEST panic_log_hook_custom_handler_unchanged(void) {
    custom_handler_called = 0;
    panic_log_hook_called = 0;
    memset(last_panic_message, 0, sizeof(last_panic_message));

    sc_set_panic_log_hook(test_panic_log_hook);
    sc_set_panic_handler(test_panic_handler);
    sc_panic("hook + handler test");

    ASSERT(custom_handler_called);
    ASSERT(strcmp(last_panic_message, "hook + handler test") == 0);
    ASSERT(panic_log_hook_called == 0);

    sc_set_panic_handler(NULL);
    sc_set_panic_log_hook(NULL);
    PASS();
}

SUITE(sc_panic_suite) {
    RUN_TEST(panic_default_handler_exists);
    RUN_TEST(panic_set_custom_handler);
    RUN_TEST(panic_handler_receives_message);
    RUN_TEST(panic_restore_default_handler);
    RUN_TEST(panic_macro);
    RUN_TEST(panic_null_message);
    RUN_TEST(panic_log_hook_set_get);
    RUN_TEST(panic_log_hook_custom_handler_unchanged);
}
