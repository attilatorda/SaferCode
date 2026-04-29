/*
 * Example: Logging
 * Build:  cc -I../../include -o example main.c
 *
 * Demonstrates the SaferCode logging system.
 */
#include <stdio.h>
#include "sc_log.h"

int main(void) {
    printf("=== Logging Example ===\n\n");

    /* Initialize logging to console with INFO level */
    printf("Initializing logger...\n");
    log_init(LOG_CONSOLE, LOG_INFO, NULL);

    /* Log messages at different levels */
    printf("\nLogging messages:\n");
    log_message(LOG_DEBUG, "This is a DEBUG message (should be filtered out)");
    log_message(LOG_INFO, "This is an INFO message");
    log_message(LOG_WARN, "This is a WARN message");
    log_message(LOG_ERROR, "This is an ERROR message");

    /* Change log level to DEBUG */
    printf("\nChanging to DEBUG level...\n");
    log_change(LOG_CONSOLE, LOG_DEBUG, NULL);

    printf("\nLogging messages again (DEBUG now visible):\n");
    log_message(LOG_DEBUG, "Now this DEBUG message is visible");
    log_message(LOG_INFO, "Another INFO message");

    /* Use printf-style formatting */
    printf("\nFormatted messages:\n");
    log_message(LOG_INFO, "Value: %d, String: %s", 42, "hello");
    log_message(LOG_WARN, "Array index %d out of bounds %d", 10, 5);

    /* Close the logger */
    printf("\nClosing logger...\n");
    log_close();

    printf("\nDone!\n");

    return 0;
}
