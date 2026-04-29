#ifndef SC_PANIC_H
#define SC_PANIC_H

/*
 * sc_panic.h
 * Configurable fatal error handler for SaferCode.
 *
 * Provides a customizable panic mechanism to handle fatal errors that occur in
 * various SaferCode primitives (sentinel corruption, allocation failures, etc).
 * Users can install a custom handler via sc_set_panic_handler() to override
 * the default behavior of calling abort().
 *
 * Usage:
 *   #include "sc_panic.h"
 *
 *   void my_handler(const char *msg) {
 *       // Custom error handling: log, cleanup, exit gracefully, etc.
 *       fprintf(stderr, "PANIC: %s\n", msg);
 *       exit(EXIT_FAILURE);
 *   }
 *
 *   int main(void) {
 *       sc_set_panic_handler(my_handler);
 *       // ... rest of code
 *   }
 */

#include <stdlib.h>
#include <stdio.h>

/* ====================================================================
 * Panic behavior configuration
 * ==================================================================== */

#define SC_PANIC_MODE_PRINT_ONLY 0
#define SC_PANIC_MODE_HYBRID     1

#ifndef SC_PANIC_MODE
#  define SC_PANIC_MODE SC_PANIC_MODE_HYBRID
#endif

/* ====================================================================
 * Panic handler callback type
 * ==================================================================== */

/*
 * Type of a panic handler function.
 * The message parameter describes the error condition.
 * The handler should not return (typically calls exit or abort).
 */
typedef void (*sc_panic_handler_t)(const char *msg);
typedef void (*sc_panic_log_hook_t)(const char *msg);

static int _sc_panic_in_progress = 0;
static sc_panic_log_hook_t _sc_panic_log_hook = NULL;

/* ====================================================================
 * Default panic handler (implementation at end of file)
 * ==================================================================== */

static void _sc_default_panic(const char *msg) {
    const char *safe_msg = msg ? msg : "unknown error";

#if SC_PANIC_MODE == SC_PANIC_MODE_HYBRID
    /* Recursion guard: if panic triggers while handling panic, print only. */
    if (_sc_panic_in_progress) {
        fprintf(stderr, "[SC PANIC] %s\n", safe_msg);
        abort();
    }
    _sc_panic_in_progress = 1;

    if (_sc_panic_log_hook) {
        _sc_panic_log_hook(safe_msg);
    } else {
        fprintf(stderr, "[SC PANIC] %s\n", safe_msg);
    }

#elif SC_PANIC_MODE == SC_PANIC_MODE_PRINT_ONLY
    fprintf(stderr, "[SC PANIC] %s\n", safe_msg);
#else
#  error "Invalid SC_PANIC_MODE value"
#endif

    abort();
}

/* ====================================================================
 * Global panic handler state (static to this TU)
 * ==================================================================== */

static sc_panic_handler_t _sc_panic_handler = _sc_default_panic;

/* ====================================================================
 * Public API
 * ==================================================================== */

/*
 * Install a custom panic handler.
 * Call this before any operations that might panic.
 * Pass NULL to restore the default handler.
 */
static inline void sc_set_panic_handler(sc_panic_handler_t handler) {
    _sc_panic_handler = handler ? handler : _sc_default_panic;
}

/*
 * Get the current panic handler (for debugging or saving/restoring).
 */
static inline sc_panic_handler_t sc_get_panic_handler(void) {
    return _sc_panic_handler;
}

/*
 * Optional logging hook used by the default panic handler in HYBRID mode.
 * Pass NULL to disable and fall back to print output.
 */
static inline void sc_set_panic_log_hook(sc_panic_log_hook_t hook) {
    _sc_panic_log_hook = hook;
}

static inline sc_panic_log_hook_t sc_get_panic_log_hook(void) {
    return _sc_panic_log_hook;
}

/*
 * Invoke the panic handler with a message.
 * This is called internally by SaferCode when a fatal error occurs.
 * Users should generally not call this directly.
 */
static inline void sc_panic(const char *msg) {
    _sc_panic_handler(msg ? msg : "unknown error");
}

/* ====================================================================
 * SC_PANIC macro — convenient way to invoke panic from C code
 * ==================================================================== */

#define SC_PANIC(msg) sc_panic(msg)

#endif /* SC_PANIC_H */
