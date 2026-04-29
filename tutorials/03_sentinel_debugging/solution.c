/*
 * Tutorial solution: Debugging with Sentinels
 * Build:  cc -I../../include -DSC_DEBUG_SENTINEL -o solution solution.c
 *
 * Demonstrates the sentinel guard system for detecting memory corruption.
 */
#include <stdio.h>
#include <string.h>
#include "sc_raii.h"

void demonstrate_sentinel(void) {
    printf("\n=== Sentinel Guard Demonstration ===\n\n");

    printf("When compiled with -DSC_DEBUG_SENTINEL:\n");
    printf("- Each allocation is wrapped with guard bytes\n");
    printf("- Layout: [16 guard bytes][data][16 guard bytes]\n");
    printf("- Background thread scans for corruption\n\n");

    /* Allocate a small buffer */
    char *buffer = SAFE_NEW_ARRAY(char, 32);

    printf("Allocated 32-byte buffer at %p\n", (void*)buffer);
    printf("(Actually allocated: 32 + 16 + 16 = 64 bytes with guards)\n\n");

    /* Safe usage: write within bounds */
    strcpy(buffer, "Safe string");
    printf("Safe write: '%s'\n", buffer);

    /* To demonstrate overflow detection, we would uncomment:
       strcat(buffer, " and a really long string that exceeds bounds");
       (Don't actually do this - it will crash!)
       Instead, the sentinel system would detect the overflow.
    */

    SC_FREE(buffer);
    printf("\nBuffer freed safely\n");
}

void show_allocation_info(void) {
    printf("\n=== Allocation Pattern ===\n\n");

    printf("Without sentinel (-DSC_DEBUG_SENTINEL not set):\n");
    printf("SC_ALLOC(1000) --> malloc(1000)\n\n");

    printf("With sentinel (-DSC_DEBUG_SENTINEL set):\n");
    printf("SC_ALLOC(1000) --> sc_sentinel_alloc(1000)\n");
    printf("                   allocates 1032 bytes total\n");
    printf("                   wraps data with 16-byte guards\n");
    printf("                   tracks allocation in linked list\n");
    printf("                   returns pointer to user data\n\n");

    printf("Background thread every 500ms:\n");
    printf("1. Iterate all live allocations\n");
    printf("2. Check guard bytes are intact\n");
    printf("3. Report any corruption with file/line context\n");
}

int main(void) {
    printf("Tutorial: Debugging with Sentinels\n");
    printf("==================================\n");

    show_allocation_info();
    demonstrate_sentinel();

    printf("\nKey takeaway: Compile with -DSC_DEBUG_SENTINEL to catch\n");
    printf("buffer overflows and underflows close to when they happen.\n");

    return 0;
}
