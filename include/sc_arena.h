#ifndef SC_ARENA_H
#define SC_ARENA_H

/*
 * sc_arena.h
 * Linear (bump) arena allocator for SaferCode.
 *
 * Features:
 *   - Two modes: dynamic (grows via SC_ALLOC) or fixed (user‑supplied buffer).
 *   - Allocations are bump‑pointer with configurable alignment.
 *   - Reset the arena to reuse memory without freeing.
 *   - Integrates with sc_raii.h (AUTO_ARENA, SC_ALLOC/SC_FREE).
 *   - Thread‑unsafe by design – caller must synchronise if needed.
 *
 * Usage:
 *   #include "sc_arena.h"
 *
 *   // Dynamic arena (grows from heap)
 *   sc_arena_t arena;
 *   sc_arena_init(&arena, 4096);        // initial capacity 4 KB
 *
 *   void *p = sc_arena_alloc(&arena, 128);
 *   char *str = sc_arena_alloc_str(&arena, "hello");
 *
 *   sc_arena_reset(&arena);             // rewinds, memory can be reused
 *   sc_arena_destroy(&arena);           // frees heap buffer
 *
 *   // Fixed arena (user‑supplied buffer, no heap allocation)
 *   uint8_t buffer[2048];
 *   sc_arena_init_fixed(&arena, buffer, sizeof(buffer));
 *   // ... use arena, no need to destroy
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "sc_raii.h"    // provides SC_ALLOC, SC_FREE, AUTO_CLEANUP
#include "sc_panic.h"   // provides SC_PANIC
#include "sc_string.h"  // for SC_String

/* =========================================================================
 * Configuration
 * ========================================================================= */
#ifndef SC_ARENA_DEFAULT_ALIGNMENT
#  define SC_ARENA_DEFAULT_ALIGNMENT 16   /* sufficient for SSE, long double, etc. */
#endif

/* If defined, dynamic arenas never grow. Out-of-space becomes a hard fail.
   Useful for safety-critical code where pointer stability is required. */
#ifndef SC_ARENA_NO_GROW
#  define SC_ARENA_NO_GROW 0
#endif

/* =========================================================================
 * Types
 * ========================================================================= */
typedef enum {
    SC_ARENA_DYNAMIC,   /* buffer allocated via SC_ALLOC, can grow */
    SC_ARENA_FIXED      /* user‑supplied buffer, cannot grow */
} sc_arena_type_t;

typedef struct sc_arena {
    uint8_t      *buffer;        /* start of memory block */
    size_t        capacity;      /* total bytes in buffer */
    size_t        offset;        /* next free byte offset */
    sc_arena_type_t type;
    /* For dynamic arenas, we also store the originally allocated pointer
       (may be different from buffer if we later realloc). We'll keep it simple:
       buffer always points to current allocation, and we use SC_FREE on destroy.
       For dynamic, we own the buffer; for fixed, we don't. */
    int           owns_buffer;    /* 1 if we should SC_FREE(buffer) on destroy */
} sc_arena_t;

/* =========================================================================
 * Initialisation
 * ========================================================================= */

/* Initialise a dynamic arena with an initial capacity (bytes).
   Capacity will be rounded up to alignment. Returns 0 on success, -1 on OOM. */
static inline int sc_arena_init(sc_arena_t *arena, size_t initial_capacity) {
    if (!arena) return -1;
    if (initial_capacity == 0) initial_capacity = 64;
    /* Align capacity to default alignment */
    size_t align = SC_ARENA_DEFAULT_ALIGNMENT;
    initial_capacity = (initial_capacity + align - 1) & ~(align - 1);

    arena->buffer = (uint8_t*)SC_ALLOC(initial_capacity);
    if (!arena->buffer) return -1;
    arena->capacity = initial_capacity;
    arena->offset = 0;
    arena->type = SC_ARENA_DYNAMIC;
    arena->owns_buffer = 1;
    return 0;
}

/* Initialise a fixed arena from a user‑supplied buffer.
   The buffer must remain valid for the lifetime of the arena.
   No heap allocation occurs. */
static inline void sc_arena_init_fixed(sc_arena_t *arena, void *buffer, size_t size) {
    if (!arena || !buffer) return;
    arena->buffer = (uint8_t*)buffer;
    arena->capacity = size;
    arena->offset = 0;
    arena->type = SC_ARENA_FIXED;
    arena->owns_buffer = 0;
}

/* Destroy an arena – frees dynamic buffer if owned.
   After destroy, the arena must not be used unless re‑initialised. */
static inline void sc_arena_destroy(sc_arena_t *arena) {
    if (!arena) return;
    if (arena->owns_buffer && arena->buffer) {
        SC_FREE(arena->buffer);
    }
    arena->buffer = NULL;
    arena->capacity = 0;
    arena->offset = 0;
}

/* Reset the arena – rewinds the offset, allowing reuse of all memory.
   Does not free the underlying buffer. */
