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

| CPU         | frame | avg row | std dev | Note        | Cast                                                                     | Listing                                                         |
| ----------- | ----: | ------: | ------: | ----------- | ------------------------------------------------------------------------ | --------------------------------------------------------------- |
| Z280        |   1.9 |   0.080 |   0.018 | cache on    | [mandelbrot.cast](../samples/z280/mandelbrot.cast)                       | [z280/mandelbrot.lst](../samples/z280/mandelbrot.lst)           |
| P8095BH     |   3.6 |   0.145 |   0.038 |             | [mandelbrot.cast](../samples/i8096/mandelbrot.cast)                      | [i8096/mandelbrot.lst](../samples/i8096/mandelbrot.lst)         |
| Z280        |   5.2 |   0.217 |   0.060 |             | [mandelbrot.cast](../samples/z280/mandelbrot.cast)                       | [z280/mandelbrot.lst](../samples/z280/mandelbrot.lst)           |
| TMS320C15   |   5.4 |   0.220 |   0.070 |             | [mandelbrot.cast](../samples/tms320c1x/mandelbrot.cast)                  | [tms320c1x/mandelbrot.lst](../samples/tms320c1x/mandelbrot.lst) |
| TMS99105    |   7.4 |   0.300 |   0.097 |             | [mandelbrot.cast](../samples/tms99105/mandelbrot.cast)                   | [tms99105/mandelbrot.lst](../samples/tms99105/mandelbrot.lst)   |
| TMS99105    |   7.4 |   0.302 |   0.097 |             | [mandelbrot_tms99105.cast](../samples/tms9995/mandelbrot_tms99105.cast)  | [tms9995/mandelbrot.lst](../samples/tms9995/mandelbrot.lst)     |
| TMS9995     |   9.2 |   0.375 |   0.119 |             | [mandelbrot.cast](../samples/tms9995/mandelbrot.cast)                    | [tms9995/mandelbrot.lst](../samples/tms9995/mandelbrot.lst)     |
| TMS99105    |   9.3 |   0.381 |   0.124 |             | [mandelbrot_tms99105.cast](../samples/tms9900/mandelbrot_tms99105.cast)  | [tms9900/mandelbrot.lst](../samples/tms9900/mandelbrot.lst)     |
| MN1613      |   9.5 |   0.388 |   0.128 |             | [mandelbrot.cast](../samples/mn1613/mandelbrot.cast)                     | [mn1613/mandelbrot.lst](../samples/mn1613/mandelbrot.lst)       |
| HD6309      |   9.8 |   0.401 |   0.128 |             | [mandelbrot.cast](../samples/hd6309/mandelbrot.cast)                     | [hd6309/mandelbrot.lst](../samples/hd6309/mandelbrot.lst)       |
| TMP90C802   |  14.1 |   0.576 |   0.191 |             | [mandelbrot.cast](../samples/tlcs90/mandelbrot.cast)                     | [tlcs90/mandelbrot.lst](../samples/tlcs90/mandelbrot.lst)       |
| TMS9995     |  14.5 |   0.591 |   0.192 |             | [mandelbrot_tms9995.cast](../samples/tms9900/mandelbrot_tms9995.cast)    | [tms9900/mandelbrot.lst](../samples/tms9900/mandelbrot.lst)     |
| MC68HC08AZ0 |  24.4 |   0.997 |   0.333 |             | [mandelbrot.cast](../samples/mc68hc08/mandelbrot.cast)                   | [mc68hc08/mandelbrot.lst](../samples/mc68hc08/mandelbrot.lst)   |
| MN1613      |  25.4 |   1.039 |   0.337 | MN1610 code | [mandelbrot.cast](../samples/mn1610/mandelbrot.cast)                     | [mn1610/mandelbrot.lst](../samples/mn1610/mandelbrot.lst)       |
| 68HC11      |  28.1 |   1.147 |   0.385 |             | [mandelbrot.cast](../samples/mc68hc11/mandelbrot.cast)                   | [mc68hc11/mandelbrot.lst](../samples/mc68hc11/mandelbrot.lst)   |
| TMS9900     |  29.0 |   1.184 |   0.387 |             | [mandelbrot.cast](../samples/tms9900/mandelbrot.cast)                    | [tms9900/mandelbrot.lst](../samples/tms9900/mandelbrot.lst)     |
| TMS370Cx5x  |  34.0 |   1.391 |   0.462 |             | [mandelbrot.cast](../samples/tms370/mandelbrot.cast)                     | [tms370/mandelbrot.lst](../samples/tms370/mandelbrot.lst)       |
| P8051       |  35.0 |   1.428 |   0.442 |             | [mandelbrot.cast](../samples/i8051/mandelbrot.cast)                      | [i8051/mandelbrot.lst](../samples/i8051/mandelbrot.lst)         |
| TMS9980     |  37.4 |   1.528 |   0.497 |             | [mandelbrot_tms9980.cast](../samples/tms9900/mandelbrot_tms9980.cast)    | [tms9900/mandelbrot.lst](../samples/tms9900/mandelbrot.lst)     |
| INS8070     |  56.6 |   2.312 |   0.744 |             | [mandelbrot.cast](../samples/ins8070/mandelbrot.cast)                    | [ins8070/mandelbrot.lst](../samples/ins8070/mandelbrot.lst)     |
| W65C816S    |  66.8 |   2.729 |   0.876 |             | [mandelbrot.cast](../samples/w65c816/mandelbrot.cast)                    | [w65c816/mandelbrot.lst](../samples/w65c816/mandelbrot.lst)     |
| MC6801      |  74.8 |   3.060 |   1.046 |             | [mandelbrot.cast](../samples/mc6801/mandelbrot.cast)                     | [mc6801/mandelbrot.lst](../samples/mc6801/mandelbrot.lst)       |
| P8085       |  77.7 |   3.172 |   1.040 |             | [mandelbrot.cast](../samples/i8085/mandelbrot.cast)                      | [i8085/mandelbrot.lst](../samples/i8085/mandelbrot.lst)         |
| MC68HC05C0  |  85.9 |   3.513 |   1.222 |             | [mandelbrot.cast](../samples/mc68hc05/mandelbrot.cast)                   | [mc68hc05/mandelbrot.lst](../samples/mc68hc05/mandelbrot.lst)   |
| MC68HC08AZ0 |  87.0 |   3.558 |   1.239 |             | [mandelbrot_mc68hc08.cast](../samples/mc68hc05/mandelbrot_mc68hc08.cast) | [mc68hc05/mandelbrot.lst](../samples/mc68hc05/mandelbrot.lst)   |
| HD6309      |  91.6 |   3.747 |   1.291 | native mode | [mandelbrot_hd6309.cast](../samples/mc6809/mandelbrot_hd6309.cast)       | [mc6809/mandelbrot.lst](../samples/mc6809/mandelbrot.lst)       |
| MC68HC05C0  |  91.7 |   3.746 |   1.245 |             | [mandelbrot_mc68hc05.cast](../samples/mc6805/mandelbrot_mc68hc05.cast)   | [mc6805/mandelbrot.lst](../samples/mc6805/mandelbrot.lst)       |
| TMS7000     |  92.7 |   3.786 |   1.230 |             | [mandelbrot.cast](../samples/tms7000/mandelbrot.cast)                    | [tms7000/mandelbrot.lst](../samples/tms7000/mandelbrot.lst)     |
| MC68HC08AZ0 |  94.5 |   3.859 |   1.281 |             | [mandelbrot_mc68hc08.cast](../samples/mc6805/mandelbrot_mc68hc08.cast)   | [mc6805/mandelbrot.lst](../samples/mc6805/mandelbrot.lst)       |
| MC6809      | 101.7 |   4.161 |   1.434 |             | [mandelbrot.cast](../samples/mc6809/mandelbrot.cast)                     | [mc6809/mandelbrot.lst](../samples/mc6809/mandelbrot.lst)       |
| MC6800      | 107.7 |   4.399 |   1.410 |             | [mandelbrot.cast](../samples/mc6800/mandelbrot.cast)                     | [mc6800/mandelbrot.lst](../samples/mc6800/mandelbrot.lst)       |
| P8080       | 132.0 |   5.390 |   1.763 |             | [mandelbrot.cast](../samples/i8080/mandelbrot.cast)                      | [i8080/mandelbrot.lst](../samples/i8080/mandelbrot.lst)         |
| MC146805E2  | 140.1 |   5.726 |   1.905 |             | [mandelbrot.cast](../samples/mc6805/mandelbrot.cast)                     | [mc6805/mandelbrot.lst](../samples/mc6805/mandelbrot.lst)       |
| MOS6502     | 144.8 |   5.917 |   1.951 |             | [mandelbrot.cast](../samples/mos6502/mandelbrot.cast)                    | [mos6502/mandelbrot.lst](../samples/mos6502/mandelbrot.lst)     |
| P8039       | 172.0 |   7.028 |   2.333 |             | [mandelbrot.cast](../samples/i8048/mandelbrot.cast)                      | [i8048/mandelbrot.lst](../samples/i8048/mandelbrot.lst)         |
| CDP1804A    | 198.3 |   8.099 |   2.639 |             | [mandelbrot.cast](../samples/cdp1804a/mandelbrot.cast)                   | [cdp1804a/mandelbrot.lst](../samples/cdp1804a/mandelbrot.lst)   |
| CDP1802     | 247.8 |  10.125 |   3.329 |             | [mandelbrot.cast](../samples/cdp1802/mandelbrot.cast)                    | [cdp1802/mandelbrot.lst](../samples/cdp1802/mandelbrot.lst)     |
| HD6120      | 258.2 |  10.552 |   3.524 |             | [mandelbrot_hd6120.cast](../samples/pdp8/mandelbrot_hd6120.cast)         | [pdp8/mandelbrot.lst](../samples/pdp8/mandelbrot.lst)           |
| SCN2650     | 355.7 |  16.045 |   4.247 |             | [mandelbrot.cast](../samples/scn2650/mandelbrot.cast)                    | [scn2650/mandelbrot.lst](../samples/scn2650/mandelbrot.lst)     |
| IM6100      | 386.1 |  17.547 |   4.958 |             | [mandelbrot.cast](../samples/pdp8/mandelbrot.cast)                       | [pdp8/mandelbrot.lst](../samples/pdp8/mandelbrot.lst)           |
| INS8060     | 716.5 |  29.262 |   9.505 |             | [mandelbrot.cast](../samples/ins8060/mandelbrot.cast)                    | [ins8060/mandelbrot.lst](../samples/ins8060/mandelbrot.lst)     |
| F3850       | 817.3 |  33.413 |  11.388 |             | [mandelbrot.cast](../samples/f3850/mandelbrot.cast)                      | [f3850/mandelbrot.lst](../samples/f3850/mandelbrot.lst)         |

