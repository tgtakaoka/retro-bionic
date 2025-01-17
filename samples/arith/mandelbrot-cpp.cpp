#include <stdio.h>
#include "add.hpp"
#include "div.hpp"
#include "mul.hpp"
#include "sub.hpp"

int main(int argc, char **argv) {
    const int debug = (argc >= 2);
    const int16_t f = 50;
    for (int8_t y = -12; y <= 12; ++y) {
        for (int8_t x = -49; x <= 29; ++x) {
            int16_t c = x;  // c = x * 229 / 100;
            mul16(c, 229);
            div16(c, 100);
            int16_t a = c;
            int16_t d = y;  // d = y * 416 / 100;
            mul16(d, 416);
            div16(d, 100);
            int16_t b = d;
            int8_t i = 0;
            if (debug)
                printf("Y=%d X=%d C=%d D=%d\n", y, x, c, d);
            do {
                int16_t q = b;  // q = b / f;
                div16(q, f);
                int16_t s = q;  // s = b - q * f;
                mul16(s, -f);
                add16(s, b);
                int16_t b2 = b;  // b2 = b * b;
                mul16(b2, b);
                int16_t tmp = a;  // (a * a - b * b) / f + c;
                mul16(tmp, a);
                sub16(tmp, b2);
                div16(tmp, f);
                add16(tmp, c);
                int16_t aq = a;  // aq = a * q;
                mul16(aq, q);
                b = a;  // b = 2 * (a * q + a * s / f) + d;
                mul16(b, s);
                div16(b, f);
                add16(b, aq);
                add16(b, b);
                add16(b, d);
                a = tmp;        // a = (a * a - b * b) / f + c;
                int16_t p = a;  // p = a / f;
                div16(p, f);
                tmp = p;  // tmp = p * p;
                mul16(tmp, p);
                q = b;  // q = b / f;
                div16(q, f);
                int16_t t = q;  // p * p + q * q;
                mul16(t, q);
                add16(t, tmp);
                if (debug)
                    printf(" I=%d A=%d B=%d P=%d Q=%d T=%d\n", i, a, b, p, q,
                            t);
                if (t > 4)
                    break;
                ++i;
            } while (i < 16);
            char out;
            if (i >= 16) {
                out = ' ';
            } else if (i < 10) {
                out = i + '0';
            } else {
                out = i - 10 + 'A';
            }
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

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
