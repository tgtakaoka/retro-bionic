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

| CPU         | frame | avg row | std dev | Note               | Cast                                                                     | Listing                                                         |
| ----------- | ----: | ------: | ------: | ------------------ | ------------------------------------------------------------------------ | --------------------------------------------------------------- |
| Z280        |   2.1 |   0.080 |   0.018 | cache on           | [mandelbrot.cast](../samples/z280/mandelbrot.cast)                       | [z280/mandelbrot.lst](../samples/z280/mandelbrot.lst)           |
| Z8002       |   2.7 |   0.111 |   0.029 | normal mode        | [mandelbrot.cast](../samples/z8002/mandelbrot.cast)                      | [z8002/mandelbrot.lst](../samples/z8002/mandelbrot.lst)         |
| Z8001       |   3.1 |   0.129 |   0.033 | segmented, normal  | [mandelbrot.cast](../samples/z8001/mandelbrot.cast)                      | [z8001/mandelbrot.lst](../samples/z8001/mandelbrot.lst)         |
| N8097BH     |   3.4 |   0.137 |   0.036 | 16-bit bus         | [mandelbrot_n8097.cast](../samples/i8096/mandelbrot_n8097.cast)          | [i8096/mandelbrot.lst](../samples/i8096/mandelbrot.lst)         |
| P8095BH     |   3.5 |   0.140 |   0.037 | 16-bit bus         | [mandelbrot.cast](../samples/i8096/mandelbrot.cast)                      | [i8096/mandelbrot.lst](../samples/i8096/mandelbrot.lst)         |
| Z380        |   3.7 |   0.153 |   0.042 |                    | [mandelbrot.cast](../samples/z380/mandelbrot.cast)                       | [z380/mandelbrot.lst](../samples/z380/mandelbrot.lst)           |
| TMS320C15   |   5.4 |   0.220 |   0.070 |                    | [mandelbrot.cast](../samples/tms320c1x/mandelbrot.cast)                  | [tms320c1x/mandelbrot.lst](../samples/tms320c1x/mandelbrot.lst) |
| Z280        |   5.7 |   0.217 |   0.060 |                    | [mandelbrot.cast](../samples/z280/mandelbrot.cast)                       | [z280/mandelbrot.lst](../samples/z280/mandelbrot.lst)           |
| TMS99105    |   7.4 |   0.300 |   0.097 |                    | [mandelbrot.cast](../samples/tms99105/mandelbrot.cast)                   | [tms99105/mandelbrot.lst](../samples/tms99105/mandelbrot.lst)   |
| TMS99105    |   7.4 |   0.302 |   0.097 |                    | [mandelbrot_tms99105.cast](../samples/tms9995/mandelbrot_tms99105.cast)  | [tms9995/mandelbrot.lst](../samples/tms9995/mandelbrot.lst)     |
| TMS9995     |   9.2 |   0.375 |   0.119 |                    | [mandelbrot.cast](../samples/tms9995/mandelbrot.cast)                    | [tms9995/mandelbrot.lst](../samples/tms9995/mandelbrot.lst)     |
| TMS99105    |   9.3 |   0.381 |   0.124 |                    | [mandelbrot_tms99105.cast](../samples/tms9900/mandelbrot_tms99105.cast)  | [tms9900/mandelbrot.lst](../samples/tms9900/mandelbrot.lst)     |
| MN1613      |   9.5 |   0.388 |   0.128 |                    | [mandelbrot.cast](../samples/mn1613/mandelbrot.cast)                     | [mn1613/mandelbrot.lst](../samples/mn1613/mandelbrot.lst)       |
| HD6309      |   9.8 |   0.401 |   0.128 |                    | [mandelbrot.cast](../samples/hd6309/mandelbrot.cast)                     | [hd6309/mandelbrot.lst](../samples/hd6309/mandelbrot.lst)       |
| HD6309E     |   9.9 |   0.402 |   0.128 |                    | [mandelbrot_hd6309e.cast](../samples/hd6309/mandelbrot_hd6309e.cast)     | [hd6309/mandelbrot.lst](../samples/hd6309/mandelbrot.lst)       |
| TMP90C802   |  14.1 |   0.576 |   0.191 |                    | [mandelbrot.cast](../samples/tlcs90/mandelbrot.cast)                     | [tlcs90/mandelbrot.lst](../samples/tlcs90/mandelbrot.lst)       |
| TMS9995     |  14.5 |   0.591 |   0.192 |                    | [mandelbrot_tms9995.cast](../samples/tms9900/mandelbrot_tms9995.cast)    | [tms9900/mandelbrot.lst](../samples/tms9900/mandelbrot.lst)     |
| MN1613      |  16.5 |   0.671 |   0.177 | MN1610 code        | [mandelbrot.cast](../samples/mn1610/mandelbrot.cast)                     | [mn1610/mandelbrot.lst](../samples/mn1610/mandelbrot.lst)       |
| Z88C00      |  20.5 |   0.837 |   0.278 |                    | [mandelbrot.cast](../samples/z88/mandelbrot.cast)                        | [z88/mandelbrot.lst](../samples/z88/mandelbrot.lst)             |
| MC68HC08AZ0 |  25.2 |   1.029 |   0.343 |                    | [mandelbrot.cast](../samples/mc68hc08/mandelbrot.cast)                   | [mc68hc08/mandelbrot.lst](../samples/mc68hc08/mandelbrot.lst)   |
| 68HC11      |  28.1 |   1.147 |   0.385 |                    | [mandelbrot.cast](../samples/mc68hc11/mandelbrot.cast)                   | [mc68hc11/mandelbrot.lst](../samples/mc68hc11/mandelbrot.lst)   |
| TMS9900     |  29.0 |   1.184 |   0.387 |                    | [mandelbrot.cast](../samples/tms9900/mandelbrot.cast)                    | [tms9900/mandelbrot.lst](../samples/tms9900/mandelbrot.lst)     |
| TMS370Cx5x  |  33.8 |   1.382 |   0.457 |                    | [mandelbrot.cast](../samples/tms370/mandelbrot.cast)                     | [tms370/mandelbrot.lst](../samples/tms370/mandelbrot.lst)       |
| Z380        |  34.3 |   1.403 |   0.480 | Z180 code          | [mandelbrot_z380.cast](../samples/z180/mandelbrot_z380.cast)             | [z180/mandelbrot.lst](../samples/z180/mandelbrot.lst)           |
| P8051       |  35.0 |   1.428 |   0.442 |                    | [mandelbrot.cast](../samples/i8051/mandelbrot.cast)                      | [i8051/mandelbrot.lst](../samples/i8051/mandelbrot.lst)         |
| TMS9980     |  37.4 |   1.528 |   0.497 |                    | [mandelbrot_tms9980.cast](../samples/tms9900/mandelbrot_tms9980.cast)    | [tms9900/mandelbrot.lst](../samples/tms9900/mandelbrot.lst)     |
| Z380        |  38.2 |   1.562 |   0.510 | Z80 code           | [mandelbrot_z380.cast](../samples/z80/mandelbrot_z380.cast)              | [z80/mandelbrot.lst](../samples/z80/mandelbrot.lst)             |
| HD6301      |  39.0 |   1.589 |   0.450 |                    | [mandelbrot_hd6301.cast](../samples/mc6801/mandelbrot_hd6301.cast)       | [mc6801/mandelbrot.lst](../samples/mc6801/mandelbrot.lst)       |
| KL5C80A12   |  39.1 |   1.595 |   0.520 | Z80 code           | [mandelbrot_kl5c80.cast](../samples/z80/mandelbrot_kl5c80.cast)          | [z80/mandelbrot.lst](../samples/z80/mandelbrot.lst)             |
| P8085       |  40.2 |   1.632 |   0.393 |                    | [mandelbrot_p8085.cast](../samples/i8080/mandelbrot_p8085.cast)          | [i8080/mandelbrot.lst](../samples/i8080/mandelbrot.lst)         |
| W65C816S    |  40.9 |   1.661 |   0.409 |                    | [mandelbrot.cast](../samples/w65c816/mandelbrot.cast)                    | [w65c816/mandelbrot.lst](../samples/w65c816/mandelbrot.lst)     |
| MC6801      |  43.2 |   1.759 |   0.493 |                    | [mandelbrot.cast](../samples/mc6801/mandelbrot.cast)                     | [mc6801/mandelbrot.lst](../samples/mc6801/mandelbrot.lst)       |
| HD6309E     |  46.9 |   1.910 |   0.541 | native mode        | [mandelbrot_hd6309e.cast](../samples/mc6809/mandelbrot_hd6309e.cast)     | [mc6809/mandelbrot.lst](../samples/mc6809/mandelbrot.lst)       |
| MC68HC08AZ0 |  48.9 |   1.991 |   0.579 | MC68HC05 code      | [mandelbrot_mc68hc08.cast](../samples/mc68hc05/mandelbrot_mc68hc08.cast) | [mc68hc05/mandelbrot.lst](../samples/mc68hc05/mandelbrot.lst)   |
| HD6309      |  49.7 |   2.023 |   0.573 | native mode        | [mandelbrot_hd6309.cast](../samples/mc6809/mandelbrot_hd6309.cast)       | [mc6809/mandelbrot.lst](../samples/mc6809/mandelbrot.lst)       |
| MC68HC08AZ0 |  53.7 |   2.185 |   0.582 | MC6805 code        | [mandelbrot_mc68hc08.cast](../samples/mc6805/mandelbrot_mc68hc08.cast)   | [mc6805/mandelbrot.lst](../samples/mc6805/mandelbrot.lst)       |
| MC68HC05C0  |  54.5 |   2.219 |   0.654 |                    | [mandelbrot.cast](../samples/mc68hc05/mandelbrot.cast)                   | [mc68hc05/mandelbrot.lst](../samples/mc68hc05/mandelbrot.lst)   |
| MC6809E     |  55.3 |   2.254 |   0.638 |                    | [mandelbrot_mc6809e.cast](../samples/mc6809/mandelbrot_mc6809e.cast)     | [mc6809/mandelbrot.lst](../samples/mc6809/mandelbrot.lst)       |
| Z86C91      |  55.9 |   2.284 |   0.741 |                    | [mandelbrot.cast](../samples/z86/mandelbrot.cast)                        | [z86/mandelbrot.lst](../samples/z86/mandelbrot.lst)             |
| INS8070     |  56.6 |   2.312 |   0.744 |                    | [mandelbrot.cast](../samples/ins8070/mandelbrot.cast)                    | [ins8070/mandelbrot.lst](../samples/ins8070/mandelbrot.lst)     |
| TMS7002     |  57.5 |   2.338 |   0.585 |                    | [mandelbrot.cast](../samples/tms7000/mandelbrot.cast)                    | [tms7000/mandelbrot.lst](../samples/tms7000/mandelbrot.lst)     |
| TMS70C02    |  57.5 |   2.338 |   0.585 |                    | [mandelbrot_tms70c02.cast](../samples/tms7000/mandelbrot_tms70c02.cast)  | [tms7000/mandelbrot.lst](../samples/tms7000/mandelbrot.lst)     |
| MC68HC05C0  |  58.1 |   2.366 |   0.638 | MC6805 code        | [mandelbrot_mc68hc05.cast](../samples/mc6805/mandelbrot_mc68hc05.cast)   | [mc6805/mandelbrot.lst](../samples/mc6805/mandelbrot.lst)       |
| MC6809      |  58.8 |   2.395 |   0.678 |                    | [mandelbrot.cast](../samples/mc6809/mandelbrot.cast)                     | [mc6809/mandelbrot.lst](../samples/mc6809/mandelbrot.lst)       |
| Z180        |  62.5 |   2.554 |   0.872 |                    | [mandelbrot.cast](../samples/z180/mandelbrot.cast)                       | [z180/mandelbrot.lst](../samples/z180/mandelbrot.lst)           |
| P8080       |  63.2 |   2.566 |   0.611 |                    | [mandelbrot.cast](../samples/i8080/mandelbrot.cast)                      | [i8080/mandelbrot.lst](../samples/i8080/mandelbrot.lst)         |
| MC6802      |  66.3 |   2.696 |   0.692 |                    | [mandelbrot_mc6802.cast](../samples/mc6800/mandelbrot_mc6802.cast)       | [mc6800/mandelbrot.lst](../samples/mc6800/mandelbrot.lst)       |
| MB8870      |  66.3 |   2.696 |   0.692 |                    | [mandelbrot_mb8870.cast](../samples/mc6800/mandelbrot_mb8870.cast)       | [mc6800/mandelbrot.lst](../samples/mc6800/mandelbrot.lst)       |
| Z80         |  67.2 |   2.746 |   0.897 |                    | [mandelbrot.cast](../samples/z80/mandelbrot.cast)                        | [z80/mandelbrot.lst](../samples/z80/mandelbrot.lst)             |
| MC6800      |  68.0 |   2.765 |   0.710 |                    | [mandelbrot.cast](../samples/mc6800/mandelbrot.cast)                     | [mc6800/mandelbrot.lst](../samples/mc6800/mandelbrot.lst)       |
| MB8861      |  68.0 |   2.765 |   0.710 |                    | [mandelbrot_mb8861.cast](../samples/mc6800/mandelbrot_mb8861.cast)       | [mc6800/mandelbrot.lst](../samples/mc6800/mandelbrot.lst)       |
| HD64180S    |  68.8 |   2.812 |   0.959 | Z180 code          | [mandelbrot_hd64180s.cast](../samples/z180/mandelbrot_hd64180s.cast)     | [z180/mandelbrot.lst](../samples/z180/mandelbrot.lst)           |
| Z280        |  73.2 |   2.990 |   0.974 | Z80 code           | [mandelbrot_z280.cast](../samples/z80/mandelbrot_z280.cast)              | [z80/mandelbrot.lst](../samples/z80/mandelbrot.lst)             |
| P8039       |  76.2 |   3.099 |   0.832 |                    | [mandelbrot.cast](../samples/i8048/mandelbrot.cast)                      | [i8048/mandelbrot.lst](../samples/i8048/mandelbrot.lst)         |
| HD64180S    |  76.4 |   3.122 |   1.017 | Z80 code           | [mandelbrot_hd64180s.cast](../samples/z80/mandelbrot_hd64180s.cast)      | [z80/mandelbrot.lst](../samples/z80/mandelbrot.lst)             |
| Z180        |  76.9 |   3.140 |   1.023 | Z80 code           | [mandelbrot_z180.cast](../samples/z80/mandelbrot_z180.cast)              | [z80/mandelbrot.lst](../samples/z80/mandelbrot.lst)             |
| HD6120      |  78.2 |   3.171 |   0.736 |                    | [mandelbrot_hd6120.cast](../samples/pdp8/mandelbrot_hd6120.cast)         | [pdp8/mandelbrot.lst](../samples/pdp8/mandelbrot.lst)           |
| MC146805E2  |  78.6 |   3.199 |   0.863 |                    | [mandelbrot.cast](../samples/mc6805/mandelbrot.cast)                     | [mc6805/mandelbrot.lst](../samples/mc6805/mandelbrot.lst)       |
| MOS6502     |  85.2 |   3.466 |   0.914 |                    | [mandelbrot.cast](../samples/mos6502/mandelbrot.cast)                    | [mos6502/mandelbrot.lst](../samples/mos6502/mandelbrot.lst)     |
| G65SC02     |  85.3 |   3.471 |   0.915 |                    | [mandelbrot_g65sc02.cast](../samples/mos6502/mandelbrot_g65sc02.cast)    | [mos6502/mandelbrot.lst](../samples/mos6502/mandelbrot.lst)     |
| R65C02      |  85.3 |   3.471 |   0.915 |                    | [mandelbrot_r65c02.cast](../samples/mos6502/mandelbrot_r65c02.cast)      | [mos6502/mandelbrot.lst](../samples/mos6502/mandelbrot.lst)     |
| W65C02S     |  85.3 |   3.471 |   0.915 |                    | [mandelbrot_w65c02s.cast](../samples/mos6502/mandelbrot_w65c02s.cast)    | [mos6502/mandelbrot.lst](../samples/mos6502/mandelbrot.lst)     |
| NSC800      |  85.8 |   3.503 |   1.144 |                    | [mandelbrot_nsc800.cast](../samples/z80/mandelbrot_nsc800.cast)          | [z80/mandelbrot.lst](../samples/z80/mandelbrot.lst)             |
| W65C816S    |  87.3 |   3.552 |   0.936 | MOS6502 code       | [mandelbrot_w65c816.cast](../samples/mos6502/mandelbrot_w65c816.cast)    | [mos6502/mandelbrot.lst](../samples/mos6502/mandelbrot.lst)     |
| CDP1804A    | 112.7 |   4.583 |   1.173 |                    | [mandelbrot.cast](../samples/cdp1804a/mandelbrot.cast)                   | [cdp1804a/mandelbrot.lst](../samples/cdp1804a/mandelbrot.lst)   |
| IM6100      | 117.7 |   4.773 |   1.100 |                    | [mandelbrot.cast](../samples/pdp8/mandelbrot.cast)                       | [pdp8/mandelbrot.lst](../samples/pdp8/mandelbrot.lst)           |
| CDP1802     | 165.7 |   6.749 |   1.903 |                    | [mandelbrot.cast](../samples/cdp1802/mandelbrot.cast)                    | [cdp1802/mandelbrot.lst](../samples/cdp1802/mandelbrot.lst)     |
| SCN2650     | 170.7 |   6.929 |   1.644 |                    | [mandelbrot.cast](../samples/scn2650/mandelbrot.cast)                    | [scn2650/mandelbrot.lst](../samples/scn2650/mandelbrot.lst)     |
| F3850       | 249.9 |  10.195 |   3.143 |                    | [mandelbrot.cast](../samples/f3850/mandelbrot.cast)                      | [f3850/mandelbrot.lst](../samples/f3850/mandelbrot.lst)         |
| INS8060     | 379.5 |  15.405 |   3.671 |                    | [mandelbrot.cast](../samples/ins8060/mandelbrot.cast)                    | [ins8060/mandelbrot.lst](../samples/ins8060/mandelbrot.lst)     |

