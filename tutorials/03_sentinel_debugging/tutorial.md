# Tutorial: Debugging with Sentinels

## Goal

Learn how the SaferCode sentinel system detects memory corruption by
wrapping allocations with guard regions ("redzones").

## Background

MEMORY CORRUPTION is hard to debug:
- A buffer overflow in function A might crash in function B
- The crash site tells you where the symptom is, not the cause
- By the time the error is detected, the corrupted memory is long gone

THE SENTINEL SOLUTION:
1. Wrap each allocation with guard bytes (redzones)
2. Periodically scan all live allocations for corruption
3. Catch the corruption close to where it happens
4. Report the exact allocation that was corrupted

## Step 1: Enable sentinel mode

When compiling with `-DSC_DEBUG_SENTINEL`, all `SC_ALLOC`/`SC_REALLOC`/`SC_FREE`
operations go through the sentinel allocator:

```bash
cc -I../../include -DSC_DEBUG_SENTINEL -o program program.c
```

The sentinel allocator adds guard bytes before and after user data:

```
[GUARD][user data][GUARD]
 16 bytes  actual   16 bytes  (configurable)
```

A background thread scans allocations and detects corruption.

## Step 2: Catching buffer overflow

```c
/* Buggy code: write beyond array bounds */
void buggy_function(void) {
    char *str = SAFE_NEW_ARRAY(char, 16);  /* 16 bytes only */
    
    strcpy(str, "This string is way too long");  /* overflow! */
    /* With sentinel enabled, the background thread will detect
       corruption of the guard bytes and abort with a message. */
    
    SC_FREE(str);
}
```

Without sentinel, the overflow might silently corrupt heap metadata.
With sentinel, it's caught and reported immediately.

## Step 3: Add explicit validation checkpoints

In debug sessions, trigger checks at points where corruption is likely to occur:

```c
void process_payload(const char *input) {
    char *buf = SC_ALLOC(128);
    if (!buf) return;

    /* copy/parse user data ... */

    /* Optional: force a sweep in strategic places (if exposed in your build) */
    /* sc_sentinel_check_all(); */

    SC_FREE(buf);
}
```

Even if you do not call explicit checks, the periodic background scanner catches
most errors early. Manual checkpoints are useful during tight fuzz/debug loops.

## Checkpoint

Before continuing:

- Build once with `-DSC_DEBUG_SENTINEL` and once without it
- Confirm debug build catches at least one intentional overflow
- Confirm release build runs with no sentinel overhead

## Full solution

See `solution.c` in this directory.
