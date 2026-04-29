#ifndef SC_MEMFILE_H
#define SC_MEMFILE_H

/*
 * sc_memfile.h
 * Cross-platform memory-file abstraction.
 *
 * Modes:
 *  1) SC_MEMFILE_MEMORY_VOLATILE  : load entire file to memory, discard on close
 *  2) SC_MEMFILE_MEMORY_WRITEBACK : load to memory, write full content on close
 *  3) SC_MEMFILE_MEMORY_WRITETHRU : load to memory, each write is mirrored to file
 *  4) SC_MEMFILE_CLASSIC          : plain FILE* I/O, no full memory mirror
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "sc_raii.h"

typedef enum sc_memfile_mode {
    SC_MEMFILE_MEMORY_VOLATILE  = 1,
    SC_MEMFILE_MEMORY_WRITEBACK = 2,
    SC_MEMFILE_MEMORY_WRITETHRU = 3,
    SC_MEMFILE_CLASSIC          = 4
} sc_memfile_mode_t;

typedef enum sc_memfile_open_result {
    SC_MEMFILE_OPEN_OK = 0,
    SC_MEMFILE_OPEN_INVALID_ARGUMENT = -1,
    SC_MEMFILE_OPEN_IO_ERROR = -2,
    SC_MEMFILE_OPEN_OUT_OF_MEMORY = -3,
    SC_MEMFILE_OPEN_INVALID_MODE = -4
} sc_memfile_open_result_t;

typedef struct sc_memfile {
    sc_memfile_mode_t requested_mode;
    sc_memfile_mode_t active_mode;

    char   *path;
    FILE   *fp;
    uint8_t *data;
    size_t  size;
    size_t  capacity;
    size_t  pos;
    int     dirty;
} sc_memfile_t;

static inline int _sc_memfile_is_memory_mode(sc_memfile_mode_t mode) {
    return mode == SC_MEMFILE_MEMORY_VOLATILE ||
           mode == SC_MEMFILE_MEMORY_WRITEBACK ||
           mode == SC_MEMFILE_MEMORY_WRITETHRU;
}

static inline char* _sc_memfile_strdup(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char *p = (char*)SC_ALLOC(n);
    if (!p) return NULL;
    memcpy(p, s, n);
    return p;
}

static inline int _sc_memfile_ensure_capacity(sc_memfile_t *mf, size_t needed) {
    if (needed <= mf->capacity) return 0;
    size_t cap = mf->capacity ? mf->capacity : 64;
    while (cap < needed) {
        if (cap > (SIZE_MAX / 2)) { cap = needed; break; }
        cap *= 2;
    }
    void *p = SC_REALLOC(mf->data, cap);
    if (!p) return -1;
    mf->data = (uint8_t*)p;
    mf->capacity = cap;
    return 0;
}

static inline int _sc_memfile_get_file_size(FILE *fp, size_t *out_size) {
    if (fseek(fp, 0, SEEK_END) != 0) return -1;
    long end = ftell(fp);
    if (end < 0) return -1;
    if (fseek(fp, 0, SEEK_SET) != 0) return -1;
    *out_size = (size_t)end;
    return 0;
}

static inline int _sc_memfile_write_full_path(const char *path, const uint8_t *buf, size_t len) {
    FILE *w = fopen(path, "wb");
    if (!w) return -1;
    if (len > 0 && fwrite(buf, 1, len, w) != len) { fclose(w); return -1; }
    if (fflush(w) != 0) { fclose(w); return -1; }
    fclose(w);
    return 0;
}

static inline sc_memfile_open_result_t _sc_memfile_open_classic(sc_memfile_t *mf, const char *path, const char *mode) {
    mf->fp = fopen(path, mode ? mode : "rb+");
    if (!mf->fp) {
        mf->fp = fopen(path, "wb+");
        if (!mf->fp) return SC_MEMFILE_OPEN_IO_ERROR;
    }
    mf->active_mode = SC_MEMFILE_CLASSIC;
    mf->size = 0;
    mf->capacity = 0;
    mf->pos = 0;
    mf->dirty = 0;
    return SC_MEMFILE_OPEN_OK;
}

static inline sc_memfile_open_result_t _sc_memfile_open_memory(sc_memfile_t *mf, const char *path, sc_memfile_mode_t mem_mode) {
    FILE *f = fopen(path, "rb");
    size_t size = 0;
    uint8_t *buffer = NULL;

    if (f) {
        if (_sc_memfile_get_file_size(f, &size) != 0) { fclose(f); return SC_MEMFILE_OPEN_IO_ERROR; }
        if (size > 0) {
            buffer = (uint8_t*)SC_ALLOC(size);
            if (!buffer) { fclose(f); return SC_MEMFILE_OPEN_OUT_OF_MEMORY; }
            if (fread(buffer, 1, size, f) != size) { fclose(f); SC_FREE(buffer); return SC_MEMFILE_OPEN_IO_ERROR; }
        }
        fclose(f);
    }

    if (!f) {
        /* File may not exist yet; treat as empty in-memory file. */
        size = 0;
        buffer = NULL;
    }

    mf->data = buffer;
    mf->size = size;
    mf->capacity = size;
    mf->pos = 0;
    mf->dirty = 0;
    mf->active_mode = mem_mode;

    if (mem_mode == SC_MEMFILE_MEMORY_WRITETHRU) {
        mf->fp = fopen(path, "rb+");
        if (!mf->fp) mf->fp = fopen(path, "wb+");
        if (!mf->fp) {
            SC_FREE(mf->data);
            mf->data = NULL;
            mf->size = 0;
            mf->capacity = 0;
            return SC_MEMFILE_OPEN_IO_ERROR;
        }
    }
    return SC_MEMFILE_OPEN_OK;
}

