#ifndef SC_STRING_BUILDER_H
#define SC_STRING_BUILDER_H

/*
 * sc_string_builder.h
 * Efficient mutable string builder for SaferCode.
 *
 * Integrates with:
 *   - sc_arena.h      – allocate buffer from an arena (always available now)
 *   - sc_raii.h       – SC_ALLOC / SC_FREE, AUTO_* macros
 *   - sc_string.h     – produces SC_String objects
 *   - sc_sentinel.h   – debug guard bytes (via SC_ALLOC)
 *
 * Usage:
 *   #include "sc_string_builder.h"
 *
 *   sc_string_builder_t sb;
 *   sc_sb_init(&sb, 0);
 *   sc_sb_append_cstr(&sb, "Hello, world");
 *   SC_String *s = sc_sb_finish(&sb);
 *   sc_string_free(s);
 *
 * Arena example:
 *   sc_arena_t arena;
 *   sc_arena_init(&arena, 4096);
 *   sc_sb_init_arena(&sb, &arena, 0);
 *   sc_sb_append_cstr(&sb, "Arena allocated");
 *   SC_String *s = sc_sb_finish(&sb);  // s lives in arena; no sc_string_free
 */

#include <stddef.h>
#include <string.h>
#include <stdarg.h>

/* Always include all dependencies. */
#include "sc_arena.h"    /* sc_arena_t, sc_arena_alloc */
#include "sc_string.h"   /* SC_String, sc_string_free  */
#include "sc_raii.h"     /* SC_ALLOC, SC_FREE, AUTO_CLEANUP */

/* =========================================================================
 * Types
 * ========================================================================= */

typedef struct {
    char       *buf;     /* mutable buffer (heap or arena) */
    size_t      len;     /* current length in bytes */
    size_t      cap;     /* allocated capacity */
    sc_arena_t *arena;   /* NULL when using heap allocations */
} sc_string_builder_t;

/* =========================================================================
 * Initialisation
 * ========================================================================= */

/* Initialise a builder using heap allocation (SC_ALLOC).
   If initial_cap == 0, a default of 64 bytes is used. */
static inline void sc_sb_init(sc_string_builder_t *sb, size_t initial_cap) {
    if (!sb) return;
    if (initial_cap == 0) initial_cap = 64;
    SC_String *blk = (SC_String*)SC_ALLOC(sizeof(SC_String) + initial_cap);
    sb->buf   = blk ? blk->data : NULL;
    sb->len   = 0;
    sb->cap   = sb->buf ? initial_cap : 0;
    sb->arena = NULL;
}

/* Initialise a builder that allocates its buffer from an arena.
   The arena must outlive the builder. */
static inline void sc_sb_init_arena(sc_string_builder_t *sb,
                                    sc_arena_t *arena, size_t initial_cap) {
    if (!sb || !arena) return;
    if (initial_cap == 0) initial_cap = 64;
    sb->buf   = (char*)sc_arena_alloc(arena, initial_cap);
    sb->len   = 0;
    sb->cap   = sb->buf ? initial_cap : 0;
    sb->arena = arena;
}

/* =========================================================================
 * Internal: ensure capacity for at least 'needed' additional bytes
 * ========================================================================= */
static inline int _sc_sb_grow(sc_string_builder_t *sb, size_t needed) {
    if (sb->len + needed <= sb->cap) return 0;

    size_t new_cap = sb->cap ? sb->cap : 64;
    while (new_cap < sb->len + needed)
        new_cap *= 2;

    char *new_buf;
    if (sb->arena) {
        /* Arena has no realloc: allocate new, copy, leave old in arena */
        new_buf = (char*)sc_arena_alloc(sb->arena, new_cap);
        if (!new_buf) return -1;
        memcpy(new_buf, sb->buf, sb->len);
    } else {
        /* Heap: SC_ALLOC new + memcpy + SC_FREE old
         * (SC_REALLOC would work too, but this keeps sentinel coverage clean) */
        SC_String *new_blk = (SC_String*)SC_ALLOC(sizeof(SC_String) + new_cap);
        new_buf = new_blk ? new_blk->data : NULL;
        if (!new_buf) return -1;
        memcpy(new_buf, sb->buf, sb->len);
        SC_FREE((uint8_t*)sb->buf - offsetof(SC_String, data));
    }
    sb->buf = new_buf;
    sb->cap = new_cap;
    return 0;
}

/* =========================================================================
 * Appending operations
 * ========================================================================= */

