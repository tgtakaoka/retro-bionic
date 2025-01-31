#include <stdio.h>

int main(int argc, char **argv) {
    const int debug = (argc >= 2);
    for (int y = -12; y <= 12; ++y) {
        for (int x = -49; x <= 29; ++x) {
            float ca = x * 0.0458;
            float cb = y * 0.08333;
            float a = ca;
            float b = cb;
            if (debug)
                printf("Y=%d X=%d A=%f B=%f\n", y, x, a, b);
            int i = 0;
            do {
                float t = a * a - b * b + ca;
                b = 2 * a * b + cb;
                a = t;
                t = a * a + b * b;
                if (debug)
                    printf(" I=%d A=%f B=%f T=%f\n", i, a, b, t);
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

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