static inline sc_memfile_open_result_t sc_memfile_open(sc_memfile_t *mf, const char *path, const char *mode, sc_memfile_mode_t open_mode) {
    if (!mf || !path) return SC_MEMFILE_OPEN_INVALID_ARGUMENT;
    memset(mf, 0, sizeof(*mf));
    mf->requested_mode = open_mode;
    mf->path = _sc_memfile_strdup(path);
    if (!mf->path) return SC_MEMFILE_OPEN_OUT_OF_MEMORY;

    if (open_mode == SC_MEMFILE_CLASSIC) {
        return _sc_memfile_open_classic(mf, path, mode);
    }

    if (_sc_memfile_is_memory_mode(open_mode)) {
        return _sc_memfile_open_memory(mf, path, open_mode);
    }

    SC_FREE(mf->path);
    mf->path = NULL;
    return SC_MEMFILE_OPEN_INVALID_MODE;
}

static inline size_t sc_memfile_read(void *ptr, size_t size, size_t count, sc_memfile_t *mf) {
    if (!mf || !ptr || size == 0 || count == 0) return 0;
    if (mf->active_mode == SC_MEMFILE_CLASSIC) {
        return fread(ptr, size, count, mf->fp);
    }

    size_t total = size * count;
    if (mf->pos >= mf->size) return 0;
    size_t avail = mf->size - mf->pos;
    size_t to_copy = (total < avail) ? total : avail;
    memcpy(ptr, mf->data + mf->pos, to_copy);
    mf->pos += to_copy;
    return to_copy / size;
}

static inline size_t sc_memfile_write(const void *ptr, size_t size, size_t count, sc_memfile_t *mf) {
    if (!mf || !ptr || size == 0 || count == 0) return 0;
    if (mf->active_mode == SC_MEMFILE_CLASSIC) {
        return fwrite(ptr, size, count, mf->fp);
    }

    size_t total = size * count;
    size_t end_pos = mf->pos + total;
    if (_sc_memfile_ensure_capacity(mf, end_pos) != 0) return 0;

    if (mf->pos > mf->size) {
        memset(mf->data + mf->size, 0, mf->pos - mf->size);
    }
    memcpy(mf->data + mf->pos, ptr, total);
    mf->pos = end_pos;
    if (mf->size < mf->pos) mf->size = mf->pos;
    mf->dirty = 1;

    if (mf->active_mode == SC_MEMFILE_MEMORY_WRITETHRU) {
        if (!mf->fp) return 0;
        if (fseek(mf->fp, (long)(mf->pos - total), SEEK_SET) != 0) return 0;
        if (fwrite(ptr, 1, total, mf->fp) != total) return 0;
        fflush(mf->fp);
    }
    return count;
}

static inline int sc_memfile_seek(sc_memfile_t *mf, long offset, int origin) {
    if (!mf) return -1;
    if (mf->active_mode == SC_MEMFILE_CLASSIC) {
        return fseek(mf->fp, offset, origin);
    }

    size_t base = 0;
    if (origin == SEEK_SET) base = 0;
    else if (origin == SEEK_CUR) base = mf->pos;
    else if (origin == SEEK_END) base = mf->size;
    else return -1;

    long long np = (long long)base + (long long)offset;
    if (np < 0) return -1;
    mf->pos = (size_t)np;
    return 0;
}

static inline long sc_memfile_tell(sc_memfile_t *mf) {
    if (!mf) return -1;
    if (mf->active_mode == SC_MEMFILE_CLASSIC) return ftell(mf->fp);
    return (long)mf->pos;
}

static inline int sc_memfile_flush(sc_memfile_t *mf) {
    if (!mf) return -1;
    if (mf->active_mode == SC_MEMFILE_CLASSIC) return fflush(mf->fp);
    if (mf->active_mode == SC_MEMFILE_MEMORY_WRITETHRU && mf->fp) return fflush(mf->fp);
    return 0;
}

static inline int sc_memfile_close(sc_memfile_t *mf) {
    if (!mf) return -1;

    if (mf->active_mode == SC_MEMFILE_MEMORY_WRITEBACK && mf->dirty && mf->path) {
        if (_sc_memfile_write_full_path(mf->path, mf->data, mf->size) != 0) return -1;
    }

    if (mf->fp) {
        fclose(mf->fp);
        mf->fp = NULL;
    }

    if (mf->data) {
        SC_FREE(mf->data);
        mf->data = NULL;
    }
    if (mf->path) {
        SC_FREE(mf->path);
        mf->path = NULL;
    }

    mf->size = 0;
    mf->capacity = 0;
    mf->pos = 0;
    mf->dirty = 0;
    return 0;
}

static inline const uint8_t* sc_memfile_data(const sc_memfile_t *mf) {
    return mf ? mf->data : NULL;
}

static inline size_t sc_memfile_size(const sc_memfile_t *mf) {
    if (!mf) return 0;
    if (mf->active_mode == SC_MEMFILE_CLASSIC) {
        long cur = ftell(mf->fp);
        if (cur < 0) return 0;
        if (fseek(mf->fp, 0, SEEK_END) != 0) return 0;
        long end = ftell(mf->fp);
        (void)fseek(mf->fp, cur, SEEK_SET);
        return (end > 0) ? (size_t)end : 0;
    }
    return mf->size;
}

static inline sc_memfile_mode_t sc_memfile_mode(const sc_memfile_t *mf) {
    return mf ? mf->active_mode : SC_MEMFILE_CLASSIC;
}

#endif
