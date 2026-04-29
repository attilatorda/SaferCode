# Tutorial: RAII Resource Safety

## Goal

Learn how to use RAII (Resource Acquisition Is Initialization) patterns in C
to ensure resources are always cleaned up, even in the face of early returns
or errors.

## Background

RESOURCE ACQUISITION IS INITIALIZATION (RAII):
- Acquire a resource when it is initialized
- Store it in a variable with automatic cleanup
- The resource is automatically released when the variable goes out of scope

In C, this is implemented via the `__attribute__((cleanup(...)))` macro,
available on GCC and Clang. SaferCode provides convenience macros:

- `AUTO_FREE_PTR` — automatically free a heap pointer on scope exit
- `AUTO_FILE` — automatically close a FILE*
- `AUTO_FD` — automatically close a file descriptor (POSIX)
- `AUTO_ARENA` — automatically destroy an arena

## Step 1: The problem - manual cleanup

```c
void unsafe_example(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return;  // leak: what if we return here?

    char *buffer = malloc(1024);
    if (!buffer) {
        fclose(f);  // must manually clean up
        return;
    }

    if (read_data(f, buffer) < 0) {
        free(buffer);  // cleanup at every exit point
        fclose(f);
        return;
    }

    process(buffer);

    free(buffer);
    fclose(f);
}
```

Problems:
- Cleanup code must be repeated at every return point
- Easy to forget one path
- Cleanup order matters

## Step 2: Solution - RAII with AUTO_* macros

```c
void safe_example(const char *filename) {
    AUTO_FILE FILE *f = fopen(filename, "r");
    if (!f) return;  // auto-cleanup (nothing to cleanup)

    AUTO_FREE_PTR char *buffer = malloc(1024);
    if (!buffer) return;  // auto-cleanup of f happens here

    if (read_data(f, buffer) < 0) {
        return;  // auto-cleanup of both buffer and f
    }

    process(buffer);
    // auto-cleanup happens at scope exit (no explicit free/fclose)
}
```

Benefits:
- Cleanup is implicit and guaranteed
- Works at every return point automatically
- Cleanup order is correct (LIFO)
- No code duplication

## Step 3: Prefer small scopes

RAII becomes most effective when variables live in the smallest scope possible.

```c
int parse_config(const char *path) {
    AUTO_FILE FILE *f = fopen(path, "r");
    if (!f) return -1;

    for (;;) {
        AUTO_FREE_PTR char *line = malloc(512);
        if (!line) return -2;
        if (!fgets(line, 512, f)) break;
        /* process one line */
    }
    return 0;
}
```

Here, `line` is reclaimed each loop iteration, while `f` is reclaimed at function
exit. This keeps ownership obvious and limits lifetime mistakes.

## Checkpoint

Before moving on, verify:

- You can remove duplicated cleanup blocks from early returns
- You understand cleanup order (reverse declaration order)
- You know when *not* to use RAII macros (non-GCC/Clang toolchains)

## Full solution

See `solution.c` in this directory.
