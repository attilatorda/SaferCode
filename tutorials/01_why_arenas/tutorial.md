# Tutorial: Why Arenas?

## Goal

Understand why arena allocators are useful for performance and simplicity.
Learn how arenas simplify memory management compared to individual malloc/free.

## Background

Memory allocation is expensive. Each `malloc()` call:
1. Acquires a lock on the heap
2. Searches for a free block
3. Updates metadata
4. Updates bookkeeping (often slow)

Each `free()` call:
1. Acquires a lock
2. Merges with adjacent blocks
3. Updates the free list

When you have many allocations with a common lifetime, doing this 
O(n) times is wasteful. **Arenas** let you allocate O(n) objects but 
free them all in O(1).

## Step 1: Traditional approach (many malloc/free calls)

```c
void process_items(int count) {
    int *items = malloc(count * sizeof(int));
    if (!items) return;

    char **labels = malloc(count * sizeof(char*));
    if (!labels) { free(items); return; }

    for (int i = 0; i < count; i++) {
        items[i] = i * 2;
        labels[i] = malloc(32);
        if (!labels[i]) { /* cleanup needed */ }
        snprintf(labels[i], 32, "Item %d", i);
    }

    // ... do work ...

    // Cleanup: O(count) free calls
    for (int i = 0; i < count; i++) {
        free(labels[i]);
    }
    free(labels);
    free(items);
}
```

Problems:
- Error handling becomes complex
- O(count) cleanup calls
- Many lock acquisitions
- Fragmentation risk

## Step 2: Arena approach (one reset)

```c
void process_items_arena(int count) {
    sc_arena_t arena;
    sc_arena_init(&arena, 4096);  // allocate once

    // Allocate everything from the arena
    int *items = sc_arena_alloc(&arena, count * sizeof(int));
    char **labels = sc_arena_alloc(&arena, count * sizeof(char*));

    for (int i = 0; i < count; i++) {
        items[i] = i * 2;
        labels[i] = sc_arena_alloc(&arena, 32);  // no error handling needed
        snprintf(labels[i], 32, "Item %d", i);
    }

    // ... do work ...

    // Cleanup: O(1) reset
    sc_arena_reset(&arena);
    sc_arena_destroy(&arena);
}
```

Benefits:
- Simple error handling (arena will grow if needed)
- O(1) cleanup
- One or two lock acquisitions total
- Greatly reduced fragmentation pressure

## Step 3: Compare both styles in practice

Try implementing the same small workload both ways:

1. Allocate `N` labels and values with `malloc/free`
2. Allocate the same set from an arena
3. Measure runtime for multiple repetitions

Even for medium `N`, the arena version will usually be easier to reason about,
and often faster because cleanup is one reset instead of `N` individual frees.

## Checkpoint

Before continuing, verify you can answer:

- When is an arena a bad fit? (mixed/random object lifetimes)
- Why does reset make cleanup O(1)?
- What ownership rule keeps arena code safe?

If those are clear, open `solution.c` and compare your implementation.

## Full solution

See `solution.c` in this directory.
