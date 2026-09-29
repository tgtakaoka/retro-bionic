# CPU speed by an assembly-language Mandelbrot

Every architecture's samples run the same Mandelbrot algorithm, written in
that chip's own assembly: [mandelbrot.c](../samples/arith/mandelbrot.c) in
16-bit integers, and on chips with floating-point instructions,
[fmandel.cpp](../samples/arith/fmandel.cpp) as well. Both draw 25 rows of 79
pixels, each printed as its iteration count, `0`-`9` and `A`-`F`, or a space
after 16. Each character goes into a queue that the serial port's
transmit interrupt drains, so sending overlaps the next pixel's work.
The tables time them on real chips, from recorded casts, with
[mandelbrot-timing.py](../scripts/mandelbrot-timing.py).

- **frame** is the seconds for a full frame, measured when the cast has
  one. Otherwise it is estimated: rows differ in work, so seconds =
  a × iterations + b is fitted to the timed rows and summed over the
  golden frame.
- **avg row** is the average seconds per row over the rows a cast timed.
- **std dev** is the standard deviation of those rows' seconds, which is
  mostly the picture's, not the chip's.
- **Cast** is an [asciinema](https://asciinema.org/) recording of the
  debugger's terminal while the chip draws, with its timing;
  `asciinema play` replays it at the speed it ran.
- **Listing** is the assembly source the chip ran.

## Integer, mandelbrot

| CPU      | frame | avg row | std dev | Note | Cast                                                   | Listing                                                       |
| -------- | ----: | ------: | ------: | ---- | ------------------------------------------------------ | ------------------------------------------------------------- |
| CDP1804A | 198.3 |   8.099 |   2.639 |      | [mandelbrot.cast](../samples/cdp1804a/mandelbrot.cast) | [cdp1804a/mandelbrot.lst](../samples/cdp1804a/mandelbrot.lst) |
| CDP1802  | 247.8 |  10.125 |   3.329 |      | [mandelbrot.cast](../samples/cdp1802/mandelbrot.cast)  | [cdp1802/mandelbrot.lst](../samples/cdp1802/mandelbrot.lst)   |
