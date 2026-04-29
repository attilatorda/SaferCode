/*
 * Example: Arena Scratch Pattern
 * Build:  cc -I../../include -o example main.c
 *
 * The "scratch arena" pattern: use a temporary arena for allocations
 * within a function scope, then reset it all at once.
 * No need to track individual allocations.
 */
#include <stdio.h>
#include "sc_arena.h"

/* Simulated work: build a result string from pieces */
void process_data(void) {
    /* Create a scratch arena for temporary allocations */
    sc_arena_t scratch;
    if (sc_arena_init(&scratch, 512) != 0) {
        fprintf(stderr, "Failed to create scratch arena\n");
        return;
    }

    /* Allocate temporary strings - don't track them individually */
    char *part1 = sc_arena_strdup(&scratch, "The quick brown ");
    char *part2 = sc_arena_strdup(&scratch, "fox jumps over ");
    char *part3 = sc_arena_strdup(&scratch, "the lazy dog");

    printf("Temporary allocation: %s%s%s\n", part1, part2, part3);
    printf("Scratch arena used: %zu bytes\n", sc_arena_used(&scratch));

    /* End of scope: reset the arena - all temporaries are gone */
    sc_arena_reset(&scratch);
    printf("After scratch reset: %zu bytes used\n", sc_arena_used(&scratch));

    /* Note: part1, part2, part3 pointers are now invalid */

    sc_arena_destroy(&scratch);
}

int main(void) {
    printf("=== Scratch Arena Pattern Example ===\n\n");

    printf("Running process_data()...\n");
    process_data();

    printf("\nDone. All scratch memory released.\n");

    return 0;
}
