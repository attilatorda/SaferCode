#ifndef SC_REFTRACKER_H
#define SC_REFTRACKER_H

/*
 * sc_reftracker.h
 * Reference tracker for SaferCode — prevents use-after-free / dangling pointers.
 *
 * Ported from https://github.com/attilatorda/ReferenceTracker (MIT)
 *
 * ── Overview ───────────────────────────────────────────────────────────────
 * When an object is destroyed, every registered pointer to it is
 * automatically nullified.  Callers check for NULL before use.
 *
 *   sc_reftracker_t tracker;
 *   sc_reftracker_init(&tracker);
 *
 *   MyObj *obj = ...;
 *   MyObj *ref1 = obj, *ref2 = obj;
 *   sc_reftracker_add(&tracker, (void**)&ref1);
 *   sc_reftracker_add(&tracker, (void**)&ref2);
 *
 *   // On object destruction:
 *   sc_reftracker_clear(&tracker);   // sets ref1 = ref2 = NULL
 *   sc_reftracker_free(&tracker);
 *
 *
 * ── Two variants, compile-time selected ────────────────────────────────────
 *
 *   Thread-unsafe (default)                  Thread-safe
 *   ─────────────────────────────────────    ──────────────────────────────
 *   Zero-overhead, no locking               Mutex-protected (see below)
 *   For single-threaded code or             For objects shared across threads
 *   externally-synchronised access          (define SC_REF_THREAD_SAFE)
 *
 * ── Thread-safety ──────────────────────────────────────────────────────────
 *  Define SC_REF_THREAD_SAFE before including this header (or globally via
 *  -DSC_REF_THREAD_SAFE) to enable mutex protection on every operation.
 *  Uses CRITICAL_SECTION on Windows, pthread_mutex_t on POSIX.
 *
 * ── Notes ──────────────────────────────────────────────────────────────────
 *  - References must be explicitly registered (sc_reftracker_add) and
 *    unregistered (sc_reftracker_remove) when no longer needed.
 *  - This header is intentionally self-contained (header-only).
 *    Include it in exactly one translation unit per binary to avoid
 *    duplicate static-state issues if thread-safe statics are used.
 */

#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "sc_raii.h"   /* SC_ALLOC, SC_FREE — sentinel-aware */

#ifdef _WIN32
#  include <windows.h>
#else
#  include <pthread.h>
#endif

/* =========================================================================
 * Internal growth factor for the reference array
 * ========================================================================= */

#ifndef SC_REF_INITIAL_CAPACITY
#  define SC_REF_INITIAL_CAPACITY 8
#endif

/* =========================================================================
 * Mutex abstraction: thread-safe vs thread-unsafe variant
 * ========================================================================= */

#ifdef SC_REF_THREAD_SAFE

#  ifdef _WIN32
typedef CRITICAL_SECTION sc_ref_mutex_t;
static inline void _sc_ref_mutex_init(sc_ref_mutex_t *m)    { InitializeCriticalSection(m); }
static inline void _sc_ref_mutex_destroy(sc_ref_mutex_t *m) { DeleteCriticalSection(m);     }
static inline void _sc_ref_mutex_lock(sc_ref_mutex_t *m)    { EnterCriticalSection(m);      }
static inline void _sc_ref_mutex_unlock(sc_ref_mutex_t *m)  { LeaveCriticalSection(m);      }
#  else
typedef pthread_mutex_t sc_ref_mutex_t;
static inline void _sc_ref_mutex_init(sc_ref_mutex_t *m)    { pthread_mutex_init(m, NULL);  }
static inline void _sc_ref_mutex_destroy(sc_ref_mutex_t *m) { pthread_mutex_destroy(m);     }
static inline void _sc_ref_mutex_lock(sc_ref_mutex_t *m)    { pthread_mutex_lock(m);        }
static inline void _sc_ref_mutex_unlock(sc_ref_mutex_t *m)  { pthread_mutex_unlock(m);      }
#  endif

#else /* Thread-unsafe variant — zero overhead */

typedef int sc_ref_mutex_t;
static inline void _sc_ref_mutex_init(sc_ref_mutex_t *m)    { (void)m; }
static inline void _sc_ref_mutex_destroy(sc_ref_mutex_t *m) { (void)m; }
static inline void _sc_ref_mutex_lock(sc_ref_mutex_t *m)    { (void)m; }
static inline void _sc_ref_mutex_unlock(sc_ref_mutex_t *m)  { (void)m; }

