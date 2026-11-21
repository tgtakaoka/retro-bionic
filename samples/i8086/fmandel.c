/* ASCII art Mandelbrot set: samples/arith/fmandel.bas in C. */
#include <stdio.h>

main()
{
    int x, y, i;
    double a, b, ca, cb, t;

    for (y = -12; y <= 12; y++) {
        for (x = -49; x <= 29; x++) {
            ca = x * 0.0458;
            cb = y * 0.08333;
            a = ca;
            b = cb;
            for (i = 0; i <= 15; i++) {
                t = a * a - b * b + ca;
                b = 2 * a * b + cb;
                a = t;
                if (a * a + b * b > 4)
                    break;
            }
            if (i > 15)
                putchar(' ');
            else
                putchar(i > 9 ? 'A' + i - 10 : '0' + i);
        }
        putchar('\n');
    }
}
