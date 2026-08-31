# Hardware

Two boards make a working system — a base and a CPU adapter. Only the adapter changes when
you swap processors. A third board, the mezzanine, is a bench aid used while bringing a new
CPU up; a finished adapter does not need it.

![Bionic base board](images/bionic-base.jpg)

## 1. Base — `schematics/base/`

*BionicBase Teensy 4.1*, the constant part of the system.

| Part | Purpose |
|---|---|
| Teensy 4.1 (MIMXRT1062DVJ6A) | Cortex-M7 at 600 MHz — generates the clock and drives every bus cycle |
| 5 × TXS0108EPW | Bidirectional 3.3 V ↔ 5 V level shifting, 8 bits each |
| 2 × SN74ACT1071DR | Holds the data bus value between cycles, until the CPU drives it |
| 48-pin ZIF socket | Accepts the mezzanine or a CPU adapter |
| HALT/RUN switch | Interrupt-driven break into a running program (`PIN_USRSW`, pin 24) |
| Power indicator LED | Sits beside the ZIF socket lever, where you cannot miss it |
| USB 2.0 Serial/PD | Power and the two serial ports |

> **The power indicator LED is placed next to the ZIF socket lever deliberately.** While it is
> lit the socket is live. Never work the lever, and never insert or remove an adapter, until
> it goes out — hot insertion is what the placement exists to prevent.

The bus holders are not a workaround for one awkward part — every target depends on them.
When the controller answers a read it drives the emulated value onto the data bus and then
lets go, and the `SN74ACT1071` holds that value there while the bus turns around, until the
CPU overwrites it on the next bus cycle. On CPUs that multiplex address and data onto the
same pins the turnaround window is far too short to flip the controller's pin direction in
software at all, so there the holders are not merely convenient but the only way the bus
keeps its value.

The Teensy's own USB port carries the debugger console. The second USB serial port is the
**software HALT** — see [build.md](build.md).

## 2. CPU adapter — 46 boards

One board per processor, and usually almost empty: the CPU, its identity EEPROM, a couple of
LEDs, decoupling, and two 48-pin connectors down to the base. Everything else is the base
board's job.

> **The LED on the adapter is a power indicator.** While it is lit the board is live. Do not
> touch the ZIF socket — and do not insert or remove an adapter — until it goes out.

Two things push parts back onto the adapter:

