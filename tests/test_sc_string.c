#include "greatest.h"
#include "sc_string.h"
#include <string.h>

TEST string_create_clone_compare_to_cstr(void) {
    SC_String *a = sc_string_from_cstr("alpha");
    SC_String *b = sc_string_clone(a);
    SC_String *c = sc_string_from_cstr("beta");
    ASSERT(a != NULL);
    ASSERT(b != NULL);
    ASSERT(c != NULL);
    ASSERT_EQ(0, sc_string_compare(a, b));
    ASSERT(sc_string_compare(a, c) < 0);
    char out[16] = {0};
    sc_string_to_cstr(c, out, sizeof(out));
    ASSERT(strcmp(out, "beta") == 0);
    sc_string_free(a);
    sc_string_free(b);
    sc_string_free(c);
    PASS();
}

SUITE(sc_string_suite) {
    RUN_TEST(string_create_clone_compare_to_cstr);
}