## Floating point, fmandel

| CPU         | frame | avg row | std dev | Note               | Cast                                                                     | Listing                                                         |
| ----------- | ----: | ------: | ------: | ------------------ | ------------------------------------------------------------------------ | --------------------------------------------------------------- |
| MN1613      |   8.1 |   0.334 |   0.129 |                    | [fmandel.cast](../samples/mn1613/fmandel.cast)                           | [mn1613/fmandel.lst](../samples/mn1613/fmandel.lst)             |
| TMS99110    |  44.9 |   1.846 |   0.629 |                    | [fmandel.cast](../samples/tms99110/fmandel.cast)                         | [tms99110/fmandel.lst](../samples/tms99110/fmandel.lst)         |

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
| [mc6805](../samples/mc6805/mandelbrot.lst) | 78.6 | 58.1 | 53.7 |
| [mc68hc05](../samples/mc68hc05/mandelbrot.lst) | | 54.5 | 48.9 |
| [mc68hc08](../samples/mc68hc08/mandelbrot.lst) | | | 25.2 |
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
| [mc6809](../samples/mc6809/mandelbrot.lst) | 58.8 | 49.7 |
| [hd6309](../samples/hd6309/mandelbrot.lst) | | 9.8 |

The HD6309 runs MC6809 code as is, and in native mode, set from the
debugger for this run, most instructions take fewer cycles. Its own code
is faster still, with `MULD`, `DIVQ` and the 16-bit W register.

