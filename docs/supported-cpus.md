# CPUs

Every processor this project targets — those already running, and the chips collected and
waiting for a board or a bus-cycle driver.

| | Meaning |
|---|---|
| ✅ | Supported. The `CPU` name is the identity string stored in the adapter board's EEPROM and printed by the boot banner as `* Bionic<identity> *`. |
| 🚧 | Planned. `Notes` gives the part number in hand. |

**Architecture** is the directory in
[libasm](https://github.com/tgtakaoka/libasm); **Debugger** is the back-end directory under
[`debugger/`](../debugger), empty where no driver exists yet. Every planned architecture
already has an assembler and disassembler in libasm — the hardware and the bus driver are
the missing half.

**Variants** are dies the firmware tells apart at runtime, so one board and one identity can
report several parts.

## All targets

<table>
<thead>
<tr><th>Architecture</th><th>Debugger</th><th></th><th>CPU</th><th>Variants</th><th>Notes</th></tr>
</thead>
<tbody>
<tr><td rowspan="5"><code>mc6800</code></td><td rowspan="5"><code>mc6800</code></td><td>✅</td><td><code>MC6800</code></td><td><code>MB8861</code></td><td></td></tr>
<tr><td>✅</td><td><code>MC6801</code></td><td><code>HD6301</code></td><td></td></tr>
<tr><td>✅</td><td><code>MC6802</code></td><td><code>MB8870</code></td><td></td></tr>
<tr><td>✅</td><td><code>MC68HC11A</code></td><td></td><td></td></tr>
<tr><td>✅</td><td><code>MC68HC11D</code></td><td></td><td></td></tr>
<tr><td rowspan="3"><code>mc6805</code></td><td rowspan="3"><code>mc6805</code></td><td>✅</td><td><code>MC146805E2</code></td><td></td><td></td></tr>
<tr><td>✅</td><td><code>MC68HC05C0</code></td><td></td><td></td></tr>
<tr><td>✅</td><td><code>MC68HC08AZ0</code></td><td></td><td>No NMI pin; stepped by monitoring every bus cycle</td></tr>
<tr><td rowspan="2"><code>mc6809</code></td><td rowspan="2"><code>mc6809</code></td><td>✅</td><td><code>MC6809</code></td><td><code>HD6309</code></td><td></td></tr>
<tr><td>✅</td><td><code>MC6809E</code></td><td><code>HD6309E</code></td><td></td></tr>
<tr><td rowspan="7"><code>mc68000</code></td><td rowspan="7"></td><td>🚧</td><td><code>MC68008</code></td><td></td><td><code>MC68008P10</code>, <code>MC68008FN10</code></td></tr>
<tr><td>🚧</td><td><code>MC68000</code></td><td></td><td><code>MC68HC000P10/P16/FN16/FN20</code>, <code>HD68HC000PS10</code></td></tr>
<tr><td>🚧</td><td><code>MC68HC001</code></td><td></td><td><code>MC68HC001FN10</code>, <code>MC68HC001FN16</code></td></tr>
<tr><td>🚧</td><td><code>MC68010</code></td><td></td><td><code>MC68010P12</code>, <code>MC68010FN12</code></td></tr>
<tr><td>🚧</td><td><code>MC68020</code></td><td></td><td><code>MC68020FC16E</code></td></tr>
<tr><td>🚧</td><td><code>MC68030</code></td><td></td><td><code>MC68030FE25C</code>, <code>MC68030FE33</code></td></tr>
<tr><td>🚧</td><td><code>MC68040</code></td><td></td><td><code>MC68040FE33A</code>; needs a bus sizer</td></tr>
<tr><td><code>mc68hc12</code></td><td></td><td>🚧</td><td><code>MC68HC12</code></td><td></td><td><code>MC68HC912BD32CFU10</code></td></tr>
<tr><td><code>mc68hc16</code></td><td></td><td>🚧</td><td><code>MC68HC16</code></td><td></td><td><code>MC68HC16Z1PV16</code></td></tr>
<tr><td><code>mos6502</code></td><td><code>mos6502</code></td><td>✅</td><td><code>MOS6502</code></td><td><code>G65SC02</code>, <code>R65C02</code>, <code>W65C02S</code>, <code>W65C816S</code></td><td>W65C816S needs a data bus holder for the multiplexed bank address</td></tr>
<tr><td rowspan="5"><code>z80</code></td><td rowspan="5"><code>z80</code></td><td>✅</td><td><code>Z80</code></td><td></td><td></td></tr>
<tr><td>✅</td><td><code>Z180</code></td><td></td><td>PLCC and QFP boards</td></tr>
<tr><td>✅</td><td><code>HD64180S</code></td><td></td><td>Hitachi original of the Z180</td></tr>
<tr><td>✅</td><td><code>NSC800</code></td><td></td><td>Multiplexed bus held across refresh cycles</td></tr>
<tr><td>✅</td><td><code>KL5C80A12</code></td><td></td><td>Improved bus, runs zero-wait so <code>READY</code> is unusable</td></tr>
<tr><td rowspan="2"><code>z8</code></td><td rowspan="2"><code>z8</code></td><td>✅</td><td><code>Z86C91</code></td><td></td><td></td></tr>
<tr><td>✅</td><td><code>Z88C00</code></td><td></td><td>Super8; differs from Z86C91 only in clock phase</td></tr>
<tr><td rowspan="2"><code>z8000</code></td><td rowspan="2"></td><td>🚧</td><td><code>Z8001</code></td><td></td><td><code>Z0800110PSC</code>, <code>Z16C0110PSC</code></td></tr>
<tr><td>🚧</td><td><code>Z8002</code></td><td></td><td><code>Z0800210PSC</code>, <code>Z16C0210PSC</code></td></tr>
<tr><td><code>z280</code></td><td></td><td>🚧</td><td><code>Z280</code></td><td></td><td><code>Z8028012VSC</code>; 16-bit bus and cache make timing unlike the Z180</td></tr>
<tr><td><code>z380</code></td><td></td><td>🚧</td><td><code>Z380</code></td><td></td><td><code>Z8038018FSC</code></td></tr>
<tr><td><code>i8048</code></td><td><code>i8048</code></td><td>✅</td><td><code>P8039</code></td><td><code>MSM80C39</code></td><td>No absolute addressing, subtract or compare</td></tr>
<tr><td><code>i8051</code></td><td><code>i8051</code></td><td>✅</td><td><code>P8051</code></td><td></td><td>Clock-to-control-signal delay is about half a cycle</td></tr>
<tr><td rowspan="2"><code>i8080</code></td><td rowspan="2"><code>i8080</code></td><td>✅</td><td><code>P8080</code></td><td></td><td>Non-overlapping two-phase 12 V clock; <code>RESET</code> resumes from a HALT breakpoint</td></tr>
<tr><td>✅</td><td><code>P8085</code></td><td></td><td></td></tr>
<tr><td rowspan="3"><code>i8096</code></td><td><code>i8096</code></td><td>✅</td><td><code>P8095BH</code></td><td></td><td>Prefetches up to 4 bytes, so stepping uses TRAP</td></tr>
<tr><td rowspan="2"></td><td>🚧</td><td><code>8097</code></td><td></td><td><code>N8097BH</code></td></tr>
<tr><td>🚧</td><td><code>80C196KC</code></td><td></td><td><code>S80C196KC20</code></td></tr>
<tr><td rowspan="8"><code>i8086</code></td><td rowspan="8"></td><td>🚧</td><td><code>8086</code></td><td></td><td><code>P80C86A-2</code></td></tr>
<tr><td>🚧</td><td><code>8088</code></td><td></td><td><code>P80C88A-2</code></td></tr>
<tr><td>🚧</td><td><code>V30</code></td><td></td><td><code>D70116C-8</code></td></tr>
<tr><td>🚧</td><td><code>V20</code></td><td></td><td><code>D70108HCZ-16</code></td></tr>
<tr><td>🚧</td><td><code>80186</code></td><td></td><td><code>N80C186XL20</code></td></tr>
<tr><td>🚧</td><td><code>80286</code></td><td></td><td><code>N80C286-12</code></td></tr>
<tr><td>🚧</td><td><code>80386SX</code></td><td></td><td><code>NG80386SX-25</code></td></tr>
<tr><td>🚧</td><td><code>80486DX4</code></td><td></td><td><code>FC80486DX4-75</code>; 3.3 V core with 5 V-tolerant I/O</td></tr>
<tr><td rowspan="5"><code>tms9900</code></td><td rowspan="5"><code>tms9900</code></td><td>✅</td><td><code>TMS9900</code></td><td></td><td>Needs a non-overlapping four-phase 12 V clock</td></tr>
<tr><td>✅</td><td><code>TMS9980</code></td><td></td><td></td></tr>
<tr><td>✅</td><td><code>TMS9981</code></td><td></td><td>8-bit bus; registers live in main memory, so cycle counts are high</td></tr>
<tr><td>✅</td><td><code>TMS9995</code></td><td></td><td>Optimised bus, 16-bit internal RAM</td></tr>
<tr><td>✅</td><td><code>TMS99105</code></td><td><code>TMS99110</code></td><td>A TMS99110 is recognised by its Macrostore ROM</td></tr>
<tr><td><code>tms7000</code></td><td><code>tms7000</code></td><td>✅</td><td><code>TMS7000</code></td><td><code>TMS7002</code></td><td>Highly orthogonal instruction set</td></tr>
<tr><td><code>tms370</code></td><td><code>tms370</code></td><td>✅</td><td><code>TMS370Cx5x</code></td><td></td><td>Oscillator monitor resets the CPU if the clock looks unstable</td></tr>
<tr><td rowspan="4"><code>tms320</code></td><td><code>tms320</code></td><td>✅</td><td><code>TMS320C15</code></td><td></td><td>So few bus signals that instructions are tracked by counting cycles</td></tr>
<tr><td rowspan="3"></td><td>🚧</td><td><code>TMS320C25</code></td><td></td><td><code>TMS320C25FNL</code></td></tr>
<tr><td>🚧</td><td><code>TMS320C51</code></td><td></td><td><code>TMS320C51PQ</code></td></tr>
<tr><td>🚧</td><td><code>TMS320C209</code></td><td></td><td><code>TMS320C209PN</code></td></tr>
<tr><td><code>tms320f</code></td><td></td><td>🚧</td><td><code>TMS320C32</code></td><td></td><td><code>TMS320C32PCM40</code>; floating point</td></tr>
<tr><td rowspan="2"><code>pdp8</code></td><td rowspan="2"><code>pdp8</code></td><td>✅</td><td><code>IM6100</code></td><td></td><td>Short address-valid window makes tuning hard</td></tr>
<tr><td>✅</td><td><code>HD6120</code></td><td></td><td>Needs no dummy read, so faster than IM6100</td></tr>
<tr><td rowspan="2"><code>pdp11</code></td><td rowspan="2"></td><td>🚧</td><td><code>T-11</code></td><td></td><td><code>DCT11</code></td></tr>
<tr><td>🚧</td><td><code>J-11</code></td><td></td><td><code>DCJ11</code></td></tr>
<tr><td><code>cdp1802</code></td><td><code>cdp1802</code></td><td>✅</td><td><code>CDP1802</code></td><td><code>CDP1804A</code></td><td></td></tr>
<tr><td><code>f3850</code></td><td><code>f3850</code></td><td>✅</td><td><code>F3850</code></td><td></td><td>Fairchild F8</td></tr>
<tr><td><code>ins8060</code></td><td><code>ins8060</code></td><td>✅</td><td><code>INS8060</code></td><td></td><td>SC/MP II</td></tr>
<tr><td><code>ins8070</code></td><td><code>ins8070</code></td><td>✅</td><td><code>INS8070</code></td><td></td><td>SC/MP III</td></tr>
<tr><td><code>scn2650</code></td><td><code>scn2650</code></td><td>✅</td><td><code>SCN2650</code></td><td></td><td></td></tr>
<tr><td><code>ns32000</code></td><td></td><td>🚧</td><td><code>NS32016</code></td><td></td><td><code>NS32016D-10</code>, <code>NS32016N10</code></td></tr>
<tr><td><code>tlcs90</code></td><td><code>tlcs90</code></td><td>✅</td><td><code>TMP90C802</code></td><td></td><td></td></tr>
<tr><td rowspan="4"><code>tlcs900</code></td><td rowspan="4"></td><td>🚧</td><td><code>TLCS-900</code></td><td></td><td><code>TMP96C041AF</code></td></tr>
<tr><td>🚧</td><td><code>TLCS-900/H</code></td><td></td><td><code>TMP95C061BF</code></td></tr>
<tr><td>🚧</td><td><code>TLCS-900/H2</code></td><td></td><td><code>TMP94C251AFG</code></td></tr>
<tr><td>🚧</td><td><code>TLCS-900/L</code></td><td></td><td><code>TMP93CS32FG</code></td></tr>
<tr><td rowspan="3"><code>h8300</code></td><td rowspan="3"></td><td>🚧</td><td><code>H8/300H</code></td><td></td><td><code>HD6413002F16</code>, <code>HD6433048F16</code></td></tr>
<tr><td>🚧</td><td><code>H8S/2000</code></td><td></td><td><code>HD64F2148FA20V</code></td></tr>
<tr><td>🚧</td><td><code>H8S/2600</code></td><td></td><td><code>HD64F2633F25V</code></td></tr>
<tr><td><code>h8500</code></td><td></td><td>🚧</td><td><code>H8/500</code></td><td></td><td><code>HD6415108F10V</code>, <code>HD6435368TFW35</code></td></tr>
<tr><td><code>h16</code></td><td></td><td>🚧</td><td><code>H16</code></td><td></td><td><code>HD641016CP10</code></td></tr>
<tr><td rowspan="2"><code>superh</code></td><td rowspan="2"></td><td>🚧</td><td><code>SH-1</code></td><td></td><td><code>HD6417034F20V</code>, <code>HD6437034F20</code></td></tr>
<tr><td>🚧</td><td><code>SH-2</code></td><td></td><td><code>HD6477042F28</code>, <code>HD64F7047F50V</code></td></tr>
<tr><td><code>mn1610</code></td><td><code>mn1610</code></td><td>✅</td><td><code>MN1613</code></td><td><code>MN1613A</code></td><td>Has floating-point instructions</td></tr>
<tr><td><code>cp1600</code></td><td></td><td>🚧</td><td><code>CP1610</code></td><td></td><td><code>CP1610</code>; needs +11 V and −3 V and a non-overlapping two-phase +11 V clock</td></tr>
<tr><td><code>am29000</code></td><td></td><td>🚧</td><td><code>Am29205</code></td><td></td><td><code>AM29205-16KC</code>; external 16-bit bus variant of Am29k</td></tr>
</tbody>
</table>

## Coprocessors

Floating-point and support chips collected alongside the CPUs:

| Coprocessor | Role | Pairs with |
|---|---|---|
| `8087` | FPU | `8086` / `8088` |
| `80287` | FPU | `80286` |
| `80387SX` | FPU | `80386SX` |
| `80C187` | FPU | `80186` |
| `MC68881` | FPU | `MC68020`, `MC68030` |
| `MC68882` | FPU | `MC68020`, `MC68030` |
| `NS32081` | FPU | `NS32016` |
| `MC68150` | Dynamic bus sizer | `MC68040`, to run it on a 16-bit bus |

## The limits

The base board supplies 5 V and at most a 16-bit data bus, which sets the practical ceiling.
Parts up to the MC68030 and 80386 fit within it; the MC68040 and 80486 need help — an
`MC68150` bus sizer in the 68k case, and in the Intel case an `i486DX4`, whose core runs at
3.3 V with 5 V-tolerant I/O and which can size its own bus.

The 5 V rail is a hard limit, not just a convention, and it is why the 3.3 V `MC68040FE25V`
in the inventory is not listed above at all. Bringing the low-voltage parts into range —
Toshiba's `TMP92CM22` and `TMP91C829`, most of the SuperH line, and that MC68040 — needs a
second base board without the level shifters. See
[A 3.3 V base board](3v3-base.md).

## Comparing CPUs

Every target gets the same [Mandelbrot program](samples.md) ported to its assembly language,
which makes it a rough cross-architecture benchmark. Chips with hardware multiply and divide
(TMS9995, TMS99105, MC68HC08AZ0, P8095BH, TMS370) finish dramatically faster than those
without (MCS-48, CDP1802).