- **Power.** Boards needing rails other than 5 V generate them on the adapter, from a
  `dcdc+12v.kicad_sch` and/or `dcdc-5v.kicad_sch` sheet beside the board's own schematic.
  [Awkward chips](#awkward-chips) lists which parts, and what else they demand.
- **Address width.** CPUs with a large address space carry an **address bus multiplexer**,
  so more address lines than the connector has can be presented in groups. The TMS9900
  board below uses a pair of `74ACT157` for exactly this.

Some processors get two boards for two packages — `z180` exists as `bionic-z180f` (QFP) and
`bionic-z180v` (PLCC), with separate gerber sets.

![BionicTMS9900 adapter board and the TMS9900NL it carries](images/bionic-tms9900.jpg)

Above: the BionicTMS9900 board and its 64-pin TMS9900NL. Almost everything on the board
exists to feed that one chip — the `11AA010` identity EEPROM, two `74ACT157` multiplexers, a
`74ACT164`, and the regulator and DC-DC parts that produce the rails and the
non-overlapping four-phase clock it needs.

Every board has a rendered PDF schematic next to its KiCad project, e.g.
[`schematics/mc6809/bionic-mc6809.pdf`](../schematics/mc6809/bionic-mc6809.pdf).

## 3. Mezzanine — `schematics/mezzanine/`

A prototyping board, not a required layer. It carries a CPU socket, two 74HCT157
multiplexers, an 11AA010 identity EEPROM and a power indicator LED, and is used to bring a
new processor up on the bench before its own adapter is designed. Finished adapters plug
straight into the base.

> **The LED on the mezzanine is a power indicator**, same as on an adapter. While it is lit
> the board is live. Do not touch the ZIF socket, or the CPU in its socket, until it goes
> out.

![Bionic mezzanine board with a Zilog CPU in its socket](images/bionic-mezzanine.jpg)

Above, mid bring-up with a Zilog `Z0800210PSC` in the socket. Every one of the 48 connector
signals is broken out to a labelled header — `P10`–`P17`, `P20`–`P27`, `P30`–`P37`,
`P40`–`P47`, `P50`–`P57` — alongside the multiplexer inputs and outputs, so a new CPU can be
wired up by hand and probed before its adapter is laid out.

## Signal naming

The 48 lines crossing the connector are named `P10`–`P17`, `P20`–`P27`, `P30`–`P37`,
`P40`–`P47`, `P50`–`P57`. What each carries depends on the CPU's bus style, which is why
[`pinout/bionic_connector.toml`](../pinout/bionic_connector.toml) has one column per style —
8-bit multiplexed, 8-bit with 16-bit address, 16-bit multiplexed, 16-bit data:

```toml
name = "Bionic Connector"
dip = 48
#          AD8,  D8, AD16, D16
#          A16, A16,  A20, A12
 2 = "P10, AD0,  D0, AD00, D00"
23 = "P53, #RESET"
26 = "P57, #USR_LED"
27 = "P56, #USR_SW"
32 = "P46, CLOCK"
```

Each CPU board then maps its own pinout onto those names in a `*_bionic.toml` file:

```toml
name = "MC6809"
title = "MC6809/HD6309"
dip = 40
 2 = "P51, #NMI"
 8 = "P20, A0"
16 = "P30, A8"
24 = "P17, D7"
32 = "P41, R/W"
34 = "P47, E"
37 = "P53, #RESET"
```

## Board identification

Every adapter carries a Microchip **11AA010** UNI/O serial EEPROM holding a 16-byte name.
At boot the firmware bit-bangs the UNI/O protocol on pin 34 (`PIN_ID`), reads the string,
and instantiates the matching target — this is why one firmware image serves all 40 CPUs.
Holding the user switch low during reset forces the null target instead. The `W` command
writes a new identity string.

## Awkward chips

The bus-cycle timing diagrams at the top of each `debugger/*/pins_*.cpp` are the real
documentation for how a given CPU is driven. A few examples of what made them hard:

- **TMS9900** (`schematics/tms9900/`) — a non-overlapping four-phase 12 V clock had to be
  generated before anything worked at all, on top of the +12 V and −5 V rails it needs.
- **TMS9980** (`schematics/tms9980/`) and **TMS9981** (`schematics/tms9981/`) — the same
  family and the same appetite for rails: +12 V and −5 V for the TMS9980, +12 V for the
  TMS9981.
- **P8080** (`schematics/p8080/`) — non-overlapping two-phase 12 V clocks, generated on the
  adapter alongside the +12 V and −5 V rails the part needs.
- **F3850** (`schematics/f3850/`) — +12 V alongside the 5 V rail.
- **CP1610** (planned) — +11 V and −3 V rails, and a non-overlapping two-phase +11 V clock.
- **W65C816S** — a multiplexed address/data bus: in native mode the bank address appears on
  the data bus, colliding with read data, and the turnaround is one of the windows too short
  to drive in software, so it depends on the data bus holders.
- **HD6120** — during IOT sample times the CPU holds `C0`, `C1` and `SKIP` high itself, with
  a specified hold current (I<sub>OSS</sub>) of 1.6 mA to 10 mA, so a device answering an IOT
  has to sink milliamps to pull one low. The base board's `TXS0108E` level shifters manage
  620 µA of I<sub>OL</sub> at 4.5 V — the 50–70 Ω output quoted for them is a one-shot edge
  accelerator, not DC drive — so the adapter drives those three lines itself, through an
  open-collector `74LS05` (`schematics/hd6120/`, U3).
- **NSC800** — a multiplexed address/data bus too, with the low address byte sharing the
  data lines, so it leans on the holders for the same reason; the instruction is also held
  until the adjacent refresh cycle.
- **P8051** — the delay from clock edge to control signal is about 50 ns, roughly half a
  cycle at the target speed, and the external clock phase differs between HMOS and CMOS parts.
- **TMS370Cx5x** — an on-chip oscillator monitor decides a software-generated clock is
  unstable and resets the CPU.
- **P8095BH** — prefetches up to four bytes whenever the bus is free, and DIP-48 exposes
  neither NMI nor an instruction-fetch signal, so single-stepping uses TRAP instructions and
  the bus trace is reconstructed by pattern matching.
- **MC68HC08AZ0** — minimal control signals, no NMI, and instruction prefetch; every bus
  cycle is monitored to keep track.

## Manufacturing

Boards are designed in KiCad 8 and manufactured at JLCPCB as 2- and 4-layer PCBs. Gerbers
are committed next to each project, so any board here can be ordered as-is.