#endif /* SC_REF_THREAD_SAFE */

/* =========================================================================
 * sc_reftracker_t
 * ========================================================================= */

typedef struct {
    void          ***refs;      /* SC_ALLOC'd array of (void **) */
    size_t           count;
    size_t           capacity;
    sc_ref_mutex_t   mutex;
} sc_reftracker_t;

/* =========================================================================
 * Lifecycle
 * ========================================================================= */

static inline int sc_reftracker_init(sc_reftracker_t *rt) {
    if (!rt) return -1;
    rt->refs     = (void***)SC_ALLOC(SC_REF_INITIAL_CAPACITY * sizeof(void**));
    rt->count    = 0;
    rt->capacity = rt->refs ? SC_REF_INITIAL_CAPACITY : 0;
    _sc_ref_mutex_init(&rt->mutex);
    return rt->refs ? 0 : -1;
}

/* Release internal storage.  Does NOT nullify remaining references —
 * call sc_reftracker_clear() first if that is desired. */
static inline void sc_reftracker_free(sc_reftracker_t *rt) {
    if (!rt) return;
    _sc_ref_mutex_destroy(&rt->mutex);
    SC_FREE(rt->refs);
    rt->refs     = NULL;
    rt->count    = 0;
    rt->capacity = 0;
}

/* =========================================================================
 * Reference registration
 * ========================================================================= */

static inline int sc_reftracker_add(sc_reftracker_t *rt, void **ref) {
    if (!rt || !ref) return -1;

    _sc_ref_mutex_lock(&rt->mutex);

    /* Check for duplicate */
    for (size_t i = 0; i < rt->count; i++) {
        if (rt->refs[i] == ref) {
            _sc_ref_mutex_unlock(&rt->mutex);
            return 0;
        }
    }

    /* Grow if needed — SC_ALLOC + memcpy + SC_FREE (no realloc in sentinel) */
    if (rt->count >= rt->capacity) {
        size_t   new_cap  = rt->capacity ? rt->capacity * 2 : SC_REF_INITIAL_CAPACITY;
        void ***new_refs = (void***)SC_ALLOC(new_cap * sizeof(void**));
        if (!new_refs) {
            _sc_ref_mutex_unlock(&rt->mutex);
            return -1;
        }
        memcpy(new_refs, rt->refs, rt->count * sizeof(void**));
        SC_FREE(rt->refs);
        rt->refs     = new_refs;
        rt->capacity = new_cap;
    }

    rt->refs[rt->count++] = ref;

    _sc_ref_mutex_unlock(&rt->mutex);
    return 0;
}

/* Unregister a pointer variable (e.g. when the reference goes out of scope
 * before the object is destroyed). */
static inline void sc_reftracker_remove(sc_reftracker_t *rt, void **ref) {
    if (!rt || !ref) return;

    _sc_ref_mutex_lock(&rt->mutex);

    for (size_t i = 0; i < rt->count; i++) {
        if (rt->refs[i] == ref) {
            rt->refs[i] = rt->refs[--rt->count];
            break;
        }
    }

    _sc_ref_mutex_unlock(&rt->mutex);
}

/* =========================================================================
 * Nullification — call this during object destruction
 * ========================================================================= */

static inline void sc_reftracker_clear(sc_reftracker_t *rt) {
    if (!rt) return;

    _sc_ref_mutex_lock(&rt->mutex);

    for (size_t i = 0; i < rt->count; i++) {
        if (rt->refs[i])
            *rt->refs[i] = NULL;
    }
    rt->count = 0;

    _sc_ref_mutex_unlock(&rt->mutex);
}

/* =========================================================================
 * Diagnostics
 * ========================================================================= */

static inline void sc_reftracker_dump(const sc_reftracker_t *rt) {
    if (!rt) return;
    fprintf(stderr, "[SC_REFTRACKER] %zu reference(s) registered (capacity %zu)\n",
            rt->count, rt->capacity);
}

/* =========================================================================
 * Version
 * ========================================================================= */
#define SC_REFTRACKER_VERSION_MAJOR 0
#define SC_REFTRACKER_VERSION_MINOR 9
#define SC_REFTRACKER_VERSION_PATCH 0

#endif /* SC_REFTRACKER_H */