static inline int sc_sb_append(sc_string_builder_t *sb,
                                const char *data, size_t len) {
    if (!sb || !data) return -1;
    if (len == 0) return 0;
    if (_sc_sb_grow(sb, len) != 0) return -1;
    memcpy(sb->buf + sb->len, data, len);
    sb->len += len;
    return 0;
}

static inline int sc_sb_append_cstr(sc_string_builder_t *sb, const char *cstr) {
    if (!cstr) return -1;
    return sc_sb_append(sb, cstr, strlen(cstr));
}

static inline int sc_sb_append_char(sc_string_builder_t *sb, char ch) {
    return sc_sb_append(sb, &ch, 1);
}

static inline int sc_sb_append_string(sc_string_builder_t *sb,
                                       const SC_String *s) {
    if (!s) return -1;
    return sc_sb_append(sb, s->data, s->length);
}

/* printf-style append.  Returns the number of characters appended, or -1. */
static inline int sc_sb_append_printf(sc_string_builder_t *sb,
                                       const char *fmt, ...) {
    if (!sb || !fmt) return -1;
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (len < 0) return -1;

    if (_sc_sb_grow(sb, (size_t)len + 1) != 0) return -1;
    va_start(args, fmt);
    vsnprintf(sb->buf + sb->len, (size_t)len + 1, fmt, args);
    va_end(args);
    sb->len += (size_t)len;
    return len;
}

/* =========================================================================
 * Finalise — produce an SC_String and reset the builder
 * ========================================================================= */

/* Transfer the accumulated data into a new SC_String.
 * The builder is reset to empty after this call.
 *
 * Heap mode:   new SC_ALLOC + memcpy, then SC_FREE of the builder buffer.
 * Arena mode:  new sc_arena_alloc + memcpy (both header and data in arena).
 *
 * Returned pointer must be sc_string_free()'d unless arena-allocated. */
static inline SC_String* sc_sb_finish(sc_string_builder_t *sb) {
    if (!sb || !sb->buf) return NULL;

    SC_String *result;
    if (sb->arena) {
        result = (SC_String*)sc_arena_alloc(sb->arena,
                                            sizeof(SC_String) + sb->len);
        if (!result) return NULL;
        result->length = (uint32_t)sb->len;
        memcpy(result->data, sb->buf, sb->len);
        /* old buffer stays in arena — freed on arena reset/destroy */
    } else {
        result = (SC_String*)((uint8_t*)sb->buf - offsetof(SC_String, data));
        result->length = (uint32_t)sb->len;
        /* Zero-copy: buffer already stored in SC_String tail space */
    }

    sb->buf   = NULL;
    sb->len   = 0;
    sb->cap   = 0;
    sb->arena = NULL;
    return result;
}

/* =========================================================================
 * Reset and free
 * ========================================================================= */

/* Clear content, keep the allocated buffer for reuse. */
static inline void sc_sb_reset(sc_string_builder_t *sb) {
    if (sb) sb->len = 0;
}

/* Release the heap buffer (no-op for arena mode — arena handles it). */
static inline void sc_sb_free(sc_string_builder_t *sb) {
    if (!sb) return;
    if (sb->buf && !sb->arena)
        SC_FREE((uint8_t*)sb->buf - offsetof(SC_String, data));
    sb->buf   = NULL;
    sb->len   = sb->cap = 0;
    sb->arena = NULL;
}

/* =========================================================================
 * RAII cleanup macro
 * ========================================================================= */
static inline void _sc_auto_string_builder(sc_string_builder_t *p) {
    if (p) sc_sb_free(p);
}
#define AUTO_STRING_BUILDER AUTO_CLEANUP(_sc_auto_string_builder)

/* =========================================================================
 * Convenience: join an array of SC_String* with a separator
 * ========================================================================= */
static inline SC_String* sc_string_join(const SC_String **strings,
                                        size_t count,
                                        const char *sep) {
    if (count == 0) return sc_string_new(NULL, 0);
    sc_string_builder_t sb;
    sc_sb_init(&sb, 0);
    for (size_t i = 0; i < count; i++) {
        if (i > 0 && sep) sc_sb_append_cstr(&sb, sep);
        if (strings[i])   sc_sb_append_string(&sb, strings[i]);
    }
    return sc_sb_finish(&sb);
}

/* =========================================================================
 * Version
 * ========================================================================= */
#define SC_STRING_BUILDER_VERSION_MAJOR 0
#define SC_STRING_BUILDER_VERSION_MINOR 9
#define SC_STRING_BUILDER_VERSION_PATCH 0

#endif /* SC_STRING_BUILDER_H */
