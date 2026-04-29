#ifndef SC_STRING_H
#define SC_STRING_H

/*
 * sc_string.h
 * Portable length-prefixed string type for SaferCode.
 *
 * Layout:
 *   struct { uint32_t length; char data[]; }
 *
 * Properties:
 *   - NOT null-terminated by default (safe to store in mmap regions).
 *   - Length is in bytes (UTF-8 compatible).
 *   - Suitable for compact, contiguous storage with fast memcpy.
 *
 * API:
 *   SC_String* sc_string_new(const char *src, uint32_t len)
 *   SC_String* sc_string_from_cstr(const char *cstr)
 *   SC_String* sc_string_clone(const SC_String *src)
 *   void       sc_string_free(SC_String *s)
 *   int        sc_string_compare(const SC_String *a, const SC_String *b)
 *   void       sc_string_to_cstr(const SC_String *s, char *out, size_t out_size)
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "sc_raii.h"   /* provides SC_ALLOC, SC_FREE */

typedef struct {
    uint32_t length;  /* byte count of data[] */
    char     data[];  /* flexible array — not null-terminated */
} SC_String;

/* Allocate a new SC_String and copy 'len' bytes from 'src'.
 * Uses SC_ALLOC so the allocation is sentinel-guarded in debug builds. */
static inline SC_String* sc_string_new(const char *src, uint32_t len) {
    SC_String *s = (SC_String*)SC_ALLOC(sizeof(SC_String) + len);
    if (!s) return NULL;
    s->length = len;
    if (len > 0 && src)
        memcpy(s->data, src, len);
    return s;
}

/* Create from a null-terminated C string (uses strlen). */
static inline SC_String* sc_string_from_cstr(const char *cstr) {
    if (!cstr) return sc_string_new(NULL, 0);
    return sc_string_new(cstr, (uint32_t)strlen(cstr));
}

/* Deep copy. */
static inline SC_String* sc_string_clone(const SC_String *src) {
    if (!src) return NULL;
    return sc_string_new(src->data, src->length);
}

/* Free a heap-allocated SC_String.
 * Uses SC_FREE — must be paired with sc_string_new / sc_string_from_cstr /
 * sc_string_clone.  Do NOT call on arena-allocated strings. */
static inline void sc_string_free(SC_String *s) {
    SC_FREE(s);
}

/* Lexical binary comparison.  Returns <0, 0, or >0. */
static inline int sc_string_compare(const SC_String *a, const SC_String *b) {
    if (!a || !b) return (a != NULL) - (b != NULL);
    uint32_t len = (a->length < b->length) ? a->length : b->length;
    int cmp = memcmp(a->data, b->data, len);
    if (cmp != 0) return cmp;
    return (int)a->length - (int)b->length;
}

/* Write a null-terminated copy into caller-supplied buffer. */
static inline void sc_string_to_cstr(const SC_String *s, char *out, size_t out_size) {
    if (!s || out_size == 0) return;
    size_t n = (s->length < out_size - 1) ? s->length : out_size - 1;
    memcpy(out, s->data, n);
    out[n] = '\0';
}

/* =========================================================================
 * Version
 * ========================================================================= */
#define SC_STRING_VERSION_MAJOR 0
#define SC_STRING_VERSION_MINOR 9
#define SC_STRING_VERSION_PATCH 0

#endif /* SC_STRING_H */
