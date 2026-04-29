#ifndef SC_RAII_H
#define SC_RAII_H

/*
 * sc_raii.h
 * RAII-style resource management and allocation helpers for SaferCode.
 *
 * Provides:
 *   - Unified allocation macros (SC_ALLOC / SC_REALLOC / SC_FREE) that swap
 *     in the sentinel allocator when SC_DEBUG_SENTINEL is defined.
 *   - Convenience type-safe allocation macros: NEW, NEW_ZERO, NEW_ARRAY, etc.
 *   - Checked variants (SAFE_NEW*) that abort on OOM with file:line context.
 *   - Scope-based cleanup via __attribute__((cleanup)) on GCC/Clang:
 *       AUTO_FREE_PTR, AUTO_FILE, AUTO_CHARBUF, AUTO_FD, AUTO_DIR
 *   - AUTODESTROY for typed struct pointers.
 *
 * Usage:
 *   #include "sc_raii.h"
 *
 *   void example(void) {
 *       AUTO_FREE_PTR char *buf = SC_ALLOC(256);   // freed at scope exit
 *       AUTO_FILE FILE *f = fopen("x.txt", "r");   // closed at scope exit
 *   }
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Pull in panic handler */
#include "sc_panic.h"

/* Pull in sentinel allocator when debug mode is active */
#ifdef SC_DEBUG_SENTINEL
#  include "sc_sentinel.h"
#endif

/* ======================================================================
 * Platform detection for cleanup attribute
 * ====================================================================== */

#if defined(__clang__) || defined(__GNUC__)
#  define SC_RAII_SUPPORTED 1
#else
#  define SC_RAII_SUPPORTED 0
#endif

/* ======================================================================
 * Core allocation interface
 * In debug mode delegates to sc_sentinel_alloc/realloc/free for guard-byte
 * checking.  In release mode uses plain malloc/realloc/calloc/free.
 * ====================================================================== */

#ifdef SC_DEBUG_SENTINEL
#  define SC_ALLOC(sz)          sc_sentinel_alloc(sz)
#  define SC_ALLOC_ZERO(sz)     (memset(sc_sentinel_alloc(sz), 0, (sz)))
#  define SC_REALLOC(ptr, sz)   sc_sentinel_realloc((ptr), (sz))
#  define SC_FREE(ptr)          sc_sentinel_free(ptr)
#else
#  define SC_ALLOC(sz)          malloc(sz)
#  define SC_ALLOC_ZERO(sz)     calloc(1, (sz))
#  define SC_REALLOC(ptr, sz)   realloc((ptr), (sz))
#  define SC_FREE(ptr)          free(ptr)
#endif

/* ======================================================================
 * Type-safe allocation helpers
 * ====================================================================== */

#define NEW(type)                   ((type*)SC_ALLOC(sizeof(type)))
#define NEW_ZERO(type)              ((type*)SC_ALLOC_ZERO(sizeof(type)))
#define NEW_ARRAY(type, count)      ((type*)SC_ALLOC(sizeof(type) * (count)))
#define NEW_ZERO_ARRAY(type, count) ((type*)SC_ALLOC_ZERO(sizeof(type) * (count)))

/* Resize a heap array in-place.  Returns new pointer; old pointer is invalid
 * after this call even on failure (matches realloc semantics). */
#define RESIZE_ARRAY(type, ptr, count) \
    ((type*)SC_REALLOC((ptr), sizeof(type) * (count)))

/* Nulls the pointer after freeing */
#define FREE_PTR(p) \
    do { if ((p)) { SC_FREE((p)); (p) = NULL; } } while (0)

/* ======================================================================
 * Checked allocation: abort with file:line on OOM
 * ====================================================================== */

static inline void* _sc_checked_alloc(size_t size, const char *file, int line) {
    void *p = SC_ALLOC(size);
    if (!p) {
        char msg[256];
        snprintf(msg, sizeof(msg),
                 "allocation failed at %s:%d (%zu bytes)",
                 file, line, size);
        SC_PANIC(msg);
    }
    return p;
}

static inline void* _sc_checked_alloc_zero(size_t nmemb, size_t size,
                                            const char *file, int line) {
    size_t total = nmemb * size;
    void  *p     = SC_ALLOC_ZERO(total);
    if (!p) {
        char msg[256];
        snprintf(msg, sizeof(msg),
                 "zero-allocation failed at %s:%d (%zu bytes)",
                 file, line, total);
        SC_PANIC(msg);
    }
    return p;
}

