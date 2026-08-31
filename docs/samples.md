# Samples

[`samples/`](../samples) holds the same handful of programs ported to every supported
architecture — 38 CPU directories plus one host-side reference. Each contains the assembly
source, a listing, and a pre-built `.s19` or `.hex` ready to paste into the debugger.

## The programs

| Program | What it does |
|---|---|
| `mandelbrot` | Draws the Mandelbrot set as ASCII art. The cross-architecture benchmark. |
| `fmandel` | Floating-point variant, for targets with FP instructions (MN1613, TMS99110) |
| `arith` | Arithmetic test suite — add, subtract, multiply, divide, negate, shift, and the resulting flags |
| `echo` | Serial echo, polled |
| `echoir` | Serial echo with an interrupt-driven receiver |
| `echoitr` | Serial echo with an interrupt-driven receiver *and* transmitter |

Shared assembly includes sit alongside them — `queue.inc`, `mc6850.inc`, `i8251.inc`,
`usart.inc`, plus per-CPU register definitions.

A few directories carry real vintage software as well:
[`samples/mc6809/ASSIST09.S19`](../samples/mc6809/ASSIST09.S19) and
[`samples/mc6809/BASIC9.S19`](../samples/mc6809/BASIC9.S19).

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
	$(ASM) -l $*.lst -o $@ $^
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
