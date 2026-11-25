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

The CP/M binaries stay out of the repository: the first `make` downloads them
with `debugger/i8080/tools/cpm80-fetch.py` into `samples/i8080/cpm80.fetch/`, checks
each archive's sha256, and keeps the archives in its `dl/` so later builds work
offline. It fetches CP/M 2.2 from [cpm.z80.de](http://www.cpm.z80.de/download/)
([license](http://www.cpm.z80.de/license.html)), BDS C and HI-TECH C. MBASIC is
Microsoft's and isn't fetched: put your own `MBASIC.COM` in
`samples/i8080/cpm80.fetch/a/`. Every port's downloads are in a `*.fetch/` folder,
so `rsync --exclude='*.fetch/'` keeps them off the SD card.
Then:

```sh
cd samples/i8080
make cpm80.dsk cpm80b.dsk cpm80c.dsk cpm80d.dsk
```

copy `cpm80.hex` and the four disks into `i8080/` on the SD card, and `L i8080/cpm80.hex`,
`R`, `G` boots to `A>`; `B>A:MBASIC FMANDEL` draws the set. The Z80, NSC800 and
KL5C80 boot the same files.

[A demo](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/i8080/mbasic_kl5c80.cast)
on the KL5C80: `L` loads `cpm80.hex` and mounts the disks beside it, CP/M boots, and
MBASIC draws the set from `fmandel.bas` on B:.

The Z180, Z280 and Z380 decode 16-bit ports, so `samples/z180` has their port
routines (0140H, 0150H, `OUT (C)`); `make cpm80.dsk` there puts that BIOS on a copy of
the 8080's A:, and `L z180/cpm80.hex` boots it. That boot also writes 16 to the
Z380's address mask register, so its memory wraps at 64K as CP/M's programs expect.

On a V20 or V30, `L v30/cpm80.hex` loads a native harness that enters 8080
emulation at 1000:0000 with the 8080's cold boot, and mounts `v30/cpm80.dsk`, a copy
of the 8080's (`make cpm80.dsk cpm80b.dsk` in `samples/v30`).

**BDS C** (public domain, [bdsoft.com](https://www.bdsoft.com/resources/bdsc.html))
compiles on the 8080; A: has `bdsc160/`'s files from `bdsc-all.zip` and
`extra/BCD.LBR`. Its `.LBR` libraries are crunched and unpack only on a
Z80 (`LBREXT`, `UNCRUNCH`). `imandel.c` on B: is the integer mandelbrot:
`A>SUBMIT BDSC IMANDEL` (`bdsc.sub` on A:: `CC`, then `CLINK`), then `A>B:IMANDEL`.

**HI-TECH C 3.09** (free, copyright HI-TECH Software,
[agn453/HI-TECH-Z80-C](https://github.com/agn453/HI-TECH-Z80-C)) needs a Z80. A: has
the files of `htc-bin.lbr`, unpacked with `debugger/i8080/tools/lbr.py`. `fmandel.c` on B: is the floating-point mandelbrot:
`A>SUBMIT HITECHC FMANDEL` (`hitechc.sub`: `C309 -LF`), then `A>FMANDEL`; `imandel.c`
builds too. Its assembler and linker take `mandel.asm` on B:, the integer mandelbrot in
Z80 assembly: `A>ZAS -OB:MANDEL.OBJ B:MANDEL.ASM`, `A>LINQ -C100H -OMANDEL.COM
B:MANDEL.OBJ`, then `A>MANDEL`.

[A demo](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/i8080/cpm80_kl5c80.cast)
on the KL5C80: `U` uploads `cpm80.hex`, `O` mounts A: and B:, and CP/M boots; ZAS and
LINQ build `mandel.asm`, BDS C builds `imandel.c`, and each draws the set. It's also on
[asciinema.org](https://asciinema.org/a/9sj5kQMvBMyLDVnO).
[The same on the Z380](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/z180/cpm80_z380.cast),
from `z180/`, and on [asciinema.org](https://asciinema.org/a/bZJg8ghJDnk8ugUM).

## CP/M-86 on the 8086

[`samples/i8086/cpm86bios.asm`](../samples/i8086/cpm86bios.asm) is a CP/M-86 1.1 BIOS
for the P8086 and D8087 boards, on the same console and sector disk. CCP, BDOS and the
BIOS sit in segment F000H, and `L` loads all three: no system tracks are read.

DRI's binaries stay out of the repository: the first `make` downloads them with
`debugger/i8086/tools/cpm86-fetch.py` into `samples/i8086/cpm86.fetch/`, checks each
archive's sha256, and keeps the archives in `cpm86.fetch/dl/` so later builds work
offline. It fetches `CPM.SYS` and the A: tools, such as `PIP.CMD`, `STAT.CMD` and
`ED.CMD`, from [CPM-86.zip](http://www.cpm.z80.de/download/CPM-86.zip)'s `V1-1R/D2`
([license](http://www.cpm.z80.de/license.html)), for an 8 MB `hd8` A:. Then:

```sh
cd samples/i8086
make cpm86.hex cpm86.dsk
```

and copy both into an `i8086/` folder on the SD card. `L i8086/cpm86.hex`, `R`, then `G`
boots to `A>`. DDT86's breakpoints and trace stop in the debugger, which owns INT 1-3.
CP/M-86's CCP has no `SAVE`; DDT86's `W` writes back what `R` read, header and all, so
`-RB:PROG.CMD`, a patch with `S` or `A`, then `-WB:PROG.CMD` keeps a loadable `.CMD`.
`R` puts the file at its offsets, so the program's addresses are 80H further on.

For BASIC, the fetch puts DRI's CBASIC compiler, `CB86.CMD`, `CB86.OR1`-`CB86.OR3`
and `CB86.L86` from
[cbasic86.zip](http://www.cpm.z80.de/download/cbasic86.zip), on A:, and DRI's Personal
BASIC, `BASIC.CMD` from [pbasic86.zip](http://www.cpm.z80.de/download/pbasic86.zip), on B:.
`make cpm86.dsk cpm86b.dsk` makes A:, with `cb.sub`, and an `hd8` B: with
[`fmandel.bas`](../samples/i8086/fmandel.bas), the `fmandel` program in CBASIC, and
`samples/i8080/fmandel.bas` as `fmandel.pb` for Personal BASIC. On B:, `BASIC`, then
`OLD FMANDEL.PB` and `RUN`, draws the set. CB86 reads its overlays from the current
drive, so `cb.sub` compiles and links from A:, then runs it on B::
`A>SUBMIT CB FMANDEL`.

For C, A: has Digital Research C 1.11 from
[drc_86.zip](http://www.cpm.z80.de/download/drc_86.zip):
`DISK1`'s `DRC.CMD`, `DRC860.CMD`-`DRC862.CMD`, `DRCRPP.CMD`, `R.CMD`, which runs its
passes, `DRC.ERR` and `.H` headers, `DISK2`'s `CLEARS.L86`, and `DISK3`'s `LINK86.CMD`,
which CBASIC uses too, and `RASM86.CMD`, with `cc.sub`. B: then also holds
[`fmandel.c`](../samples/i8086/fmandel.c) and [`imandel.c`](../samples/i8086/imandel.c),
which adds the `stdio.h` DRI's `putchar` needs to `samples/arith/imandel.c`, on B: as
`imandel.inc`; `cc.sub` compiles and links from A: too: `A>SUBMIT CC FMANDEL`. DRC 1.32, the one with 8087 code,
needs a CP/M-86 whose loader applies its programs' fixups, which 1.1's does not.

For the 8087 on the D8087 board, B: has [`fmandel.a86`](../samples/i8086/fmandel.a86),
the `fmandel` of `samples/i8087` in DRI's ASM86 with its `8087.LIB` from `CPM-86.zip`,
which ASM86 reads from the source's drive: `A>ASM86 B:FMANDEL`, `A>GENCMD B:FMANDEL`, then
`A>B:FMANDEL` draws the set in seconds.
[A demo](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/i8086/cpm86_80c86_8087.cast)
on the P80C86A-2 and D8087: `U` uploads `cpm86.hex`, `O` mounts A: and B:, and CP/M-86
boots; ASM86 builds `fmandel.a86` and DRI's C builds `imandel.c`, and each draws the set.
It's also on [asciinema.org](https://asciinema.org/a/26nhB7uRNsBEngo9).

## Unix V6 on the DCJ11

[`samples/dcj11/unix_v6.asm`](../samples/dcj11/unix_v6.asm) is the RK05 bootstrap: it
reads drive 0's first block into memory and runs it. `L dcj11/unix_v6.hex` loads it and
mounts `dcj11/unix_v6.dsk` beside it on the debugger's RK11 as rk0, rk1 and rk2 (see
[I/O devices](debugger.md#io-devices)). V6 also wants the KW11-L line clock and the
switch register, which the board emulates, and the bit-banged clock: at 10 MHz its
compiler misread the bus now and then.

The system is not in the repository. `debugger/pdp11/tools/v6-fetch.py` downloads
Ken Wellsch's V6 distribution tape from [TUHS](https://www.tuhs.org/Archive/Distributions/Research/Ken_Wellsch_v6/)
into `samples/dcj11/unix_v6.fetch/`, checks its sha256, and cuts the three RK05 disks
it carries, the root, `/usr/source` and `/usr/doc`, as V6's setup would; the tape
is distributed under [Caldera's license](https://www.tuhs.org/Archive/Caldera-license.pdf)
for ancient UNIX. Then:

```sh
cd samples/dcj11
make unix_v6.dsk
```

puts them in the disk image's slots, and the installer does the rest over the console,
with the board at its prompt and `unix_v6.hex` in `dcj11/` on the SD card
(`scripts/bionic-control.py sd put unix_v6.hex dcj11/unix_v6.hex` puts it there):

```sh
../../debugger/pdp11/tools/v6-install.py --save unix_v6.installed.dsk unix_v6.dsk \
    v6/fmandel.s v6/mandelbrot.s ../arith/fmandel.c ../arith/dmandel.c
```

It syncs the image onto the card with `Y`, boots the tape's `rkunix` single-user,
builds `/unix` for the 11/40 with the `rk`, `rp` and `tm` drivers, makes `/dev`'s
nodes, boots that kernel, adds the mounts of rk1 and rk2 to `/etc/rc`, types the
files named into `/usr/mandel`, checks the root with `icheck` and `dcheck`, and
reads the installed disk back to `--save`'s file. About seven minutes, as V6's
console takes a keystroke every 50 ms. A run that stops starts over from the image.

`L dcj11/unix_v6.hex`, `R`, `= SP 1000` (the power-up SP may point into the I/O
page), `G`, Enter, then `unix` at the bootstrap's `@` boots single-user to `#`; `stty
-lcase` makes the console lower-case. `/usr/mandel` holds
[`fmandel.s`](../samples/dcj11/v6/fmandel.s) and
[`mandelbrot.s`](../samples/dcj11/v6/mandelbrot.s), the DCJ11 samples in V6's
assembler (`as fmandel.s; a.out`), and `samples/arith`'s `fmandel.c` and `dmandel.c`
for its C compiler (`cc -f fmandel.c; a.out` draws the set in about 24 s). The
console sends each character with a parity bit, as V6's DL11 driver does; `I DL11 177`
drops it, as the terminal of the time did.
[A demo](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/dcj11/unix_v6.cast):
`L` loads the bootstrap and mounts the disk, V6 boots single-user, and its `as` and
`cc` build `fmandel`, each drawing the set. It's also on
[asciinema.org](https://asciinema.org/a/HtOD7HEGkPyrR5U9).

## 2.11BSD on the DCJ11

[`samples/dcj11/bsd211.asm`](../samples/dcj11/bsd211.asm) is the MSCP bootstrap, what
a KDJ11's ROM does: it initializes the controller, brings unit 0 online, reads its
first block into memory and runs it. `L dcj11/bsd211.hex` loads it and mounts
`dcj11/bsd211.dsk` beside it as ra0 on the debugger's MSCP controller (see
[I/O devices](debugger.md#io-devices)). 2.11BSD wants split I/D, 22-bit addresses, an
FP11 and a megabyte or more, which the DCJ11 and its board have (the board's memory is
4088 KB), and the KW11-L line clock, which the board emulates.

The system is not in the repository. `debugger/pdp11/tools/bsd211-fetch.py` downloads
the 2.11BSD PL500 tape from [TUHS](https://www.tuhs.org/Archive/Distributions/UCB/2BSD/2.11BSD/Patches/)
into `samples/dcj11/bsd211.fetch/`, checks its sha256, and builds open-simh's PDP-11
simulator there from a pinned commit; the system is distributed under the BSD license
and [Caldera's license](https://www.tuhs.org/Archive/Caldera-license.pdf) for ancient
UNIX. `bsd211-install.py` then installs it under the simulator, as 2.11BSD's setup
document has it, on an 11/73 with an RQDX3 and an RD54 (159 MB: a root of 7.8 MB,
8.2 MB of swap, the rest `/usr` with the sources), with the GENERIC kernel, cuts
`/etc/dtab` to the MSCP controller, so that the kernel's autoconfig looks for nothing
the board has not, and puts the DCJ11 samples in `/usr/root` from a tape of its own: [`bsd211/fmandel.s`](../samples/dcj11/bsd211/fmandel.s),
V6's `fmandel.s` with the system calls as 2.11BSD's `as` and kernel want them, and
`samples/arith`'s `fmandel.c`. No tape is emulated on the board: it boots the
installed disk.

```sh
cd samples/dcj11
make bsd211.dsk
```

does both in a few minutes, and the image goes to the card over the console
(`scripts/bionic-control.py sd put bsd211.dsk dcj11/bsd211.dsk`, and `bsd211.hex`
to `dcj11/bsd211.hex` beside it).

`L dcj11/bsd211.hex`, `R`, `= SP 1000` (the power-up SP may point into the I/O
page), `I DL11 177` (the console sends each character with a parity bit, as V6's
does), `G`: the bootstrap reads block 0, rauboot prints
`73Boot from ra(0,0,0) at 0172150` and its `: ` prompt, and Enter loads
`ra(0,0,0)unix`, the GENERIC kernel, which comes up single-user at `#`. From `#`,
`^D` runs `/etc/rc` without the disk checks (the operator is taken to have done them:
`fsck -p` takes 24 s on a root left dirty) to `login:`; `root` has no password. On the
bench rauboot's banner comes a second after `G`, the kernel's at 40 s, `#` at 2 minutes
and `login:` at 3 min 20 s, with the kernel's own MMU aborts, which it handles,
reported as `?#ABORT` while the debugger is verbose. `halt` syncs the disks and
executes HALT, which the board's power-up configuration traps to 4, so the kernel ends
in `panic: trap`; the debugger's halt switch then brings the prompt back, the disks
already synced. `ra(0,0,0)unix -m`, the boot straight to multi-user, is refused by
`/etc/rc` as installed (`rc.conf` is not configured); configured, it checks the root,
which no halt left marked clean, marks it and reboots, which is that same HALT. So
multi-user is reached from `#`. Single-user, `mount /usr`
reaches `/usr/root`, where `as fmandel.s; ./a.out` and `cc -O -o fmandel fmandel.c;
./fmandel` each draw the set.
[A demo](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/dcj11/bsd211.cast):
`U` uploads the bootstrap and `O` mounts the disk, 2.11BSD boots single-user, and its
`as` and `cc` build `fmandel`, each drawing the set. It's also on
[asciinema.org](https://asciinema.org/a/0g96wFIKzY4koiQ4).

## The reference implementation

[`samples/arith/`](../samples/arith) is the golden model, compiled and run on the host rather
than on a target. It holds C++ implementations of each operation (`add.cpp`, `div.cpp`,
`mul.cpp`, `neg.cpp`, `shift.cpp`, `sub.cpp`, with `flags.hpp`), the original BASIC
listings the Mandelbrot ports derive from (`mandelbrot.bas`, `fmandel.bas`), and a C version
(`mandelbrot.c`), with `imandel.c`, the same in K&R C with 16-bit `int`s, and `fmandel.c`
and `dmandel.c`, `fmandel` in K&R C with `float` and `double`. When a port
misbehaves on real silicon, this is what its output is compared against.

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
