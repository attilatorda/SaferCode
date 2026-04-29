# Example: Hello Arena

## Build

```sh
cc -I../../include -o example main.c
./example
```

## What this shows

Basic arena allocation. This example demonstrates:

- Creating a dynamic arena with initial capacity
- Allocating strings from the arena
- Checking arena usage
- Resetting the arena to free all allocations at once
- Reusing arena memory after reset
- Proper cleanup with arena destroy

Arenas are useful for temporary allocations with a known lifetime.
Instead of freeing each object individually, you can reset the entire
arena and reuse the memory — O(1) cleanup instead of O(n).
