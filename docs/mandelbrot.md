# CPU speed by an assembly-language Mandelbrot

Every architecture's samples run the same Mandelbrot algorithm, written in
that chip's own assembly: [mandelbrot.c](../samples/arith/mandelbrot.c) in
16-bit integers, and on chips with floating-point instructions,
[fmandel.cpp](../samples/arith/fmandel.cpp) as well. Both draw 25 rows of 79
pixels, each printed as its iteration count, `0`-`9` and `A`-`F`, or a space
after 16. Each character goes into a queue that the serial port's
transmit interrupt drains, so sending overlaps the next pixel's work.
On chips that draw a frame in under 10 seconds, a frame ends once the
queue is empty.
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
  debugger's terminal while the chip draws, with its timing; the link
  plays it in the browser at the speed it ran, beside its listing, and
  `asciinema play` replays the file in a terminal.
- **Listing** is the assembly source the chip ran.

<!-- Link a cast as
     https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/<arch>/<name>.cast&lst=samples/<arch>/<name>.lst
     with the cast file name as the text and the row's listing; docs/play.html
     plays it beside the listing. -->

## Integer, mandelbrot

| CPU         | frame | avg row | std dev | Note                 | Cast                                                                                                                                                              | Listing                                                               |
| ----------- | ----: | ------: | ------: | -------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------- |
| S80296SA    |   1.7 |   0.068 |   0.018 | i80296 code          | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/i80296/mandelbrot.cast&lst=samples/i80296/mandelbrot.lst)                       | [i80296/mandelbrot.lst](../samples/i80296/mandelbrot.lst)             |
| S80296SA    |   1.8 |   0.071 |   0.019 | i8096 code           | [mandelbrot_s80296sa.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/i8096/mandelbrot_s80296sa.cast&lst=samples/i8096/mandelbrot.lst)       | [i8096/mandelbrot.lst](../samples/i8096/mandelbrot.lst)               |
| Z280        |   1.9 |   0.078 |   0.017 | cache on             | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/z280/mandelbrot.cast&lst=samples/z280/mandelbrot.lst)                           | [z280/mandelbrot.lst](../samples/z280/mandelbrot.lst)                 |
| MC68HC16Z1  |   2.3 |   0.095 |   0.029 | PWM 10MHz            | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mc68hc16/mandelbrot.cast&lst=samples/mc68hc16/mandelbrot.lst)                   | [mc68hc16/mandelbrot.lst](../samples/mc68hc16/mandelbrot.lst)         |
| Z8002       |   2.7 |   0.108 |   0.030 | normal mode          | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/z8002/mandelbrot.cast&lst=samples/z8002/mandelbrot.lst)                         | [z8002/mandelbrot.lst](../samples/z8002/mandelbrot.lst)               |
| MC68HC12    |   2.8 |   0.116 |   0.033 |                      | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mc68hc12/mandelbrot.cast&lst=samples/mc68hc12/mandelbrot.lst)                   | [mc68hc12/mandelbrot.lst](../samples/mc68hc12/mandelbrot.lst)         |
| N8097BH     |   3.0 |   0.121 |   0.034 | i8096 code           | [mandelbrot_n8097.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/i8096/mandelbrot_n8097.cast&lst=samples/i8096/mandelbrot.lst)             | [i8096/mandelbrot.lst](../samples/i8096/mandelbrot.lst)               |
| S80C196KC   |   3.1 |   0.125 |   0.035 | i8096 code           | [mandelbrot_s80c196kc.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/i8096/mandelbrot_s80c196kc.cast&lst=samples/i8096/mandelbrot.lst)     | [i8096/mandelbrot.lst](../samples/i8096/mandelbrot.lst)               |
| Z8001       |   3.1 |   0.126 |   0.035 | segmented, normal    | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/z8001/mandelbrot.cast&lst=samples/z8001/mandelbrot.lst)                         | [z8001/mandelbrot.lst](../samples/z8001/mandelbrot.lst)               |
| Z8001       |   3.1 |   0.125 |   0.034 | nonsegmented, normal | [mandelbrot_nonseg.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/z8001/mandelbrot_nonseg.cast&lst=samples/z8001/mandelbrot_nonseg.lst)    | [z8001/mandelbrot_nonseg.lst](../samples/z8001/mandelbrot_nonseg.lst) |
| P8095BH     |   3.1 |   0.127 |   0.036 | i8096 code           | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/i8096/mandelbrot.cast&lst=samples/i8096/mandelbrot.lst)                         | [i8096/mandelbrot.lst](../samples/i8096/mandelbrot.lst)               |
| Z380        |   3.7 |   0.151 |   0.044 |                      | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/z380/mandelbrot.cast&lst=samples/z380/mandelbrot.lst)                           | [z380/mandelbrot.lst](../samples/z380/mandelbrot.lst)                 |
| Z280        |   5.2 |   0.213 |   0.063 |                      | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/z280/mandelbrot.cast&lst=samples/z280/mandelbrot.lst)                           | [z280/mandelbrot.lst](../samples/z280/mandelbrot.lst)                 |
| TMS320C15   |   5.7 |   0.233 |   0.075 | TMS320C1x code       | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/tms320c1x/mandelbrot.cast&lst=samples/tms320c1x/mandelbrot.lst)                 | [tms320c1x/mandelbrot.lst](../samples/tms320c1x/mandelbrot.lst)       |
| TMS99105    |   6.5 |   0.267 |   0.086 |                      | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/tms99105/mandelbrot.cast&lst=samples/tms99105/mandelbrot.lst)                   | [tms99105/mandelbrot.lst](../samples/tms99105/mandelbrot.lst)         |
| TMS99105    |   6.6 |   0.268 |   0.086 | TMS9995 code         | [mandelbrot_tms99105.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/tms9995/mandelbrot_tms99105.cast&lst=samples/tms9995/mandelbrot.lst)   | [tms9995/mandelbrot.lst](../samples/tms9995/mandelbrot.lst)           |
| TMS9995     |   8.2 |   0.334 |   0.106 |                      | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/tms9995/mandelbrot.cast&lst=samples/tms9995/mandelbrot.lst)                     | [tms9995/mandelbrot.lst](../samples/tms9995/mandelbrot.lst)           |
| TMS99105    |   8.3 |   0.340 |   0.111 | TMS9900 code         | [mandelbrot_tms99105.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/tms9900/mandelbrot_tms99105.cast&lst=samples/tms9900/mandelbrot.lst)   | [tms9900/mandelbrot.lst](../samples/tms9900/mandelbrot.lst)           |
| HD6309E     |  10.0 |   0.410 |   0.130 | HD6309 code          | [mandelbrot_hd6309e.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/hd6309/mandelbrot_hd6309e.cast&lst=samples/hd6309/mandelbrot.lst)       | [hd6309/mandelbrot.lst](../samples/hd6309/mandelbrot.lst)             |
| MN1613      |  10.4 |   0.427 |   0.141 |                      | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mn1613/mandelbrot.cast&lst=samples/mn1613/mandelbrot.lst)                       | [mn1613/mandelbrot.lst](../samples/mn1613/mandelbrot.lst)             |
| HD6309      |  10.8 |   0.443 |   0.142 |                      | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/hd6309/mandelbrot.cast&lst=samples/hd6309/mandelbrot.lst)                       | [hd6309/mandelbrot.lst](../samples/hd6309/mandelbrot.lst)             |
| TMS9995     |  12.8 |   0.525 |   0.171 | TMS9900 code         | [mandelbrot_tms9995.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/tms9900/mandelbrot_tms9995.cast&lst=samples/tms9900/mandelbrot.lst)     | [tms9900/mandelbrot.lst](../samples/tms9900/mandelbrot.lst)           |
| MN1613      |  13.0 |   0.530 |   0.135 | MN1610 code          | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mn1610/mandelbrot.cast&lst=samples/mn1610/mandelbrot.lst)                       | [mn1610/mandelbrot.lst](../samples/mn1610/mandelbrot.lst)             |
| TMS370Cx5x  |  13.8 |   0.561 |   0.176 | TMS370 code          | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/tms370/mandelbrot.cast&lst=samples/tms370/mandelbrot.lst)                       | [tms370/mandelbrot.lst](../samples/tms370/mandelbrot.lst)             |
| TMP90C802   |  14.1 |   0.576 |   0.191 | TLCS90 code          | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/tlcs90/mandelbrot.cast&lst=samples/tlcs90/mandelbrot.lst)                       | [tlcs90/mandelbrot.lst](../samples/tlcs90/mandelbrot.lst)             |
| KL5C80A12   |  16.5 |   0.669 |   0.172 | Z80 code             | [mandelbrot_kl5c80.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/z80/mandelbrot_kl5c80.cast&lst=samples/z80/mandelbrot.lst)               | [z80/mandelbrot.lst](../samples/z80/mandelbrot.lst)                   |
| Z380        |  16.7 |   0.681 |   0.182 | Z180 code            | [mandelbrot_z380.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/z180/mandelbrot_z380.cast&lst=samples/z180/mandelbrot.lst)                 | [z180/mandelbrot.lst](../samples/z180/mandelbrot.lst)                 |
| Z380        |  17.4 |   0.709 |   0.185 | Z80 code             | [mandelbrot_z380.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/z80/mandelbrot_z380.cast&lst=samples/z80/mandelbrot.lst)                   | [z80/mandelbrot.lst](../samples/z80/mandelbrot.lst)                   |
| Z88C00      |  21.3 |   0.870 |   0.289 | Z88 code             | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/z88/mandelbrot.cast&lst=samples/z88/mandelbrot.lst)                             | [z88/mandelbrot.lst](../samples/z88/mandelbrot.lst)                   |
| MC68HC08AZ0 |  23.0 |   0.941 |   0.312 |                      | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mc68hc08/mandelbrot.cast&lst=samples/mc68hc08/mandelbrot.lst)                   | [mc68hc08/mandelbrot.lst](../samples/mc68hc08/mandelbrot.lst)         |
| TMS9900     |  27.3 |   1.116 |   0.365 |                      | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/tms9900/mandelbrot.cast&lst=samples/tms9900/mandelbrot.lst)                     | [tms9900/mandelbrot.lst](../samples/tms9900/mandelbrot.lst)           |
| Z80         |  27.5 |   1.117 |   0.283 |                      | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/z80/mandelbrot.cast&lst=samples/z80/mandelbrot.lst)                             | [z80/mandelbrot.lst](../samples/z80/mandelbrot.lst)                   |
| MC68HC11    |  28.1 |   1.147 |   0.385 |                      | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mc68hc11/mandelbrot.cast&lst=samples/mc68hc11/mandelbrot.lst)                   | [mc68hc11/mandelbrot.lst](../samples/mc68hc11/mandelbrot.lst)         |
| Z180        |  28.7 |   1.167 |   0.323 |                      | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/z180/mandelbrot.cast&lst=samples/z180/mandelbrot.lst)                           | [z180/mandelbrot.lst](../samples/z180/mandelbrot.lst)                 |
| HD64180S    |  28.7 |   1.168 |   0.323 | Z180 code            | [mandelbrot_hd64180s.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/z180/mandelbrot_hd64180s.cast&lst=samples/z180/mandelbrot.lst)         | [z180/mandelbrot.lst](../samples/z180/mandelbrot.lst)                 |
| TMS7002     |  29.6 |   1.202 |   0.316 | TMS7000 code         | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/tms7000/mandelbrot.cast&lst=samples/tms7000/mandelbrot.lst)                     | [tms7000/mandelbrot.lst](../samples/tms7000/mandelbrot.lst)           |
| TMS70C02    |  29.6 |   1.202 |   0.315 | TMS7000 code         | [mandelbrot_tms70c02.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/tms7000/mandelbrot_tms70c02.cast&lst=samples/tms7000/mandelbrot.lst)   | [tms7000/mandelbrot.lst](../samples/tms7000/mandelbrot.lst)           |
| HD6301      |  30.2 |   1.226 |   0.285 | MC6801 code          | [mandelbrot_hd6301.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mc6801/mandelbrot_hd6301.cast&lst=samples/mc6801/mandelbrot.lst)         | [mc6801/mandelbrot.lst](../samples/mc6801/mandelbrot.lst)             |
| Z280        |  30.7 |   1.250 |   0.320 | Z80 code             | [mandelbrot_z280.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/z80/mandelbrot_z280.cast&lst=samples/z80/mandelbrot.lst)                   | [z80/mandelbrot.lst](../samples/z80/mandelbrot.lst)                   |
| Z86C91      |  30.7 |   1.250 |   0.350 | Z86 code             | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/z86/mandelbrot.cast&lst=samples/z86/mandelbrot.lst)                             | [z86/mandelbrot.lst](../samples/z86/mandelbrot.lst)                   |
| Z180        |  31.3 |   1.271 |   0.324 | Z80 code             | [mandelbrot_z180.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/z80/mandelbrot_z180.cast&lst=samples/z80/mandelbrot.lst)                   | [z80/mandelbrot.lst](../samples/z80/mandelbrot.lst)                   |
| HD64180S    |  31.3 |   1.272 |   0.324 | Z80 code             | [mandelbrot_hd64180s.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/z80/mandelbrot_hd64180s.cast&lst=samples/z80/mandelbrot.lst)           | [z80/mandelbrot.lst](../samples/z80/mandelbrot.lst)                   |
| HD6309E     |  31.3 |   1.270 |   0.299 | MC6809 code          | [mandelbrot_hd6309e.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mc6809/mandelbrot_hd6309e.cast&lst=samples/mc6809/mandelbrot.lst)       | [mc6809/mandelbrot.lst](../samples/mc6809/mandelbrot.lst)             |
| Z80         |  32.6 |   1.323 |   0.319 | i8080 code           | [mandelbrot_z80.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/i8080/mandelbrot_z80.cast&lst=samples/i8080/mandelbrot.lst)                 | [i8080/mandelbrot.lst](../samples/i8080/mandelbrot.lst)               |
| MC68HC11    |  32.7 |   1.328 |   0.303 | MC6801 code          | [mandelbrot_mc68hc11.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mc6801/mandelbrot_mc68hc11.cast&lst=samples/mc6801/mandelbrot.lst)     | [mc6801/mandelbrot.lst](../samples/mc6801/mandelbrot.lst)             |
| MC6801      |  32.9 |   1.336 |   0.304 |                      | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mc6801/mandelbrot.cast&lst=samples/mc6801/mandelbrot.lst)                       | [mc6801/mandelbrot.lst](../samples/mc6801/mandelbrot.lst)             |
| HD6309      |  33.0 |   1.339 |   0.315 | MC6809 code          | [mandelbrot_hd6309.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mc6809/mandelbrot_hd6309.cast&lst=samples/mc6809/mandelbrot.lst)         | [mc6809/mandelbrot.lst](../samples/mc6809/mandelbrot.lst)             |
| W65C816S    |  33.7 |   1.368 |   0.331 |                      | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/w65c816/mandelbrot.cast&lst=samples/w65c816/mandelbrot.lst)                     | [w65c816/mandelbrot.lst](../samples/w65c816/mandelbrot.lst)           |
| MC6809E     |  34.7 |   1.410 |   0.335 | MC6809 code          | [mandelbrot_mc6809e.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mc6809/mandelbrot_mc6809e.cast&lst=samples/mc6809/mandelbrot.lst)       | [mc6809/mandelbrot.lst](../samples/mc6809/mandelbrot.lst)             |
| P8051       |  35.0 |   1.428 |   0.442 | i8051 code           | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/i8051/mandelbrot.cast&lst=samples/i8051/mandelbrot.lst)                         | [i8051/mandelbrot.lst](../samples/i8051/mandelbrot.lst)               |
| NSC800      |  36.2 |   1.472 |   0.372 | Z80 code             | [mandelbrot_nsc800.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/z80/mandelbrot_nsc800.cast&lst=samples/z80/mandelbrot.lst)               | [z80/mandelbrot.lst](../samples/z80/mandelbrot.lst)                   |
| MC6809      |  36.6 |   1.488 |   0.354 |                      | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mc6809/mandelbrot.cast&lst=samples/mc6809/mandelbrot.lst)                       | [mc6809/mandelbrot.lst](../samples/mc6809/mandelbrot.lst)             |
| TMS9980     |  38.9 |   1.588 |   0.515 | TMS9900 code         | [mandelbrot_tms9980.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/tms9900/mandelbrot_tms9980.cast&lst=samples/tms9900/mandelbrot.lst)     | [tms9900/mandelbrot.lst](../samples/tms9900/mandelbrot.lst)           |
| TMS9981     |  38.9 |   1.588 |   0.515 | TMS9900 code         | [mandelbrot_tms9981.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/tms9900/mandelbrot_tms9981.cast&lst=samples/tms9900/mandelbrot.lst)     | [tms9900/mandelbrot.lst](../samples/tms9900/mandelbrot.lst)           |
| MC68HC08AZ0 |  39.9 |   1.628 |   0.521 | MC68HC05 code        | [mandelbrot_mc68hc08.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mc68hc05/mandelbrot_mc68hc08.cast&lst=samples/mc68hc05/mandelbrot.lst) | [mc68hc05/mandelbrot.lst](../samples/mc68hc05/mandelbrot.lst)         |
| P8085       |  40.2 |   1.632 |   0.393 | i8080 code           | [mandelbrot_p8085.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/i8080/mandelbrot_p8085.cast&lst=samples/i8080/mandelbrot.lst)             | [i8080/mandelbrot.lst](../samples/i8080/mandelbrot.lst)               |
| HD6301      |  41.1 |   1.666 |   0.375 | MC6800 code          | [mandelbrot_hd6301.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mc6800/mandelbrot_hd6301.cast&lst=samples/mc6800/mandelbrot.lst)         | [mc6800/mandelbrot.lst](../samples/mc6800/mandelbrot.lst)             |
| MC68HC11    |  42.3 |   1.716 |   0.382 | MC6800 code          | [mandelbrot_mc68hc11.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mc6800/mandelbrot_mc68hc11.cast&lst=samples/mc6800/mandelbrot.lst)     | [mc6800/mandelbrot.lst](../samples/mc6800/mandelbrot.lst)             |
| NSC800      |  43.2 |   1.755 |   0.423 | i8080 code           | [mandelbrot_nsc800.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/i8080/mandelbrot_nsc800.cast&lst=samples/i8080/mandelbrot.lst)           | [i8080/mandelbrot.lst](../samples/i8080/mandelbrot.lst)               |
| MC6801      |  43.4 |   1.760 |   0.392 | MC6800 code          | [mandelbrot_mc6801.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mc6800/mandelbrot_mc6801.cast&lst=samples/mc6800/mandelbrot.lst)         | [mc6800/mandelbrot.lst](../samples/mc6800/mandelbrot.lst)             |
| MC68HC05C0  |  44.5 |   1.816 |   0.582 |                      | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mc68hc05/mandelbrot.cast&lst=samples/mc68hc05/mandelbrot.lst)                   | [mc68hc05/mandelbrot.lst](../samples/mc68hc05/mandelbrot.lst)         |
| MC68HC08AZ0 |  44.6 |   1.820 |   0.541 | MC6805 code          | [mandelbrot_mc68hc08.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mc6805/mandelbrot_mc68hc08.cast&lst=samples/mc6805/mandelbrot.lst)     | [mc6805/mandelbrot.lst](../samples/mc6805/mandelbrot.lst)             |
| MC68HC05C0  |  49.6 |   2.024 |   0.602 | MC6805 code          | [mandelbrot_mc68hc05.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mc6805/mandelbrot_mc68hc05.cast&lst=samples/mc6805/mandelbrot.lst)     | [mc6805/mandelbrot.lst](../samples/mc6805/mandelbrot.lst)             |
| MC6802      |  49.8 |   2.018 |   0.453 | MC6800 code          | [mandelbrot_mc6802.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mc6800/mandelbrot_mc6802.cast&lst=samples/mc6800/mandelbrot.lst)         | [mc6800/mandelbrot.lst](../samples/mc6800/mandelbrot.lst)             |
| MB8870      |  49.8 |   2.018 |   0.453 | MC6800 code          | [mandelbrot_mb8870.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mc6800/mandelbrot_mb8870.cast&lst=samples/mc6800/mandelbrot.lst)         | [mc6800/mandelbrot.lst](../samples/mc6800/mandelbrot.lst)             |
| MC6800      |  51.1 |   2.071 |   0.465 |                      | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mc6800/mandelbrot.cast&lst=samples/mc6800/mandelbrot.lst)                       | [mc6800/mandelbrot.lst](../samples/mc6800/mandelbrot.lst)             |
| MB8861      |  51.1 |   2.071 |   0.465 | MC6800 code          | [mandelbrot_mb8861.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mc6800/mandelbrot_mb8861.cast&lst=samples/mc6800/mandelbrot.lst)         | [mc6800/mandelbrot.lst](../samples/mc6800/mandelbrot.lst)             |
| INS8070     |  58.1 |   2.374 |   0.763 |                      | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/ins8070/mandelbrot.cast&lst=samples/ins8070/mandelbrot.lst)                     | [ins8070/mandelbrot.lst](../samples/ins8070/mandelbrot.lst)           |
| HD6120      |  59.2 |   2.401 |   0.546 | PDP8 code            | [mandelbrot_hd6120.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/pdp8/mandelbrot_hd6120.cast&lst=samples/pdp8/mandelbrot.lst)             | [pdp8/mandelbrot.lst](../samples/pdp8/mandelbrot.lst)                 |
| P8080       |  63.2 |   2.566 |   0.611 |                      | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/i8080/mandelbrot.cast&lst=samples/i8080/mandelbrot.lst)                         | [i8080/mandelbrot.lst](../samples/i8080/mandelbrot.lst)               |
| MC146805E2  |  67.0 |   2.733 |   0.814 | MC6805 code          | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mc6805/mandelbrot.cast&lst=samples/mc6805/mandelbrot.lst)                       | [mc6805/mandelbrot.lst](../samples/mc6805/mandelbrot.lst)             |
| SCN2650     |  69.8 |   2.830 |   0.649 | PWM 2.0MHz           | [mandelbrot_2mhz.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/scn2650/mandelbrot_2mhz.cast&lst=samples/scn2650/mandelbrot.lst)           | [scn2650/mandelbrot.lst](../samples/scn2650/mandelbrot.lst)           |
| MOS6502     |  69.9 |   2.843 |   0.769 |                      | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mos6502/mandelbrot.cast&lst=samples/mos6502/mandelbrot.lst)                     | [mos6502/mandelbrot.lst](../samples/mos6502/mandelbrot.lst)           |
| G65SC02     |  70.0 |   2.849 |   0.770 | MOS6502 code         | [mandelbrot_g65sc02.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mos6502/mandelbrot_g65sc02.cast&lst=samples/mos6502/mandelbrot.lst)     | [mos6502/mandelbrot.lst](../samples/mos6502/mandelbrot.lst)           |
| R65C02      |  70.0 |   2.849 |   0.770 | MOS6502 code         | [mandelbrot_r65c02.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mos6502/mandelbrot_r65c02.cast&lst=samples/mos6502/mandelbrot.lst)       | [mos6502/mandelbrot.lst](../samples/mos6502/mandelbrot.lst)           |
| W65C02S     |  70.0 |   2.849 |   0.770 | MOS6502 code         | [mandelbrot_w65c02s.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mos6502/mandelbrot_w65c02s.cast&lst=samples/mos6502/mandelbrot.lst)     | [mos6502/mandelbrot.lst](../samples/mos6502/mandelbrot.lst)           |
| W65C816S    |  71.6 |   2.915 |   0.788 | MOS6502 code         | [mandelbrot_w65c816.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mos6502/mandelbrot_w65c816.cast&lst=samples/mos6502/mandelbrot.lst)     | [mos6502/mandelbrot.lst](../samples/mos6502/mandelbrot.lst)           |
| P8039       |  76.2 |   3.099 |   0.832 | i8048 code           | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/i8048/mandelbrot.cast&lst=samples/i8048/mandelbrot.lst)                         | [i8048/mandelbrot.lst](../samples/i8048/mandelbrot.lst)               |
| IM6100      |  89.4 |   3.627 |   0.817 | PDP8 code            | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/pdp8/mandelbrot.cast&lst=samples/pdp8/mandelbrot.lst)                           | [pdp8/mandelbrot.lst](../samples/pdp8/mandelbrot.lst)                 |
| SCN2650     | 111.6 |   4.526 |   1.038 | PWM 1.25MHz          | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/scn2650/mandelbrot.cast&lst=samples/scn2650/mandelbrot.lst)                     | [scn2650/mandelbrot.lst](../samples/scn2650/mandelbrot.lst)           |
| CDP1804A    | 112.7 |   4.583 |   1.173 |                      | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/cdp1804a/mandelbrot.cast&lst=samples/cdp1804a/mandelbrot.lst)                   | [cdp1804a/mandelbrot.lst](../samples/cdp1804a/mandelbrot.lst)         |
| CDP1802     | 165.7 |   6.749 |   1.903 |                      | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/cdp1802/mandelbrot.cast&lst=samples/cdp1802/mandelbrot.lst)                     | [cdp1802/mandelbrot.lst](../samples/cdp1802/mandelbrot.lst)           |
| F3850       | 249.9 |  10.195 |   3.143 |                      | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/f3850/mandelbrot.cast&lst=samples/f3850/mandelbrot.lst)                         | [f3850/mandelbrot.lst](../samples/f3850/mandelbrot.lst)               |
| INS8060     | 256.3 |  10.390 |   2.264 |                      | [mandelbrot.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/ins8060/mandelbrot.cast&lst=samples/ins8060/mandelbrot.lst)                     | [ins8060/mandelbrot.lst](../samples/ins8060/mandelbrot.lst)           |

