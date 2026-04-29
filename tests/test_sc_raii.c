#include "greatest.h"
#include "sc_raii.h"
#include "sc_string_builder.h"

TEST raii_smoke(void) {
    int *nums = NEW_ZERO_ARRAY(int, 4);
    ASSERT(nums != NULL);
    nums[0] = 7;
    nums = RESIZE_ARRAY(int, nums, 8);
    ASSERT(nums != NULL);
    ASSERT_EQ(7, nums[0]);
    FREE_PTR(nums);
    ASSERT(nums == NULL);
#if SC_RAII_SUPPORTED
    AUTO_STRING_BUILDER sc_string_builder_t sb;
    sc_sb_init(&sb, 0);
    ASSERT_EQ(0, sc_sb_append_cstr(&sb, "raii"));
#endif
    PASS();
}

SUITE(sc_raii_suite) { RUN_TEST(raii_smoke); }
