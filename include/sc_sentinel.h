#ifndef SC_SENTINEL_H
#define SC_SENTINEL_H

/*
 * sc_sentinel.h
 * Debug-only memory sentinel system for SaferCode.
 *
 * Enabled when:
 *      #define SC_DEBUG_SENTINEL
 *
 * Features:
 *   - Allocates blocks with "redzones" (sentinel bytes) before/after user memory.
 *   - Periodically scans all live allocations for corruption.
 *   - Thread-safe (Windows CriticalSection / POSIX mutex).
 *   - Works on Windows, Linux, macOS, BSD.
 *
 * Memory layout:
 *   [ SC_SENTINEL_SIZE bytes ][ user data ][ SC_SENTINEL_SIZE bytes ]
 *
 * Background thread:
 *   sc_sentinel_start() launches a low-frequency checker thread.
 *   sc_sentinel_stop()  shuts it down cleanly.
 *
 * Allocation API:
 *   void* sc_sentinel_alloc(size_t user_bytes)    -- allocate with guards
 *   void* sc_sentinel_realloc(void *ptr,          -- resize + re-guard
 *                             size_t new_bytes)
 *   void  sc_sentinel_free(void *ptr)             -- free + validate
 *
 * Note: this header is intentionally self-contained (header-only).
 *       Include it in exactly one translation unit per binary to avoid
 *       duplicate static-state issues across TUs.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

#ifdef _WIN32
   /* WIN32_LEAN_AND_MEAN prevents windows.h from pulling in the old
    * winsock.h, which conflicts with winsock2.h used by sc_log.h. */
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#else
#  include <pthread.h>
#  include <unistd.h>
#endif

/* ====================================================================
 * Configuration  (override before including this header if needed)
 * ==================================================================== */

#ifndef SC_SENTINEL_SIZE
#define SC_SENTINEL_SIZE 16              /* guard bytes on each side */
#endif

#ifndef SC_SENTINEL_PATTERN
#define SC_SENTINEL_PATTERN 0xAB        /* fill byte */
#endif

#ifndef SC_SENTINEL_CHECK_INTERVAL_MS
#define SC_SENTINEL_CHECK_INTERVAL_MS 500
#endif

/* ====================================================================
 * Internal block metadata (one node per live allocation)
 * ==================================================================== */

typedef struct sc_sentinel_block {
    struct sc_sentinel_block *next;
    size_t   user_size;
    uint8_t *user_ptr;   /* pointer returned to the caller */
    uint8_t *base_ptr;   /* actual malloc'd base (before the front guard) */
} sc_sentinel_block;

static sc_sentinel_block *sc_sentinel_head    = NULL;
static int                sc_sentinel_running = 0;

#ifdef _WIN32
static HANDLE            sc_sentinel_thread;
static CRITICAL_SECTION  sc_sentinel_lock;
#  define SC_LOCK()    EnterCriticalSection(&sc_sentinel_lock)
#  define SC_UNLOCK()  LeaveCriticalSection(&sc_sentinel_lock)
#else
static pthread_t         sc_sentinel_thread;
static pthread_mutex_t   sc_sentinel_lock = PTHREAD_MUTEX_INITIALIZER;
#  define SC_LOCK()    pthread_mutex_lock(&sc_sentinel_lock)
#  define SC_UNLOCK()  pthread_mutex_unlock(&sc_sentinel_lock)
#endif

/* ====================================================================
 * Sentinel checking
 * ==================================================================== */

static void sc_sentinel_check_one(const sc_sentinel_block *b) {
    for (size_t i = 0; i < SC_SENTINEL_SIZE; i++) {
        if (b->base_ptr[i] != (uint8_t)SC_SENTINEL_PATTERN) {
            fprintf(stderr, "[SC_SENTINEL] UNDERFLOW at block size %zu\n", b->user_size);
            break;
        }
    }
    uint8_t *after = b->user_ptr + b->user_size;
    for (size_t i = 0; i < SC_SENTINEL_SIZE; i++) {
        if (after[i] != (uint8_t)SC_SENTINEL_PATTERN) {
            fprintf(stderr, "[SC_SENTINEL] OVERFLOW at block size %zu\n", b->user_size);
            break;
        }
    }
}

static void sc_sentinel_check_all(void) {
    SC_LOCK();
    for (sc_sentinel_block *cur = sc_sentinel_head; cur; cur = cur->next)
        sc_sentinel_check_one(cur);
    SC_UNLOCK();
}

/* ====================================================================
 * Background checker thread
 * ==================================================================== */

#ifdef _WIN32
static DWORD WINAPI sc_sentinel_thread_func(LPVOID unused) {
    (void)unused;
    while (sc_sentinel_running) {
        sc_sentinel_check_all();
        Sleep(SC_SENTINEL_CHECK_INTERVAL_MS);
    }
    return 0;
}
#else
static void* sc_sentinel_thread_func(void *unused) {
    (void)unused;
    while (sc_sentinel_running) {
        sc_sentinel_check_all();
        usleep(SC_SENTINEL_CHECK_INTERVAL_MS * 1000);
    }
    return NULL;
}
#endif

/* ====================================================================
 * Lifecycle: start / stop the background checker
 * ==================================================================== */

static inline void sc_sentinel_start(void) {
    if (sc_sentinel_running) return;
    sc_sentinel_running = 1;
#ifdef _WIN32
    InitializeCriticalSection(&sc_sentinel_lock);
    sc_sentinel_thread = CreateThread(NULL, 0, sc_sentinel_thread_func, NULL, 0, NULL);
#else
    pthread_create(&sc_sentinel_thread, NULL, sc_sentinel_thread_func, NULL);
#endif
}

