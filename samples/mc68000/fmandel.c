/* ASCII art Mandelbrot set: samples/arith/fmandel.cpp in K&R C, for
   DRI's Alcyon C of CP/M-68K 1.2. */
#include <stdio.h>

main()
{
    int x, y, i;
    float ca, cb, a, b, t;

    for (y = -12; y <= 12; y++) {
        for (x = -49; x <= 29; x++) {
            ca = x * 0.0458;
            cb = y * 0.08333;
            a = ca;
            b = cb;
            for (i = 0; i < 16; i++) {
                t = a * a - b * b + ca;
                b = 2 * a * b + cb;
                a = t;
                if (a * a + b * b > 4)
                    break;
            }
            putchar(i < 10 ? i + '0' : (i < 16 ? i - 10 + 'A' : ' '));
        }
        putchar('\n');
    }
}