### MCS-96 family

| | P8095BH | N8097BH |
|---|---:|---:|
| [i8096](../samples/i8096/mandelbrot.lst) | 3.5 | 3.4 |
| Data bus | 16-bit, multiplexed | 16-bit, multiplexed |

Both chips run on a 16-bit data bus, with ALE marking the address. A word
moves in one bus cycle, so the code runs faster than on an 8-bit bus.
Every read takes a whole word, so the USART's registers sit a word
apart; otherwise a status read would also read the received data and
lose it.

### MN1610 family

| | MN1613 |
|---|---:|
| [mn1610](../samples/mn1610/mandelbrot.lst) | 16.5 |
| [mn1613](../samples/mn1613/mandelbrot.lst) | 9.5 |

The MN1613 runs MN1610 code as is. Its own code is faster because it
uses the hardware multiply `M` and divide `D` the MN1610 lacks.

### PDP-8 family

| | IM6100 | HD6120 |
|---|---:|---:|
| [pdp8](../samples/pdp8/mandelbrot.lst) | 117.7 | 78.2 |
| Data bus | 12-bit, multiplexed | 12-bit, multiplexed |

The HD6120 runs the IM6100's code as is, and in less time on its board.
The code uses none of the HD6120's own instructions, such as its stacks
(`PPC1`/`RTN1`).

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

