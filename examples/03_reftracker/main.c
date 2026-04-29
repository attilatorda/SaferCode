/*
 * Example: Reference Tracker
 * Build:  cc -I../../include -o example main.c
 *
 * Demonstrates the reference tracker for preventing use-after-free bugs.
 */
#include <stdio.h>
#include <stdlib.h>
#include "sc_reftracker.h"

/* A simple object with embedded reference tracker */
typedef struct {
    char name[64];
    int value;
    sc_reftracker_t tracker;
} MyObject;

int main(void) {
    printf("=== Reference Tracker Example ===\n\n");

    /* Create an object */
    MyObject *obj = malloc(sizeof(MyObject));
    snprintf(obj->name, sizeof(obj->name), "MyObject");
    obj->value = 42;
    sc_reftracker_init(&obj->tracker);

    printf("Created object: %s (value=%d)\n", obj->name, obj->value);

    /* Create multiple references to the object */
    MyObject *ref1 = obj;
    MyObject *ref2 = obj;
    MyObject *ref3 = obj;

    printf("Created 3 references\n\n");

    /* Register references with tracker */
    sc_reftracker_add(&obj->tracker, (void**)&ref1);
    sc_reftracker_add(&obj->tracker, (void**)&ref2);
    sc_reftracker_add(&obj->tracker, (void**)&ref3);

    printf("Before nullification:\n");
    printf("  obj is %p\n", (void*)obj);
    printf("  ref1 is %p\n", (void*)ref1);
    printf("  ref2 is %p\n", (void*)ref2);
    printf("  ref3 is %p\n\n", (void*)ref3);

    /* Nullify all references (simulates object destruction) */
    printf("Calling sc_reftracker_clear()...\n\n");
    sc_reftracker_clear(&obj->tracker);

    printf("After nullification:\n");
    printf("  ref1 is %p (should be NULL)\n", (void*)ref1);
    printf("  ref2 is %p (should be NULL)\n", (void*)ref2);
    printf("  ref3 is %p (should be NULL)\n\n", (void*)ref3);

    /* Clean up */
    sc_reftracker_free(&obj->tracker);
    free(obj);

    printf("This prevents use-after-free bugs!\n");

    return 0;
}