## Floating point, fmandel

| CPU      | frame | avg row | std dev | Note | Cast                                                                                                                                   | Listing                                                 |
| -------- | ----: | ------: | ------: | ---- | -------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------- |
| MN1613   |   8.4 |   0.345 |   0.133 |      | [fmandel.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mn1613/fmandel.cast&lst=samples/mn1613/fmandel.lst)     | [mn1613/fmandel.lst](../samples/mn1613/fmandel.lst)     |
| TMS99110 |  40.3 |   1.653 |   0.564 |      | [fmandel.cast](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/tms99110/fmandel.cast&lst=samples/tms99110/fmandel.lst) | [tms99110/fmandel.lst](../samples/tms99110/fmandel.lst) |

## Binary compatibility: the same code on related chips

A family's later chips run its earlier chips' code unchanged, so one
listing times several chips. Each table below is a family, in seconds per
frame from the casts in the tables above: a row is a listing, a
column the chip that ran it. Below them are each chip's bus and cycles,
from its data manual. The debugger drives each chip's clock, serves its
memory and records every bus cycle, so a board's pace is its own, not
the chip's.

### MC6800 family

| | MC6800 | MC6802 | MC6801 | HD6301 | MC68HC11 |
|---|---:|---:|---:|---:|---:|
| [mc6800](../samples/mc6800/mandelbrot.lst) | 51.1 | 49.8 | 43.4 | 41.1 | 42.3 |
| [mc6801](../samples/mc6801/mandelbrot.lst) | | | 32.9 | 30.2 | 32.7 |
| [mc68hc11](../samples/mc68hc11/mandelbrot.lst) | | | | | 28.1 |

