/* Example: CSV to JSON */
#include <stdio.h>
#include <string.h>
#include "sc_string_builder.h"

static void csv_line_to_json(const char *line) {
    char name[64] = {0};
    char age[32] = {0};
    char city[64] = {0};
    if (sscanf(line, "%63[^,],%31[^,],%63[^\n]", name, age, city) != 3) return;

    sc_string_builder_t sb;
    sc_sb_init(&sb, 128);
    sc_sb_append_cstr(&sb, "{\"name\":\"");
    sc_sb_append_cstr(&sb, name);
    sc_sb_append_cstr(&sb, "\",\"age\":");
    sc_sb_append_cstr(&sb, age);
    sc_sb_append_cstr(&sb, ",\"city\":\"");
    sc_sb_append_cstr(&sb, city);
    sc_sb_append_cstr(&sb, "\"}");

    SC_String *json = sc_sb_finish(&sb);
    printf("%.*s\n", (int)json->length, json->data);
    sc_string_free(json);
}

int main(void) {
    const char *rows[] = {
        "Alice,31,Budapest",
        "Bob,27,Vienna",
        "Carla,44,Prague"
    };
    for (size_t i = 0; i < sizeof(rows)/sizeof(rows[0]); i++) {
        csv_line_to_json(rows[i]);
    }
    return 0;
}
