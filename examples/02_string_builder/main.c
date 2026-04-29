/*
 * Example: String Builder
 * Build:  cc -I../../include -o example main.c
 *
 * Demonstrates the string builder for efficient string construction.
 */
#include <stdio.h>
#include "sc_string_builder.h"

int main(void) {
    printf("=== String Builder Example ===\n\n");

    /* Create a string builder */
    sc_string_builder_t sb;
    sc_sb_init(&sb, 128);

    /* Append various pieces */
    sc_sb_append_cstr(&sb, "Hello");
    sc_sb_append_char(&sb, ' ');
    sc_sb_append_cstr(&sb, "from");
    sc_sb_append_char(&sb, ' ');
    sc_sb_append_cstr(&sb, "string builder");

    /* Finish and get the resulting string */
    SC_String *result = sc_sb_finish(&sb);
    printf("Result: \"%.*s\" (length=%u)\n",
           (int)result->length, result->data, result->length);

    /* Build another string with printf-style formatting */
    sc_sb_init(&sb, 128);
    sc_sb_append_printf(&sb, "Number: %d, Value: %d\n", 42, 100);
    SC_String *result2 = sc_sb_finish(&sb);
    printf("Formatted: %.*s", (int)result2->length, result2->data);

    /* Cleanup */
    sc_string_free(result);
    sc_string_free(result2);

    return 0;
}