### 8080 and Z80 family

| | P8080 | P8085 | Z80 | Z180 | Z280 | Z380 | NSC800 | HD64180S | KL5C80A12 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [i8080](../samples/i8080/mandelbrot.lst) | 63.2 | 40.2 | 32.6 | | | | 43.2 | | |
| [z80](../samples/z80/mandelbrot.lst) | | | 67.2 | 76.9 | 73.2 | 38.2 | 85.8 | 76.4 | 39.1 |
| [z180](../samples/z180/mandelbrot.lst) | | | | 62.5 | | 34.3 | | 68.8 | |
| [z280](../samples/z280/mandelbrot.lst) | | | | | 5.7 | | | | |
| [z380](../samples/z380/mandelbrot.lst) | | | | | | 3.7 | | | |
| Data bus | 8-bit | 8-bit, multiplexed | 8-bit | 8-bit | 16-bit, multiplexed | 16-bit | 8-bit, multiplexed | 8-bit | 8-bit |

Every chip runs with its cache off, as reset leaves it. The Z80 sample
reaches its USART through `IN`/`OUT (C)` with B=01H, where `IN`/`OUT (n)`
would put A on A8-A15: the Z180 and Z280 boards decode all 16 bits of an
I/O address, the Z380 board all 32, with BC's upper half clear after a
reset, and find it at 0140H, clear of the on-chip I/O; the Z80, NSC800
and KL5C80A12 boards decode A0-A7 alone and find it at 40H. The Z180's
own code, with `MLT`, takes a fifth less time than the Z80 code; the
Z280 runs the Z80 code a tenth slower than a Z80, and its own, with its
multiply and divide, thirteen times as fast as that. The Z380 runs the
Z80 code 1.8 times as fast as a Z80, the Z180 code, whose `MLT` it has
too, a tenth faster still, and its own, with `MULTW` and `DIVUW`, ten
times as fast as the Z80 code. The HD64180S runs both much as the Z180
does, its own code a tenth slower. The KL5C80A12 runs the Z80 code 1.7
times as fast as a Z80. The Z80 code takes its interrupts in mode 2, the
only one the KL5C80A12's on-chip interrupt controller serves: the code
unmasks that controller's IR0 and gives it the vector the USART gives
the other chips, so all of them reach the same handler.
