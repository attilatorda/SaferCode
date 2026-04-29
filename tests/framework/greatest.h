/* Minimal Greatest-compatible test shim for this repository. */
#ifndef GREATEST_H
#define GREATEST_H

#include <stdio.h>
#include <stdlib.h>

typedef int greatest_test_res;
#define TEST static greatest_test_res
#define SUITE(name) static void name(void)

#define PASS() return 0
#define FAIL() return 1
#define ASSERT(x) do { if (!(x)) return 1; } while (0)
#define ASSERT_EQ(a,b) ASSERT((a)==(b))
#define ASSERT_NEQ(a,b) ASSERT((a)!=(b))

static int _greatest_failures = 0;

#define RUN_TEST(fn) do { \
    int _r = (fn)(); \
    if (_r != 0) { \
        _greatest_failures++; \
        printf("[FAIL] %s\n", #fn); \
    } else { \
        printf("[ OK ] %s\n", #fn); \
    } \
} while (0)

#define RUN_SUITE(fn) do { \
    printf("\n== Suite: %s ==\n", #fn); \
    (fn)(); \
} while (0)

#define GREATEST_MAIN_DEFS()
#define GREATEST_MAIN_BEGIN() ((void)argc, (void)argv)
#define GREATEST_MAIN_END() return _greatest_failures ? EXIT_FAILURE : EXIT_SUCCESS

#endif
