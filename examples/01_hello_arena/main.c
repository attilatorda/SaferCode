/*
 * Example: Hello Arena
 * Build:  cc -I../../include -o example main.c
 * 
 * This example demonstrates basic arena allocation.
 * We create a dynamic arena, allocate some strings, and reset it.
 */
#include <stdio.h>
#include "sc_arena.h"

int main(void) {
    /* Create a dynamic arena with 256 bytes initial capacity */
    sc_arena_t arena;
    if (sc_arena_init(&arena, 256) != 0) {
        fprintf(stderr, "Failed to initialize arena\n");
        return 1;
    }

    printf("Arena initialized with %zu bytes capacity\n",
           sc_arena_capacity(&arena));

    /* Allocate some strings from the arena */
    char *hello = sc_arena_strdup(&arena, "Hello");
    char *world = sc_arena_strdup(&arena, "World");
    char *exclaim = sc_arena_strdup(&arena, "!");

    /* Print them */
    printf("%s %s%s\n", hello, world, exclaim);

    printf("Arena used: %zu / %zu bytes\n",
           sc_arena_used(&arena),
           sc_arena_capacity(&arena));

    /* Reset the arena - all allocations are freed and pointer is rewound */
    sc_arena_reset(&arena);
    printf("After reset: %zu / %zu bytes\n",
           sc_arena_used(&arena),
           sc_arena_capacity(&arena));

    /* Allocate again - reuses the same memory */
    char *reused = sc_arena_strdup(&arena, "Reused memory!");
    printf("%s\n", reused);

    /* Clean up */
    sc_arena_destroy(&arena);
    printf("Arena destroyed\n");

    return 0;
}
