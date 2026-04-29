/* Example: Log Formatter */
#include <stdio.h>
#include "sc_string_builder.h"

static void print_log_line(const char *level, const char *component, const char *msg) {
    sc_string_builder_t sb;
    sc_sb_init(&sb, 128);
    sc_sb_append_char(&sb, '[');
    sc_sb_append_cstr(&sb, level);
    sc_sb_append_cstr(&sb, "] ");
    sc_sb_append_cstr(&sb, component);
    sc_sb_append_cstr(&sb, ": ");
    sc_sb_append_cstr(&sb, msg);

    SC_String *line = sc_sb_finish(&sb);
    printf("%.*s\n", (int)line->length, line->data);
    sc_string_free(line);
}

int main(void) {
    print_log_line("INFO", "auth", "user login ok");
    print_log_line("WARN", "cache", "stale entry evicted");
    print_log_line("ERROR", "db", "query timeout");
    return 0;
}
