#include "greatest.h"
#include "sc_memfile.h"

#include <string.h>

static const char *SC_MEMFILE_TEST_PATH = "sc_memfile_test_tmp.bin";

static void sc_memfile_write_raw_file(const char *path, const char *content) {
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fwrite(content, 1, strlen(content), f);
    fclose(f);
}

static void sc_memfile_read_raw_file(const char *path, char *out, size_t out_sz) {
    if (!out || out_sz == 0) return;
    out[0] = '\0';
    FILE *f = fopen(path, "rb");
    if (!f) return;
    size_t n = fread(out, 1, out_sz - 1, f);
    out[n] = '\0';
    fclose(f);
}

TEST memfile_volatile_discard_on_close(void) {
    sc_memfile_write_raw_file(SC_MEMFILE_TEST_PATH, "orig");

    sc_memfile_t mf;
    ASSERT_EQ((int)SC_MEMFILE_OPEN_OK, (int)sc_memfile_open(&mf, SC_MEMFILE_TEST_PATH, "rb+", SC_MEMFILE_MEMORY_VOLATILE));
    ASSERT_EQ((int)SC_MEMFILE_MEMORY_VOLATILE, (int)sc_memfile_mode(&mf));

    ASSERT_EQ(0, sc_memfile_seek(&mf, 0, SEEK_SET));
    ASSERT_EQ(4, (int)sc_memfile_write("NEWW", 1, 4, &mf));
    ASSERT_EQ(0, sc_memfile_close(&mf));

    char out[16];
    sc_memfile_read_raw_file(SC_MEMFILE_TEST_PATH, out, sizeof(out));
    ASSERT(strcmp(out, "orig") == 0);
    remove(SC_MEMFILE_TEST_PATH);
    PASS();
}

TEST memfile_writeback_persist_on_close(void) {
    sc_memfile_write_raw_file(SC_MEMFILE_TEST_PATH, "orig");

    sc_memfile_t mf;
    ASSERT_EQ((int)SC_MEMFILE_OPEN_OK, (int)sc_memfile_open(&mf, SC_MEMFILE_TEST_PATH, "rb+", SC_MEMFILE_MEMORY_WRITEBACK));
    ASSERT_EQ((int)SC_MEMFILE_MEMORY_WRITEBACK, (int)sc_memfile_mode(&mf));

    ASSERT_EQ(0, sc_memfile_seek(&mf, 0, SEEK_SET));
    ASSERT_EQ(4, (int)sc_memfile_write("done", 1, 4, &mf));
    ASSERT_EQ(0, sc_memfile_close(&mf));

    char out[16];
    sc_memfile_read_raw_file(SC_MEMFILE_TEST_PATH, out, sizeof(out));
    ASSERT(strcmp(out, "done") == 0);
    remove(SC_MEMFILE_TEST_PATH);
    PASS();
}

TEST memfile_writethru_immediate_persist(void) {
    sc_memfile_write_raw_file(SC_MEMFILE_TEST_PATH, "orig");

    sc_memfile_t mf;
    ASSERT_EQ((int)SC_MEMFILE_OPEN_OK, (int)sc_memfile_open(&mf, SC_MEMFILE_TEST_PATH, "rb+", SC_MEMFILE_MEMORY_WRITETHRU));
    ASSERT_EQ((int)SC_MEMFILE_MEMORY_WRITETHRU, (int)sc_memfile_mode(&mf));

    ASSERT_EQ(0, sc_memfile_seek(&mf, 0, SEEK_SET));
    ASSERT_EQ(4, (int)sc_memfile_write("sync", 1, 4, &mf));

    /* Should be persisted even before close in writethru mode. */
    char out_mid[16];
    sc_memfile_read_raw_file(SC_MEMFILE_TEST_PATH, out_mid, sizeof(out_mid));
    ASSERT(strcmp(out_mid, "sync") == 0);

    ASSERT_EQ(0, sc_memfile_close(&mf));
    remove(SC_MEMFILE_TEST_PATH);
    PASS();
}

TEST memfile_classic_roundtrip(void) {
    remove(SC_MEMFILE_TEST_PATH);
    sc_memfile_t mf;
    ASSERT_EQ((int)SC_MEMFILE_OPEN_OK, (int)sc_memfile_open(&mf, SC_MEMFILE_TEST_PATH, "wb+", SC_MEMFILE_CLASSIC));
    ASSERT_EQ((int)SC_MEMFILE_CLASSIC, (int)sc_memfile_mode(&mf));

    ASSERT_EQ(5, (int)sc_memfile_write("hello", 1, 5, &mf));
    ASSERT_EQ(0, sc_memfile_seek(&mf, 0, SEEK_SET));

    char out[16] = {0};
    ASSERT_EQ(5, (int)sc_memfile_read(out, 1, 5, &mf));
    ASSERT(strcmp(out, "hello") == 0);

    ASSERT_EQ(0, sc_memfile_close(&mf));
    remove(SC_MEMFILE_TEST_PATH);
    PASS();
}

TEST memfile_invalid_mode_returns_error_and_caller_decides(void) {
    sc_memfile_t mf;
    ASSERT_EQ((int)SC_MEMFILE_OPEN_INVALID_MODE,
              (int)sc_memfile_open(&mf, SC_MEMFILE_TEST_PATH, "rb+", (sc_memfile_mode_t)5));
    PASS();
}

SUITE(sc_memfile_suite) {
    RUN_TEST(memfile_volatile_discard_on_close);
    RUN_TEST(memfile_writeback_persist_on_close);
    RUN_TEST(memfile_writethru_immediate_persist);
    RUN_TEST(memfile_classic_roundtrip);
    RUN_TEST(memfile_invalid_mode_returns_error_and_caller_decides);
}
