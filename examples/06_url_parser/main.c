/*
 * Example: URL Parser
 * Build: cc -I../../include -o example main.c
 */
#include <stdio.h>
#include <string.h>
#include "sc_arena.h"

typedef struct {
    const char *scheme;
    const char *host;
    const char *path;
} url_parts_t;

static int parse_url(sc_arena_t *arena, const char *url, url_parts_t *out) {
    const char *scheme_end = strstr(url, "://");
    if (!scheme_end) return -1;
    const char *host_start = scheme_end + 3;
    const char *path_start = strchr(host_start, '/');
    if (!path_start) path_start = url + strlen(url);

    size_t scheme_len = (size_t)(scheme_end - url);
    size_t host_len = (size_t)(path_start - host_start);
    size_t path_len = strlen(path_start);

    char *scheme = (char*)sc_arena_alloc(arena, scheme_len + 1);
    char *host = (char*)sc_arena_alloc(arena, host_len + 1);
    char *path = (char*)sc_arena_alloc(arena, path_len + 1);
    if (!scheme || !host || !path) return -1;

    memcpy(scheme, url, scheme_len); scheme[scheme_len] = '\0';
    memcpy(host, host_start, host_len); host[host_len] = '\0';
    memcpy(path, path_start, path_len + 1);

    out->scheme = scheme;
    out->host = host;
    out->path = path;
    return 0;
}

int main(void) {
    sc_arena_t arena;
    url_parts_t p;
    const char *url = "https://example.com/docs/index.html";

    if (sc_arena_init(&arena, 256) != 0) return 1;
    if (parse_url(&arena, url, &p) != 0) {
        sc_arena_destroy(&arena);
        return 1;
    }

    printf("URL:    %s\n", url);
    printf("scheme: %s\n", p.scheme);
    printf("host:   %s\n", p.host);
    printf("path:   %s\n", p.path);

    sc_arena_destroy(&arena);
    return 0;
}
