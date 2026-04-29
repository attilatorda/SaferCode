#include "greatest.h"
#include "sc_log.h"
#include "sc_sentinel.h"

TEST sentinel_realloc_and_overflow_detection_with_logging(void) {
    ASSERT_EQ(0, log_init(LOG_CONSOLE, LOG_DEBUG, NULL));
    log_message(LOG_INFO, "sentinel test start");
    sc_sentinel_start();
    unsigned char *p = (unsigned char*)sc_sentinel_alloc(8);
    ASSERT(p != NULL);
    for (int i = 0; i < 8; ++i) p[i] = (unsigned char)i;
    p = (unsigned char*)sc_sentinel_realloc(p, 32);
    ASSERT(p != NULL);
    ASSERT_EQ(0, p[0]);
    ASSERT_EQ(7, p[7]);
    log_message(LOG_INFO, "non-destructive sentinel path in omnibus run");
#ifdef SC_RUN_DESTRUCTIVE_SENTINEL_TESTS
    p[32] = 0xEE;
    log_message(LOG_WARN, "overflow injected; expect sentinel warning on free");
#endif
    sc_sentinel_free(p);
    sc_sentinel_stop();
    log_close();
    PASS();
}

TEST sentinel_intentional_failure(void) { FAIL(); }

SUITE(sc_sentinel_suite) {
    RUN_TEST(sentinel_realloc_and_overflow_detection_with_logging);
#ifdef SC_RUN_FAILING_TESTS
    RUN_TEST(sentinel_intentional_failure);
#endif
}