The MC6802 runs MC6800 code on the same cycles, with its clock on chip.
The MC6801, HD6301 and MC68HC11 run it too, in fewer cycles: `BSR`
takes 8 on the MC6800, 6 on the MC6801 and 5 on the HD6301. The MC6801
code uses `MUL` and the 16-bit D register, and the MC68HC11's own code
divides with `IDIV` and swaps D and X with `XGDX`.

### MC6805 family

| | MC146805E2 | MC68HC05C0 | MC68HC08AZ0 |
|---|---:|---:|---:|
| [mc6805](../samples/mc6805/mandelbrot.lst) | 67.0 | 49.6 | 44.6 |
| [mc68hc05](../samples/mc68hc05/mandelbrot.lst) | | 44.5 | 39.9 |
| [mc68hc08](../samples/mc68hc08/mandelbrot.lst) | | | 23.0 |
| Data bus | 8-bit, multiplexed | 8-bit, multiplexed | 8-bit |
| Bus cycle | OSC1 ÷ 5 | OSC1 ÷ 4 | OSC1 ÷ 4 |
| `STA ,X` / `BSR` cycles | 4 / 6 | 4 / 6 | 2 / 4 |
| Cycles of the 207 shared opcodes | 840 | 837 | 649 |

The MC68HC05 takes the MC146805's cycles for every opcode both have; the
MC68HC08 takes fewer, overlapping each opcode fetch with the instruction
before it. The boards set much of the pace: the MC68HC05C0 runs the 6805
code faster than the MC146805E2 on the same cycles because its bus cycle
is four OSC1 periods, not five, and the MC68HC08AZ0 board spends longer
on each bus cycle than the MC68HC05C0 board, which eats most of its
fewer cycles. The MC68HC05 code gains from `MUL`, and the MC68HC08's own
code from `DIV` and 16-bit `H:X` loads and stores. The 6805 code reaches
the ACIA at 17F8 on all three chips, moved there on the later two, and
services the MC68HC08's COP at FFFF, which the MC146805E2's 13-bit bus
sees as 1FFF.

