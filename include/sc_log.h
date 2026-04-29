#ifndef SC_LOG_H
#define SC_LOG_H

/*
 * sc_log.h
 * Platform-independent logging for SaferCode.
 *
 * Targets:
 *   LOG_FILE      -- append to a named file
 *   LOG_CONSOLE   -- write to stdout
 *   LOG_NETWORK   -- send over TCP (param: "host:port")
 *
 * Levels (in ascending severity):
 *   LOG_DEBUG (0)  LOG_INFO (1)  LOG_WARN (2)  LOG_ERROR (3)
 *
 * Messages below the configured min_level are silently dropped.
 * Every message is timestamped automatically.
 *
 * API:
 *   int  log_init(log_target_t, log_level_t, const char *param)
 *   void log_message(log_level_t, const char *fmt, ...)
 *   int  log_change(log_target_t, log_level_t, const char *param)
 *   void log_close(void)
 *
 * Note: this header is intentionally self-contained (header-only).
 *       Include it in exactly one translation unit per binary to avoid
 *       duplicate static-state issues across TUs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <stdint.h>

#ifdef _WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>   /* inet_pton on Windows */
#  pragma comment(lib, "ws2_32.lib")
   typedef SOCKET socket_t;
#  define SC_CLOSE_SOCKET closesocket
#else
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>   /* inet_pton on POSIX */
#  include <unistd.h>
   typedef int socket_t;
#  define SC_CLOSE_SOCKET close
#  define INVALID_SOCKET  (-1)
#endif

typedef enum { LOG_DEBUG = 0, LOG_INFO = 1, LOG_WARN = 2, LOG_ERROR = 3 } log_level_t;
typedef enum { LOG_FILE, LOG_CONSOLE, LOG_NETWORK }                        log_target_t;

typedef struct {
    log_target_t target;
    log_level_t  min_level;
    FILE        *file;
    socket_t     sock;
} logger_t;

static logger_t g_sc_logger = { LOG_CONSOLE, LOG_INFO, NULL, INVALID_SOCKET };

static const char * const sc_level_strings[] = { "DEBUG", "INFO", "WARN", "ERROR" };

static void _sc_log_close_internal(void) {
    if (g_sc_logger.file) {
        fclose(g_sc_logger.file);
        g_sc_logger.file = NULL;
    }
    if (g_sc_logger.sock != INVALID_SOCKET) {
        SC_CLOSE_SOCKET(g_sc_logger.sock);
        g_sc_logger.sock = INVALID_SOCKET;
    }
}

static int log_init(log_target_t target, log_level_t min_level, const char *param) {
    _sc_log_close_internal();
    g_sc_logger.target    = target;
    g_sc_logger.min_level = min_level;

    if (target == LOG_FILE) {
        if (!param) return -1;
        g_sc_logger.file = fopen(param, "a");
        return g_sc_logger.file ? 0 : -1;
    }

    if (target == LOG_NETWORK) {
        if (!param) return -1;

#ifdef _WIN32
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return -1;
#endif
        /* Parse "host:port" */
        char host[256];
        strncpy(host, param, sizeof(host) - 1);
        host[sizeof(host) - 1] = '\0';

        char *colon = strchr(host, ':');
        if (!colon) return -1;
        *colon = '\0';
        int port = atoi(colon + 1);
        if (port <= 0 || port > 65535) return -1;

        g_sc_logger.sock = socket(AF_INET, SOCK_STREAM, 0);
        if (g_sc_logger.sock == INVALID_SOCKET) return -1;

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port   = htons((uint16_t)port);

        /* inet_pton returns 1 on success, 0 for invalid address, -1 on error. */
        int pton_result = inet_pton(AF_INET, host, &addr.sin_addr);
        if (pton_result != 1) {
            fprintf(stderr, "[SC_LOG] invalid host address: '%s'\n", host);
            SC_CLOSE_SOCKET(g_sc_logger.sock);
            g_sc_logger.sock = INVALID_SOCKET;
            return -1;
        }

        if (connect(g_sc_logger.sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            SC_CLOSE_SOCKET(g_sc_logger.sock);
            g_sc_logger.sock = INVALID_SOCKET;
            return -1;
        }
    }
    return 0;
}

static void log_message(log_level_t level, const char *format, ...) {
    if (level < g_sc_logger.min_level) return;

    char   buf[1024];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    int len = snprintf(buf, sizeof(buf),
                       "[%04d-%02d-%02d %02d:%02d:%02d] [%s] ",
                       t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                       t->tm_hour, t->tm_min, t->tm_sec,
                       sc_level_strings[level]);

    va_list args;
    va_start(args, format);
    vsnprintf(buf + len, sizeof(buf) - (size_t)len, format, args);
    va_end(args);

    strncat(buf, "\n", sizeof(buf) - strlen(buf) - 1);

    switch (g_sc_logger.target) {
    case LOG_CONSOLE:
        fputs(buf, stdout);
        fflush(stdout);
        break;
    case LOG_FILE:
        if (g_sc_logger.file) {
            fputs(buf, g_sc_logger.file);
            fflush(g_sc_logger.file);
        }
        break;
    case LOG_NETWORK:
        if (g_sc_logger.sock != INVALID_SOCKET)
            send(g_sc_logger.sock, buf, (int)strlen(buf), 0);
        break;
    }
}

static int log_change(log_target_t target, log_level_t min_level, const char *param) {
    return log_init(target, min_level, param);
}

static void log_close(void) {
    _sc_log_close_internal();
#ifdef _WIN32
    WSACleanup();
#endif
}

#endif /* SC_LOG_H */