## Floating point, fmandel

| CPU         | frame | avg row | std dev | Note        | Cast                                                                     | Listing                                                         |
| ----------- | ----: | ------: | ------: | ----------- | ------------------------------------------------------------------------ | --------------------------------------------------------------- |
| MN1613      |   8.1 |   0.334 |   0.129 |             | [fmandel.cast](../samples/mn1613/fmandel.cast)                           | [mn1613/fmandel.lst](../samples/mn1613/fmandel.lst)             |
| TMS99110    |  44.9 |   1.846 |   0.629 |             | [fmandel.cast](../samples/tms99110/fmandel.cast)                         | [tms99110/fmandel.lst](../samples/tms99110/fmandel.lst)         |

## Binary compatibility: the same code on related chips

A family's later chips run its earlier chips' code unchanged, so one
listing times several chips. Each table below is a family, in seconds per
frame from the casts in the tables above: a row is a listing, a
column the chip that ran it. Below them are each chip's bus and cycles,
from its data manual. The debugger drives each chip's clock, serves its
memory and records every bus cycle, so a board's pace is its own, not
the chip's.

### MC6805 family

| | MC146805E2 | MC68HC05C0 | MC68HC08AZ0 |
|---|---:|---:|---:|
| [mc6805](../samples/mc6805/mandelbrot.lst) | 140.1 | 91.7 | 94.5 |
| [mc68hc05](../samples/mc68hc05/mandelbrot.lst) | | 85.9 | 87.0 |
| [mc68hc08](../samples/mc68hc08/mandelbrot.lst) | | | 24.4 |
| Data bus | 8-bit, multiplexed | 8-bit, multiplexed | 8-bit |
| Bus cycle | OSC1 ÷ 5 | OSC1 ÷ 4 | OSC1 ÷ 4 |
| `STA ,X` / `BSR` cycles | 4 / 6 | 4 / 6 | 2 / 4 |
| Cycles of the 207 shared opcodes | 840 | 837 | 649 |