### MC6809 family

| | MC6809 | HD6309, native mode |
|---|---:|---:|
| [mc6809](../samples/mc6809/mandelbrot.lst) | 36.6 | 33.0 |
| [hd6309](../samples/hd6309/mandelbrot.lst) | | 10.8 |

The HD6309 runs MC6809 code as is, and in native mode, set from the
debugger for this run, most instructions take fewer cycles. Its own code
is faster still, with `MULD`, `DIVQ` and the 16-bit W register.

### MCS-96 family

| | P8095BH | N8097BH | S80C196KC | S80296SA |
|---|---:|---:|---:|---:|
| [i8096](../samples/i8096/mandelbrot.lst) | 3.1 | 3.0 | 3.1 | 1.8 |
| [i80296](../samples/i80296/mandelbrot.lst) | | | | 1.7 |
| Data bus | 16-bit, multiplexed | 16-bit, multiplexed | 16-bit, multiplexed | 16-bit, multiplexed |

All four chips run on a 16-bit data bus, with ALE marking the address. A word
moves in one bus cycle, so the code runs faster than on an 8-bit bus.
Every read takes a whole word, so the USART's registers sit a word
apart; otherwise a status read would also read the received data and
lose it.

### MN1610 family

| | MN1613 |
|---|---:|
| [mn1610](../samples/mn1610/mandelbrot.lst) | 13.0 |
| [mn1613](../samples/mn1613/mandelbrot.lst) | 10.4 |

