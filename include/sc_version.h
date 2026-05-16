#ifndef SC_VERSION_H
#define SC_VERSION_H

/*
 * sc_version.h
 * Centralised version helpers for SaferCode headers.
 */

#include <stdint.h>

#define SC_VERSION_MAJOR 0
#define SC_VERSION_MINOR 9
#define SC_VERSION_PATCH 1

#define SC_MAKE_VERSION(maj, min, pat) \
    ((uint32_t)((maj) * 1000000u + (min) * 1000u + (pat)))

#define SC_VERSION SC_MAKE_VERSION(SC_VERSION_MAJOR, SC_VERSION_MINOR, SC_VERSION_PATCH)

#define SC_HEADER_VERSION(HEADER) \
    SC_MAKE_VERSION(HEADER##_VERSION_MAJOR, HEADER##_VERSION_MINOR, HEADER##_VERSION_PATCH)

#if __STDC_VERSION__ >= 201112L
#  define SC_REQUIRE_VERSION(HEADER, maj, min, pat) \
    _Static_assert(SC_HEADER_VERSION(HEADER) >= SC_MAKE_VERSION((maj), (min), (pat)), \
                   #HEADER " version too old")
#else
#  define SC_REQUIRE_VERSION(HEADER, maj, min, pat)
#endif

#endif /* SC_VERSION_H */
