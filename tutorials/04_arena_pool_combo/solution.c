/*
 * Tutorial solution: Arena + Pool Combination
 * Build:  cc -I../../include -o solution solution.c
 *
 * Demonstrates combining permanent and temporary arenas.
 */
#include <stdio.h>
#include <string.h>
#include "sc_arena.h"

/* Application context with permanent and pool arenas */
typedef struct {
    sc_arena_t main;     /* Long-lived allocations */
    sc_arena_t pool;     /* Per-request temporary allocations */
} AppContext;

/* Simulated item in the application */
typedef struct {
    char name[64];
    int value;
} Item;

/* Process a request using both arenas */
void process_request(AppContext *ctx, int request_id) {
    printf("\nProcessing request %d\n", request_id);
    printf("  Main arena before: %zu bytes\n", sc_arena_used(&ctx->main));
    printf("  Pool arena before: %zu bytes\n", sc_arena_used(&ctx->pool));

    /* Allocate permanent item from main arena */
    Item *item = sc_arena_alloc(&ctx->main, sizeof(Item));
    snprintf(item->name, sizeof(item->name), "Item-%d", request_id);
    item->value = request_id * 100;

    /* Allocate temporary working strings from pool */
    char *temp1 = sc_arena_strdup(&ctx->pool, "Temporary string 1");
    char *temp2 = sc_arena_strdup(&ctx->pool, "Temporary string 2");

    printf("  Allocated item: %s (value=%d)\n", item->name, item->value);
    printf("  Created temporary strings: '%s', '%s'\n", temp1, temp2);

    printf("  Main arena after: %zu bytes\n", sc_arena_used(&ctx->main));
    printf("  Pool arena after: %zu bytes\n", sc_arena_used(&ctx->pool));

    /* Reset pool - temporary strings are freed, item is safe */
    sc_arena_reset(&ctx->pool);
    printf("  After pool reset: %zu bytes\n", sc_arena_used(&ctx->pool));
    printf("  Item still accessible: %s\n", item->name);
}

int main(void) {
    printf("Tutorial: Arena + Pool Combination\n");
    printf("==================================\n");

    /* Create application context */
    AppContext ctx;
    sc_arena_init(&ctx.main, 2048);
    sc_arena_init(&ctx.pool, 1024);

    printf("\nApplication started\n");
    printf("Main arena: %zu bytes capacity\n", sc_arena_capacity(&ctx.main));
    printf("Pool arena: %zu bytes capacity\n", sc_arena_capacity(&ctx.pool));

    /* Simulate multiple requests */
    process_request(&ctx, 1);
    process_request(&ctx, 2);
    process_request(&ctx, 3);

    printf("\n\nFinal state:\n");
    printf("Main arena used: %zu bytes (contains all permanent items)\n",
           sc_arena_used(&ctx.main));
    printf("Pool arena used: %zu bytes (temporary allocations cleaned up)\n",
           sc_arena_used(&ctx.pool));

    /* Cleanup */
    sc_arena_destroy(&ctx.main);
    sc_arena_destroy(&ctx.pool);

    printf("\nKey takeaway: Use separate arenas for different lifetime scopes.\n");
    printf("- Main arena for permanent data\n");
    printf("- Pool arenas for request/scope-scoped temporaries\n");
    printf("- Reset pools between requests for O(1) cleanup\n");

    return 0;
}