The MN1613 runs MN1610 code as is. Its own code is faster because it
uses the hardware multiply `M` and divide `D` the MN1610 lacks.

### PDP-8 family

| | IM6100 | HD6120 |
|---|---:|---:|
| [pdp8](../samples/pdp8/mandelbrot.lst) | 89.4 | 59.2 |
| Data bus | 12-bit, multiplexed | 12-bit, multiplexed |

The HD6120 runs the IM6100's code as is, and in less time on its board.
The code uses none of the HD6120's own instructions, such as its stacks
(`PPC1`/`RTN1`).

### TMS9900 family

| | TMS9900 | TMS9980 | TMS9981 | TMS9995 | TMS99105 |
|---|---:|---:|---:|---:|---:|
| [tms9900](../samples/tms9900/mandelbrot.lst) | 27.3 | 38.9 | 38.9 | 12.8 | 8.3 |
| [tms9995](../samples/tms9995/mandelbrot.lst) | | | | 8.2 | 6.6 |
| [tms99105](../samples/tms99105/mandelbrot.lst) | | | | | 6.5 |
| Data bus | 16-bit | 8-bit | 8-bit | 8-bit | 16-bit, multiplexed |
| A word moves in | 1 bus cycle of 2 clocks | 2 byte cycles | 2 byte cycles | 2 byte cycles | 1 machine cycle |
| `A Rs,Rd` clocks | 14 | 22 | 22 | 4 on-chip, 8 off-chip | |