static inline void sc_arena_reset(sc_arena_t *arena) {
    if (arena) arena->offset = 0;
}

/* =========================================================================
 * Allocation
 * ========================================================================= */

/* Internal: align offset and check capacity.
   Returns aligned offset, or SIZE_MAX if not enough space.
   For dynamic arenas, we could grow here – we'll implement that later. */
static inline size_t _sc_arena_align_offset(sc_arena_t *arena, size_t alignment) {
    size_t off = arena->offset;
    size_t aligned = (off + alignment - 1) & ~(alignment - 1);
    return aligned;
}

/* Try to grow a dynamic arena when space is insufficient.
   Returns 0 on success, -1 on failure. */
static inline int _sc_arena_grow(sc_arena_t *arena, size_t min_capacity) {
    if (arena->type != SC_ARENA_DYNAMIC) return -1;  /* fixed cannot grow */

#if SC_ARENA_NO_GROW
    (void)min_capacity;
    SC_PANIC("arena growth disabled (SC_ARENA_NO_GROW=1); "
             "allocation would invalidate existing pointers");
#else

    size_t new_cap = arena->capacity;
    do {
        if (new_cap == 0) new_cap = 64;
        else new_cap *= 2;
    } while (new_cap < min_capacity);
    /* Align new_cap to alignment */
    size_t align = SC_ARENA_DEFAULT_ALIGNMENT;
    new_cap = (new_cap + align - 1) & ~(align - 1);

    uint8_t *new_buf = (uint8_t*)SC_ALLOC(new_cap);
    if (!new_buf) return -1;
    memcpy(new_buf, arena->buffer, arena->offset);
    SC_FREE(arena->buffer);
    arena->buffer = new_buf;
    arena->capacity = new_cap;
    return 0;
#endif
}

/* Allocate a block of uninitialised memory from the arena.
   Returns NULL if out of memory (or OOM on grow attempt). */
static inline void* sc_arena_alloc(sc_arena_t *arena, size_t size) {
    if (!arena || size == 0) return NULL;

    size_t align = SC_ARENA_DEFAULT_ALIGNMENT;
    size_t aligned_off = _sc_arena_align_offset(arena, align);
    if (aligned_off + size > arena->capacity) {
        /* Try to grow if dynamic */
        if (arena->type == SC_ARENA_DYNAMIC) {
            if (_sc_arena_grow(arena, aligned_off + size) != 0)
                return NULL;
            /* After grow, re‑compute aligned offset (offset unchanged, capacity increased) */
            aligned_off = _sc_arena_align_offset(arena, align);
            if (aligned_off + size > arena->capacity) return NULL; /* should not happen */
        } else {
            return NULL; /* fixed arena out of space */
        }
    }
    void *ptr = arena->buffer + aligned_off;
    arena->offset = aligned_off + size;
    return ptr;
}

/* Allocate zero‑initialised memory. */
static inline void* sc_arena_calloc(sc_arena_t *arena, size_t size) {
    void *p = sc_arena_alloc(arena, size);
    if (p) memset(p, 0, size);
    return p;
}

/* Duplicate a C string into the arena (including null terminator).
   Returns a pointer to the copy, or NULL on OOM. */
static inline char* sc_arena_strdup(sc_arena_t *arena, const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char *p = (char*)sc_arena_alloc(arena, len);
    if (p) memcpy(p, s, len);
    return p;
}

/* Duplicate a length‑prefixed SC_String into the arena (no null terminator). */
static inline SC_String* sc_arena_string_dup(sc_arena_t *arena, const SC_String *src) {
    if (!src) return NULL;
    SC_String *dst = (SC_String*)sc_arena_alloc(arena, sizeof(SC_String) + src->length);
    if (!dst) return NULL;
    dst->length = src->length;
    memcpy(dst->data, src->data, src->length);
    return dst;
}

/* =========================================================================
 * RAII cleanup macro (requires sc_raii.h)
 * ========================================================================= */
static inline void _sc_auto_arena(sc_arena_t *p) {
    if (p) sc_arena_destroy(p);
}
#define AUTO_ARENA AUTO_CLEANUP(_sc_auto_arena)

/* =========================================================================
 * Query functions
 * ========================================================================= */
static inline size_t sc_arena_used(const sc_arena_t *arena) {
    return arena ? arena->offset : 0;
}
static inline size_t sc_arena_remaining(const sc_arena_t *arena) {
    return arena ? arena->capacity - arena->offset : 0;
}
static inline size_t sc_arena_capacity(const sc_arena_t *arena) {
    return arena ? arena->capacity : 0;
}

/* =========================================================================
 * Version
 * ========================================================================= */
#define SC_ARENA_VERSION_MAJOR 0
#define SC_ARENA_VERSION_MINOR 9
#define SC_ARENA_VERSION_PATCH 0

#endif /* SC_ARENA_H */