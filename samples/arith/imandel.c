/* ASCII art Mandelbrot set: mandelbrot.c in K&R C with 16-bit int
   arithmetic. */

main()
{
	int f, x, y, a, b, c, d, i, p, q, s, t;

	f = 50;
	for (y = -12; y <= 12; y++) {
		for (x = -49; x <= 29; x++) {
			c = x * 229 / 100;
			d = y * 416 / 100;
			a = c;
			b = d;
			for (i = 0; i < 16; i++) {
				q = b / f;
				s = b - q * f;
				t = (a * a - b * b) / f + c;
				b = 2 * (a * q + a * s / f) + d;
				a = t;
				p = a / f;
				q = b / f;
				if (p * p + q * q > 4)
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