The TMS9980 and TMS9981 keep the TMS9900's cycles but move each word as
two bytes, so the same code takes longer. Both run from a free-running
10 MHz CKIN and wait on `READY` for each byte, so they draw alike. The TMS9995 has an 8-bit bus too, yet
prefetches and keeps RAM on chip, and needs far fewer clocks: it runs the TMS9900 code in half the time, and its own build,
which keeps the workspace registers in that RAM at `F000`, in a third.
The TMS99105 moves a word in one machine cycle on its multiplexed bus and
also prefetches; it runs all three, and its own build's `MPYS` and
`BLSK`/`BIND` gain nothing over the TMS9995 build on it. Its column
ran on a TMS99110 with no macrostore ROM (`MACRO=0`), which runs as a
TMS99105.

### 8080 and Z80 family

| | P8080 | P8085 | Z80 | Z180 | Z280 | Z380 | NSC800 | HD64180S | KL5C80A12 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [i8080](../samples/i8080/mandelbrot.lst) | 63.2 | 40.2 | 32.6 | | | | 43.2 | | |
| [z80](../samples/z80/mandelbrot.lst) | | | 27.5 | 31.3 | 30.7 | 17.4 | 36.2 | 31.3 | 16.5 |
| [z180](../samples/z180/mandelbrot.lst) | | | | 28.7 | | 16.7 | | 28.7 | |
| [z280](../samples/z280/mandelbrot.lst) | | | | | 5.2 | | | | |
| [z380](../samples/z380/mandelbrot.lst) | | | | | | 3.7 | | | |
| Data bus | 8-bit | 8-bit, multiplexed | 8-bit | 8-bit | 16-bit, multiplexed | 16-bit | 8-bit, multiplexed | 8-bit | 8-bit |

Every chip runs with its cache off, as reset leaves it. The Z80 code
beats the 8080 code on the same chip because it shifts its registers
in place with `SRL`, `RR`, `SLA` and `RL`, where the 8080 code moves
every byte through A, and its divide keeps its count in A instead of
on the stack. The Z180 code adds `MLT`, which replaces the multiply
loop; the Z280 and Z380 codes go further, with `MULTW` and `DIVW`
or `DIVUW` inline in the inner loop, so neither loop is left. The
Z380 and the KL5C80A12 run the Z80 code itself faster than a Z80
because they take fewer clocks per instruction, and the NSC800 runs
it slower, its multiplexed bus taking extra clocks per access.
