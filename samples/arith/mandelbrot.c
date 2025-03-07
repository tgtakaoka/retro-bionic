#include <stdint.h>
#include <stdio.h>

#define check(v)                                                 \
    do {                                                         \
        if ((v) >= 0x800 || (v) < -0x800)                        \
            printf("%s:%d: %s=%d\n", __FILE__, __LINE__, #v, v); \
    } while (0)

int main(int argc, char **argv) {
    const int debug = (argc >= 2);
    const int16_t f = 50;
    for (int16_t y = -12; y <= 12; ++y) {
        for (int16_t x = -49; x <= 29; ++x) {
            int16_t c = x * 229 / 100;
            check(c);
            int16_t d = y * 416 / 100;
            check(d);
            int16_t a = c;
            int16_t b = d;
            int16_t i = 0;
            if (debug)
                printf("Y=%d X=%d C=%d D=%d\n", y, x, c, d);
            do {
                int16_t q = b / f;
                int16_t s = b - q * f;
                int16_t tmp = (a * a - b * b) / f + c;
                b = 2 * (a * q + a * s / f) + d;
                check(b);
                a = tmp;
                check(a);
                int16_t p = a / f;
                q = b / f;
                int16_t t = p * p + q * q;
                check(t);
                if (debug)
                    printf(" I=%d A=%d B=%d P=%d Q=%d T=%d\n", i, a, b, p, q,
                            t);
                if (t > 4)
                    break;
                ++i;
            } while (i < 16);
            char out = i < 10 ? i + '0' : (i < 16 ? i - 10 + 'A' : ' ');
            if (debug) {
                printf("@=%c\n", out);
            } else {
                putchar(out);
            }
        }
        if (!debug)
            printf("\n");
    }
    return 0;
}