static inline void* _sc_checked_realloc(void *ptr, size_t size,
                                         const char *file, int line) {
    void *p = SC_REALLOC(ptr, size);
    if (!p && size > 0) {
        char msg[256];
        snprintf(msg, sizeof(msg),
                 "reallocation failed at %s:%d (%zu bytes)",
                 file, line, size);
        SC_PANIC(msg);
    }
    return p;
}

#define SAFE_NEW(type) \
    ((type*)_sc_checked_alloc(sizeof(type), __FILE__, __LINE__))

#define SAFE_NEW_ZERO(type) \
    ((type*)_sc_checked_alloc_zero(1, sizeof(type), __FILE__, __LINE__))

#define SAFE_NEW_ARRAY(type, count) \
    ((type*)_sc_checked_alloc(sizeof(type) * (count), __FILE__, __LINE__))

#define SAFE_NEW_ZERO_ARRAY(type, count) \
    ((type*)_sc_checked_alloc_zero((count), sizeof(type), __FILE__, __LINE__))

#define SAFE_RESIZE_ARRAY(type, ptr, count) \
    ((type*)_sc_checked_realloc((ptr), sizeof(type) * (count), __FILE__, __LINE__))

/* ======================================================================
 * Cleanup attribute wrapper
 * ====================================================================== */

#if SC_RAII_SUPPORTED
#  define AUTO_CLEANUP(func) __attribute__((cleanup(func)))
#else
#  define AUTO_CLEANUP(func)  /* no-op on MSVC */
#endif

#define DEFER(func) AUTO_CLEANUP(func)

/* ======================================================================
 * Automatic heap pointer cleanup
 * ====================================================================== */

static inline void _sc_auto_free_ptr(void *p) {
    void **pp = (void**)p;
    if (*pp) { SC_FREE(*pp); *pp = NULL; }
}
#define AUTO_FREE_PTR DEFER(_sc_auto_free_ptr)
#define AUTO_PTR      AUTO_FREE_PTR

/* ======================================================================
 * AUTODESTROY: typed struct pointer with custom destructor
 * ====================================================================== */

#define AUTODESTROY(TYPE, func) AUTO_CLEANUP(func) TYPE

/* ======================================================================
 * FILE* cleanup
 * ====================================================================== */

static inline void _sc_close_FILE(void *p) {
    FILE **fp = (FILE**)p;
    if (*fp) { fclose(*fp); *fp = NULL; }
}
#define AUTO_FILE DEFER(_sc_close_FILE)

/* ======================================================================
 * char* heap buffer cleanup
 * ====================================================================== */

static inline void _sc_free_charbuf(void *p) {
    char **pp = (char**)p;
    if (*pp) { SC_FREE(*pp); *pp = NULL; }
}
#define AUTO_CHARBUF DEFER(_sc_free_charbuf)

/* ======================================================================
 * Unix-only: file descriptor and DIR* cleanup
 * ====================================================================== */

#if defined(__unix__) || defined(__APPLE__) || defined(__linux__)

static inline void _sc_close_fd(void *p) {
    int *fd = (int*)p;
    if (*fd >= 0) { extern int close(int); close(*fd); *fd = -1; }
}
#define AUTO_FD DEFER(_sc_close_fd)

#include <dirent.h>
static inline void _sc_close_DIR(void *p) {
    DIR **dp = (DIR**)p;
    if (*dp) { closedir(*dp); *dp = NULL; }
}
#define AUTO_DIR DEFER(_sc_close_DIR)

#endif /* Unix */

/* ======================================================================
 * Optional debug logging
 * ====================================================================== */

#ifdef SC_RAII_DEBUG
#  define SC_RAII_LOG(fmt, ...) fprintf(stderr, "[SC_RAII] " fmt "\n", ##__VA_ARGS__)
#else
#  define SC_RAII_LOG(fmt, ...) ((void)0)
#endif

/* ======================================================================
 * Version
 * ====================================================================== */

#define SC_RAII_VERSION_MAJOR 0
#define SC_RAII_VERSION_MINOR 9
#define SC_RAII_VERSION_PATCH 0

#endif /* SC_RAII_H */
