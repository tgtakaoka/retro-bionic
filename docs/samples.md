# Samples

[`samples/`](../samples) holds the same handful of programs ported to every supported
architecture — 43 CPU directories plus one host-side reference. Each contains the assembly
source, a listing, and a pre-built `.s19` or `.hex` ready to paste into the debugger.

## The programs

| Program | What it does |
|---|---|
| `mandelbrot` | Draws the Mandelbrot set as ASCII art. The cross-architecture benchmark; see [its timings](mandelbrot.md). |
| `fmandel` | Floating-point variant, for targets with FP instructions (MN1613, TMS99110, 8087) |
| `dmandel` | Double-precision `fmandel` (DCJ11 FP11, 8087) |
| `arith` | Arithmetic test suite — add, subtract, multiply, divide, negate, shift, and the resulting flags |
| `echo` | Serial echo, polled |
| `echoir` | Serial echo with an interrupt-driven receiver |
| `echoitr` | Serial echo with an interrupt-driven receiver *and* transmitter |

Shared assembly includes sit alongside them — `queue.inc`, `mc6850.inc`, `i8251.inc`,
`usart.inc`, plus per-CPU register definitions.

Where a family runs the same binary code, each CPU directory's `.asm` is only a harness:
the `cpu` directive, the reset entry and vectors, and setup. The program itself is a
shared include in the family's base directory, so a new variant adds harnesses, not
copies. `samples/i8086` holds the x86 programs (`mandelbrot.inc`, `arith.inc`, `echo*.inc`,
the `io.inc` console), which `samples/i8087` and later 80186-80486 harnesses include; a
shared include names no CPU and stays in the 8086 instruction set.

A few directories carry real vintage software as well:
[`samples/mc6809/ASSIST09.S19`](../samples/mc6809/ASSIST09.S19) and
[`samples/mc6809/BASIC9.S19`](../samples/mc6809/BASIC9.S19).

## CP/M 2.2 on the 8080

[`samples/i8080/cpm80bios.inc`](../samples/i8080/cpm80bios.inc) is a CP/M 2.2 BIOS in
8080 instructions: the console on the i8251 and four drives on the debugger's sector
disk, reached through port routines its harness supplies (`cpm80io.inc`).
[`cpm80.asm`](../samples/i8080/cpm80.asm) is the cold boot that `L` loads; it reads the
system from A:'s system tracks. [`scripts/cpm-disk.py`](../scripts/cpm-disk.py) builds
and reads the disks.

The disks: A: the system, an 8 MB `hd8` disk with the CP/M tools and MBASIC; B: the
user disk, an 8 MB `hd8` with `fmandel.bas`; C: and D: blank `ibm-3740` floppies
(empty files), removable through `O` and `^C`.

The CP/M binaries stay out of the repository. Unpack
[cpm2-asm.zip](http://www.cpm.z80.de/download/cpm2-asm.zip) (`CPM22.ASM`) into
`samples/i8080/cpm80/`, and put the A: files, such as those of
[cpm22-b.zip](http://www.cpm.z80.de/download/cpm22-b.zip) and `MBASIC.COM`, in
`samples/i8080/cpm80/a/` ([license](http://www.cpm.z80.de/license.html); MBASIC is
Microsoft's). Then:

```sh
cd samples/i8080
make cpm80.dsk cpm80b.dsk cpm80c.dsk cpm80d.dsk
```

copy `cpm80.hex` and the four disks into `i8080/` on the SD card, and `L i8080/cpm80.hex`,
`R`, `G` boots to `A>`; `B>A:MBASIC FMANDEL` draws the set. The Z80, NSC800 and
KL5C80 boot the same files.

The Z180, Z280 and Z380 decode 16-bit ports, so `samples/z180` has their port
routines (0140H, 0150H, `OUT (C)`); `make cpm80.dsk` there puts that BIOS on a copy of
the 8080's A:, and `L z180/cpm80.hex` boots it.

On a V20 or V30, `L i8086/cpm80.hex` loads a native harness that enters 8080
emulation at 1000:0000 with the 8080's cold boot, and mounts `i8086/cpm80.dsk`, a copy
of the 8080's (`make cpm80.dsk cpm80b.dsk` in `samples/i8086`).

**BDS C** (public domain, [bdsoft.com](https://www.bdsoft.com/resources/bdsc.html))
compiles on the 8080. Put `bdsc160/`'s files from `bdsc-all.zip` in
`samples/i8080/cpm80/a/`; its `.LBR` libraries are crunched and unpack only on a
Z80 (`LBREXT`, `UNCRUNCH`). `mandel.c` on B: is the integer mandelbrot:
`A>SUBMIT BDSC MANDEL` (`bdsc.sub` on A:: `CC`, then `CLINK`), then `A>B:MANDEL`.

## The reference implementation

[`samples/arith/`](../samples/arith) is the golden model, compiled and run on the host rather
than on a target. It holds C++ implementations of each operation (`add.cpp`, `div.cpp`,
`mul.cpp`, `neg.cpp`, `shift.cpp`, `sub.cpp`, with `flags.hpp`), the original BASIC
listings the Mandelbrot ports derive from (`mandelbrot.bas`, `fmandel.bas`), and a C version
(`mandelbrot.c`). When a port misbehaves on real silicon, this is what its output is
compared against.

## Building a sample

Each directory has a Makefile driving the `asm` tool from
[libasm](https://github.com/tgtakaoka/libasm):

```make
ASM=asm

all: echo.s19 echoir.s19 echoitr.s19 arith.s19 mandelbrot.s19 mc6x09.s19

%.s19:	%.asm
	$(ASM) -l $*.lst -o $@ $<
```

So with `asm` on your `PATH`:

```sh
cd samples/mc6809
make
```

The committed `.s19`/`.hex` files mean you do not have to build anything to try a target.

## Running one

1. Plug in the CPU board and connect to the console at 115200.
2. Press `U` to start an upload, then paste the contents of the `.s19` or `.hex` file.
3. Press `R` to reset, `r` to see the registers, `S` to step, or `G` to let it run.

The `echo` programs talk to the emulated MC6850 or i8251 — or, on microcontroller targets,
to the on-chip serial port. That traffic shares the debugger console, so whatever you type
goes to the running program and its output comes straight back at you. The second USB serial
port is not involved; it is the software HALT (see [build.md](build.md)), which is how you
stop the program afterwards.

## Why Mandelbrot

Because it is small, it is pure computation, and it exercises multiply and divide hard.
Porting it to every architecture makes the machines directly comparable: targets with
hardware multiply and divide (TMS9995, TMS99105, MC68HC08AZ0, P8095BH, TMS370) finish it
dramatically faster than those without (MCS-48, CDP1802), and the difference is visible as
the picture paints on screen.
