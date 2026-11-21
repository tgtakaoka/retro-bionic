/* ASCII art Mandelbrot set in floating point: samples/arith/fmandel.cpp
   for HI-TECH C, which needs a Z80; link with -LF. */
#include <stdio.h>

int main(void)
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
			putchar(i < 10 ? '0' + i : i < 16 ? 'A' + i - 10 : ' ');
		}
		putchar('\n');
	}
	return 0;
}
