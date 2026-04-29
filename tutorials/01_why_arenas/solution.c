/*
 * Tutorial solution: Why Arenas?
 * Build:  cc -I../../include -o solution solution.c
 *
 * This solution shows how arenas simplify memory management.
 */
#include <stdio.h>
#include <string.h>
#include "sc_arena.h"

#define ITEM_COUNT 100

/* Traditional approach: many malloc/free calls */
void process_items_traditional(void) {
    printf("\n=== Traditional Approach (malloc/free) ===\n");

    int *items = malloc(ITEM_COUNT * sizeof(int));
    char **labels = malloc(ITEM_COUNT * sizeof(char*));

    for (int i = 0; i < ITEM_COUNT; i++) {
        items[i] = i * 2;
        labels[i] = malloc(32);
        snprintf(labels[i], 32, "Item %d", i);
    }

    printf("Allocated %d items with labels\n", ITEM_COUNT);
    printf("First: %s (value=%d)\n", labels[0], items[0]);
    printf("Last: %s (value=%d)\n", labels[ITEM_COUNT-1], items[ITEM_COUNT-1]);

    /* Cleanup: O(count) free calls */
    for (int i = 0; i < ITEM_COUNT; i++) {
        free(labels[i]);
    }
    free(labels);
    free(items);
    printf("Cleaned up with %d individual free() calls\n", ITEM_COUNT + 2);
}

/* Arena approach: one reset */
void process_items_arena(void) {
    printf("\n=== Arena Approach (single reset) ===\n");

    sc_arena_t arena;
    sc_arena_init(&arena, 4096);

    int *items = sc_arena_alloc(&arena, ITEM_COUNT * sizeof(int));
    char **labels = sc_arena_alloc(&arena, ITEM_COUNT * sizeof(char*));

    for (int i = 0; i < ITEM_COUNT; i++) {
        items[i] = i * 2;
        labels[i] = sc_arena_alloc(&arena, 32);
        snprintf(labels[i], 32, "Item %d", i);
    }

    printf("Allocated %d items from arena\n", ITEM_COUNT);
    printf("Arena used: %zu bytes\n", sc_arena_used(&arena));
    printf("First: %s (value=%d)\n", labels[0], items[0]);
    printf("Last: %s (value=%d)\n", labels[ITEM_COUNT-1], items[ITEM_COUNT-1]);

    /* Cleanup: O(1) reset */
    sc_arena_reset(&arena);
    printf("Cleaned up with single reset\n");
    sc_arena_destroy(&arena);
}

int main(void) {
    printf("Tutorial: Why Arenas?\n");
    printf("======================\n");

    process_items_traditional();
    process_items_arena();

    printf("\nKey takeaway: arenas provide O(1) cleanup for O(n) allocations.\n");

    return 0;
}
