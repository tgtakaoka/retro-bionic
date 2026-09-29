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
| P8095BH  |   3.6 |   0.145 |   0.038 |      | [mandelbrot.cast](../samples/i8096/mandelbrot.cast)    | [i8096/mandelbrot.lst](../samples/i8096/mandelbrot.lst)       |
| P8051    |  35.0 |   1.428 |   0.442 |      | [mandelbrot.cast](../samples/i8051/mandelbrot.cast)    | [i8051/mandelbrot.lst](../samples/i8051/mandelbrot.lst)       |
| INS8070  |  56.6 |   2.312 |   0.744 |      | [mandelbrot.cast](../samples/ins8070/mandelbrot.cast)  | [ins8070/mandelbrot.lst](../samples/ins8070/mandelbrot.lst)   |
| P8085    |  77.7 |   3.172 |   1.040 |      | [mandelbrot.cast](../samples/i8085/mandelbrot.cast)    | [i8085/mandelbrot.lst](../samples/i8085/mandelbrot.lst)       |
| P8080    | 132.0 |   5.390 |   1.763 |      | [mandelbrot.cast](../samples/i8080/mandelbrot.cast)    | [i8080/mandelbrot.lst](../samples/i8080/mandelbrot.lst)       |
| P8039    | 172.0 |   7.028 |   2.333 |      | [mandelbrot.cast](../samples/i8048/mandelbrot.cast)    | [i8048/mandelbrot.lst](../samples/i8048/mandelbrot.lst)       |
| CDP1804A | 198.3 |   8.099 |   2.639 |      | [mandelbrot.cast](../samples/cdp1804a/mandelbrot.cast) | [cdp1804a/mandelbrot.lst](../samples/cdp1804a/mandelbrot.lst) |
| CDP1802  | 247.8 |  10.125 |   3.329 |      | [mandelbrot.cast](../samples/cdp1802/mandelbrot.cast)  | [cdp1802/mandelbrot.lst](../samples/cdp1802/mandelbrot.lst)   |
| INS8060  | 716.5 |  29.262 |   9.505 |      | [mandelbrot.cast](../samples/ins8060/mandelbrot.cast)  | [ins8060/mandelbrot.lst](../samples/ins8060/mandelbrot.lst)   |
| F3850    | 817.3 |  33.413 |  11.388 |      | [mandelbrot.cast](../samples/f3850/mandelbrot.cast)    | [f3850/mandelbrot.lst](../samples/f3850/mandelbrot.lst)       |
