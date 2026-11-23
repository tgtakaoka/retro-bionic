/* ASCII art Mandelbrot set: fmandel.cpp in K&R C with double
   arithmetic. */

main()
{
	int x, y, i;
	double ca, cb, a, b, t;

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
			if (i < 10)
				putchar('0' + i);
			else if (i < 16)
				putchar('A' + i - 10);
			else
				putchar(' ');
		}
		putchar('\n');
	}
}
