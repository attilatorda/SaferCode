#include <stdio.h>
#include <string.h>

static int parse_line(const char *line, char *key, size_t ksz, char *val, size_t vsz) {
    const char *eq = strchr(line, '=');
    if (!eq) return -1;
    size_t klen = (size_t)(eq - line);
    size_t vlen = strlen(eq + 1);
    if (klen == 0 || klen >= ksz || vlen == 0 || vlen >= vsz) return -1;
    memcpy(key, line, klen); key[klen] = '\0';
    memcpy(val, eq + 1, vlen + 1);
    return 0;
}

int main(void) {
    const char *cfg[] = {
        "host=localhost",
        "port=8080",
        "mode=debug"
    };
    for (size_t i = 0; i < sizeof(cfg)/sizeof(cfg[0]); i++) {
        char key[64], value[64];
        if (parse_line(cfg[i], key, sizeof(key), value, sizeof(value)) == 0) {
            printf("%s => %s\n", key, value);
        }
    }
    return 0;
}
