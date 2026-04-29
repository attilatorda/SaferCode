#include <stdio.h>
#include <string.h>

static int parse_request_line(const char *line,
                              char *method, size_t msz,
                              char *path, size_t psz,
                              char *version, size_t vsz) {
    return sscanf(line, "%15s %127s %15s", method, path, version) == 3 ? 0 : -1;
}

int main(void) {
    char method[16], path[128], version[16];
    const char *req = "GET /api/v1/users HTTP/1.1";
    if (parse_request_line(req, method, sizeof(method), path, sizeof(path), version, sizeof(version)) != 0) {
        return 1;
    }
    printf("method=%s\npath=%s\nversion=%s\n", method, path, version);
    return 0;
}
