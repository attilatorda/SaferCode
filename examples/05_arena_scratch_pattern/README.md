# Example: Arena Scratch Pattern

## Build

```sh
cc -I../../include -o example main.c
./example
```

## What this shows

The "scratch arena" pattern for temporary allocations. This example demonstrates:

- Creating an arena for temporary, scoped allocations
- Making many allocations without tracking each one
- Resetting the arena once (O(1) cleanup instead of O(n) free calls)
- Proper lifetime management within a function scope

This pattern is useful when you need to allocate several temporary objects
within a function or computation, and you want a simple way to clean them all up
at the end without manually freeing each one.
