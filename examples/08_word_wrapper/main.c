/* Example: Word Wrapper */
#include <stdio.h>
#include <string.h>
#include "sc_string_builder.h"

static void wrap_and_print(const char *text, size_t width) {
    sc_string_builder_t out;
    sc_sb_init(&out, 256);

    size_t line_len = 0;
    const char *p = text;
    while (*p) {
        while (*p == ' ') p++;
        if (!*p) break;

        const char *start = p;
        while (*p && *p != ' ') p++;
        size_t wlen = (size_t)(p - start);

        if (line_len != 0 && line_len + 1 + wlen > width) {
            sc_sb_append_char(&out, '\n');
            line_len = 0;
        } else if (line_len != 0) {
            sc_sb_append_char(&out, ' ');
            line_len++;
        }

        sc_sb_append(&out, start, wlen);
        line_len += wlen;
    }

    SC_String *wrapped = sc_sb_finish(&out);
    printf("%.*s\n", (int)wrapped->length, wrapped->data);
    sc_string_free(wrapped);
}

int main(void) {
    const char *text = "SaferCode provides lightweight C primitives for safer memory management and easier diagnostics.";
    wrap_and_print(text, 36);
    return 0;
}