The MC68HC05 takes the MC146805's cycles for every opcode both have; the
MC68HC08 takes about a quarter fewer, overlapping each opcode fetch with
the instruction before it. Here the boards set the pace: the MC68HC05C0
board runs the 6805 code 1.5 times as fast as the MC146805E2 board on the
same cycles, and the MC68HC08AZ0 board spends about a quarter longer on
each bus cycle than the MC68HC05C0 board, which eats its fewer cycles.
What shows is the instruction set: `MUL` saves 6% on the MC68HC05, and the
MC68HC08's own code, with `DIV` and 16-bit `H:X` loads and stores, runs
nearly four times as fast as the code it inherited.

### MC6809 family

| | MC6809 | HD6309, native mode |
|---|---:|---:|
| [mc6809](../samples/mc6809/mandelbrot.lst) | 101.7 | 91.6 |
| [hd6309](../samples/hd6309/mandelbrot.lst) | | 9.8 |

The HD6309 runs MC6809 code as is, and in native mode, set from the
debugger for this run, most instructions take fewer cycles: 10% faster
here. Its own code, with `MULD`, `DIVQ` and the 16-bit W register, runs
nine times as fast.

### MN1610 family

| | MN1613 |
|---|---:|
| [mn1610](../samples/mn1610/mandelbrot.lst) | 25.4 |
| [mn1613](../samples/mn1613/mandelbrot.lst) | 9.5 |

