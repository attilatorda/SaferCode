#include <stdio.h>
#include <ctype.h>

static int eval_sum_expr(const char *expr) {
    int total = 0, sign = 1, n = 0, has_num = 0;
    for (const char *p = expr; ; ++p) {
        if (isdigit((unsigned char)*p)) {
            n = n * 10 + (*p - '0');
            has_num = 1;
        } else {
            if (has_num) { total += sign * n; n = 0; has_num = 0; }
            if (*p == '+') sign = 1;
            else if (*p == '-') sign = -1;
            else if (*p == '\0') break;
        }
    }
    return total;
}

int main(void) {
    const char *expr = "10+25-7+3";
    printf("%s = %d\n", expr, eval_sum_expr(expr));
    return 0;
}
