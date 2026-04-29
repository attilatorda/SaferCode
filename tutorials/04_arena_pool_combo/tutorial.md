# Tutorial: Arena + Pool Combination

## Goal

Learn how to combine arena allocation with "pool" semantics for efficient
management of allocations with different lifetimes.

## Background

In real programs, you often have allocations with different lifetimes:

1. **Long-lived** — the main application state
2. **Request-scoped** — created per request, all freed at once
3. **Temporary** — working memory within a function

A POOL is a small arena that you reset frequently. For example:

```
Main Arena (long-lived)
  ├─ Application state
  └─ ┌─────────────┐
     │ Pool Arena  │ (reset after each request)
     └─────────────┘
```

## Step 1: Separate arenas for different lifetimes

```c
struct AppContext {
    sc_arena_t main;     /* Long-lived allocations */
    sc_arena_t pool;     /* Per-request, reset frequently */
};

void process_request(AppContext *ctx) {
    /* Allocate from main arena for permanent data */
    Item *item = sc_arena_alloc(&ctx->main, sizeof(Item));
    
    /* Allocate from pool for temporary work */
    char *temp_str = sc_arena_strdup(&ctx->pool, "temp");
    
    /* ... do work ... */
    
    /* Reset pool - all temporary allocations are gone */
    sc_arena_reset(&ctx->pool);
    
    /* item is still valid (allocated from main) */
}
```

Benefits:
- Permanent data stays in main
- Temporary data disappears cleanly
- No fragmentation issues
- Clear lifetime semantics

## Step 2: Multiple pools for different scopes

You can use multiple scratch arenas for different scopes:

```c
void process_request(AppContext *ctx) {
    sc_arena_t request_pool;
    sc_arena_init(&request_pool, 4096);
    
    /* Request-level allocations */
    for (int i = 0; i < num_items; i++) {
        process_item(ctx, &request_pool);
    }
    
    /* Reset request pool */
    sc_arena_reset(&request_pool);
}

void process_item(AppContext *ctx, sc_arena_t *req_pool) {
    sc_arena_t item_pool;
    sc_arena_init(&item_pool, 512);
    
    /* Item-level temporary work */
    char *temp = sc_arena_alloc(&item_pool, 256);
    /* ... work ... */
    
    sc_arena_reset(&item_pool);  /* Done with this item */
}
```

This gives you fine-grained control over memory lifetime.

## Step 3: Make lifetime boundaries explicit

When combining arenas, define exactly where each one resets:

- **Application arena**: reset only on shutdown/reload
- **Request arena**: reset after each request/job
- **Scratch/item arena**: reset inside tight loops

This removes ambiguity about ownership and prevents accidental retention of
temporary buffers in long-lived memory.

## Common mistakes to avoid

1. Returning pointers from a short-lived pool
2. Storing request-local pointers in global/app structures
3. Forgetting to reset a frequently reused pool

## Checkpoint

Before moving on, verify you can trace each pointer to the arena that owns it,
and identify exactly where that arena is reset or destroyed.

## Full solution

See `solution.c` in this directory.