The MN1613 runs MN1610 code as is. Its own code, with the hardware
multiply `M` and divide `D` the MN1610 lacks, runs nearly three times as
fast.

### TMS9900 family

| | TMS9900 | TMS9980 | TMS9995 | TMS99105 |
|---|---:|---:|---:|---:|
| [tms9900](../samples/tms9900/mandelbrot.lst) | 29.0 | 37.4 | 14.5 | 9.3 |
| [tms9995](../samples/tms9995/mandelbrot.lst) | | | 9.2 | 7.4 |
| [tms99105](../samples/tms99105/mandelbrot.lst) | | | | 7.4 |
| Data bus | 16-bit | 8-bit | 8-bit | 16-bit, multiplexed |
| A word moves in | 1 bus cycle of 2 clocks | 2 byte cycles | 2 byte cycles | 1 machine cycle |
| `A Rs,Rd` clocks | 14 | 22 | 4 on-chip, 8 off-chip | |

The TMS9980 keeps the TMS9900's cycles but moves each word as two bytes,
so the same code takes 30% longer. The TMS9995 has an 8-bit bus too, yet
prefetches and keeps RAM on chip, and needs far fewer clocks: it runs the TMS9900 code in half the time, and its own build,
which keeps the workspace registers in that RAM at `F000`, in a third.
The TMS99105 moves a word in one machine cycle on its multiplexed bus and
also prefetches; it runs all three, and its own build's `MPYS` and
`BLSK`/`BIND` gain nothing over the TMS9995 build on it.