static inline void sc_sentinel_stop(void) {
    if (!sc_sentinel_running) return;
    sc_sentinel_running = 0;
#ifdef _WIN32
    WaitForSingleObject(sc_sentinel_thread, INFINITE);
    DeleteCriticalSection(&sc_sentinel_lock);
#else
    pthread_join(sc_sentinel_thread, NULL);
#endif
}

/* ====================================================================
 * Public allocation API
 *
 * In release builds (SC_DEBUG_SENTINEL not defined) these are thin
 * wrappers around malloc/realloc/free with zero overhead.
 * ==================================================================== */

static inline void* sc_sentinel_alloc(size_t user_bytes) {
#ifndef SC_DEBUG_SENTINEL
    return malloc(user_bytes);
#else
    size_t   total    = user_bytes + 2 * SC_SENTINEL_SIZE;
    uint8_t *raw      = (uint8_t*)malloc(total);
    if (!raw) return NULL;

    /* Write guard bytes */
    memset(raw,                                 SC_SENTINEL_PATTERN, SC_SENTINEL_SIZE);
    memset(raw + SC_SENTINEL_SIZE + user_bytes, SC_SENTINEL_PATTERN, SC_SENTINEL_SIZE);

    uint8_t *user_ptr = raw + SC_SENTINEL_SIZE;

    /* Register block in the linked list */
    SC_LOCK();
    sc_sentinel_block *blk = (sc_sentinel_block*)malloc(sizeof(sc_sentinel_block));
    if (blk) {
        blk->next      = sc_sentinel_head;
        blk->user_size = user_bytes;
        blk->user_ptr  = user_ptr;
        blk->base_ptr  = raw;
        sc_sentinel_head = blk;
    }
    SC_UNLOCK();

    return user_ptr;
#endif
}

/* ====================================================================
 * sc_sentinel_realloc
 *
 * Resize a sentinel-guarded allocation.  In debug mode:
 *   1. Validates the old block's guard bytes before touching it.
 *   2. Allocates a new guarded block of new_bytes.
 *   3. Copies min(old_size, new_bytes) bytes of user data.
 *   4. Removes the old block from the list and frees it.
 *   5. Returns the new user pointer.
 *
 * Passing ptr == NULL is equivalent to sc_sentinel_alloc(new_bytes).
 * Passing new_bytes == 0 frees ptr and returns NULL.
 *
 * In release mode this is a plain realloc().
 * ==================================================================== */
static inline void* sc_sentinel_realloc(void *ptr, size_t new_bytes) {
#ifndef SC_DEBUG_SENTINEL
    return realloc(ptr, new_bytes);
#else
    /* realloc(NULL, n) == malloc(n) */
    if (!ptr) return sc_sentinel_alloc(new_bytes);

    /* realloc(p, 0) == free(p), return NULL */
    if (new_bytes == 0) {
        /* sc_sentinel_free inline to avoid forward-declaration issues */
        SC_LOCK();
        sc_sentinel_block **cur = &sc_sentinel_head;
        while (*cur) {
            sc_sentinel_block *blk = *cur;
            if (blk->user_ptr == (uint8_t*)ptr) {
                sc_sentinel_check_one(blk);
                *cur = blk->next;
                free(blk->base_ptr);
                free(blk);
                break;
            }
            cur = &blk->next;
        }
        SC_UNLOCK();
        return NULL;
    }

    /* Find the old block */
    SC_LOCK();
    sc_sentinel_block *old_blk = NULL;
    for (sc_sentinel_block *b = sc_sentinel_head; b; b = b->next) {
        if (b->user_ptr == (uint8_t*)ptr) { old_blk = b; break; }
    }

    if (!old_blk) {
        SC_UNLOCK();
        fprintf(stderr, "[SC_SENTINEL] sc_sentinel_realloc: unknown pointer %p\n", ptr);
        return NULL;
    }

    /* Validate before touching */
    sc_sentinel_check_one(old_blk);
    size_t old_size = old_blk->user_size;
    SC_UNLOCK();

    /* Allocate new guarded block (lock released — sc_sentinel_alloc re-locks) */
    uint8_t *new_ptr = (uint8_t*)sc_sentinel_alloc(new_bytes);
    if (!new_ptr) return NULL;

    /* Copy user data */
    size_t copy_size = (old_size < new_bytes) ? old_size : new_bytes;
    memcpy(new_ptr, ptr, copy_size);

    /* Remove and free old block */
    SC_LOCK();
    sc_sentinel_block **cur = &sc_sentinel_head;
    while (*cur) {
        sc_sentinel_block *blk = *cur;
        if (blk->user_ptr == (uint8_t*)ptr) {
            *cur = blk->next;
            free(blk->base_ptr);
            free(blk);
            break;
        }
        cur = &blk->next;
    }
    SC_UNLOCK();

    return new_ptr;
#endif
}

static inline void sc_sentinel_free(void *ptr) {
#ifndef SC_DEBUG_SENTINEL
    free(ptr);
#else
    if (!ptr) return;

    SC_LOCK();
    sc_sentinel_block **cur = &sc_sentinel_head;
    while (*cur) {
        sc_sentinel_block *blk = *cur;
        if (blk->user_ptr == (uint8_t*)ptr) {
            sc_sentinel_check_one(blk);   /* final validation on free */
            *cur = blk->next;
            free(blk->base_ptr);
            free(blk);
            break;
        }
        cur = &blk->next;
    }
    SC_UNLOCK();
#endif
}

#endif /* SC_SENTINEL_H */
