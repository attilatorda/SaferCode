/*
 * Tutorial solution: RAII Resource Safety
 * Build:  cc -I../../include -o solution solution.c
 *
 * Shows how to use AUTO_* macros for automatic resource cleanup.
 */
#include <stdio.h>
#include <string.h>
#include "sc_raii.h"
#include "sc_arena.h"

/* Demonstration: compare unsafe vs safe approaches */

void unsafe_approach(void) {
    printf("\n=== Unsafe Approach (Manual Cleanup) ===\n");

    /* Allocate buffer */
    char *buffer = malloc(256);
    if (!buffer) {
        printf("Failed to allocate\n");
        return;  // Oops! No cleanup needed, but in real code this happens
    }

    strcpy(buffer, "Hello from unsafe approach");
    printf("Buffer: %s\n", buffer);

    /* Must remember to free */
    free(buffer);
    printf("Buffer freed manually\n");
}

void safe_approach(void) {
    printf("\n=== Safe Approach (RAII with AUTO_FREE_PTR) ===\n");

    /* Allocate and bind cleanup automatically */
    AUTO_FREE_PTR char *buffer = malloc(256);
    if (!buffer) {
        printf("Failed to allocate\n");
        return;  /* Cleanup happens automatically even here */
    }

    strcpy(buffer, "Hello from safe approach");
    printf("Buffer: %s\n", buffer);

    printf("Buffer will be freed automatically at scope exit\n");
    /* No need for explicit free() - cleanup is automatic */
}

void with_arena(void) {
    printf("\n=== RAII with Arena ===\n");

    /* Arena is automatically destroyed at scope exit */
    AUTO_ARENA sc_arena_t arena;
    if (sc_arena_init(&arena, 512) != 0) {
        printf("Failed to init arena\n");
        return;  /* Cleanup happens automatically */
    }

    /* Allocate from arena */
    char *str1 = sc_arena_strdup(&arena, "First string");
    char *str2 = sc_arena_strdup(&arena, "Second string");

    printf("%s\n", str1);
    printf("%s\n", str2);
    printf("Arena used: %zu bytes\n", sc_arena_used(&arena));

    printf("Arena will be destroyed automatically at scope exit\n");
    /* No need for sc_arena_destroy() - cleanup is automatic */
}

int main(void) {
    printf("Tutorial: RAII Resource Safety\n");
    printf("===============================\n");

    unsafe_approach();
    safe_approach();
    with_arena();

    printf("\nKey takeaway: Use AUTO_* macros to ensure resources are\n");
    printf("automatically cleaned up at scope exit.\n");

    return 0;
}
