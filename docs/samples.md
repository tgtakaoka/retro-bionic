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

## Operating systems

The boards below boot an operating system from disk images on the SD card. `L` of the
system's `.hex` loads it and mounts the `.dsk` images beside it on the debugger's disk
controller; `O` mounts or unmounts a single drive and lists them, and `U`, an upload,
mounts nothing, so the demos mount their disks with `O`. Images go through the
[disk cache](debugger.md#disk-cache) on a target that lends it memory.

| System | Boards | `L` | Disks | Demo |
|---|---|---|---|---|
| [CP/M 2.2](#cpm-22-on-the-8080) | P8080, P8085, Z80, NSC800, KL5C80A12 | `i8080/cpm80.hex` | A: and B:, sector disk at 50H | [KL5C80](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/i8080/cpm80_kl5c80.cast), [MBASIC](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/i8080/mbasic_kl5c80.cast) |
| CP/M 2.2 | Z180, HD64180S, Z280, Z380 | `z180/cpm80.hex` | A: and B:, sector disk at 0150H | [Z380](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/z180/cpm80_z380.cast) |
| CP/M 2.2, in 8080 emulation | P8086 with a V20 or V30 | `v30/cpm80.hex` | A: and B:, sector disk at 50H | |
| [CP/M-86 1.1](#cpm-86-on-the-8086) | P8086, D8087 | `i8086/cpm86.hex` | A: and B:, sector disk at 50H | [80C86](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/i8086/cpm86_80c86.cast), [80C86 and 8087](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/i8086/cpm86_80c86_8087.cast) |
| [CP/M-68K 1.3](#cpm-68k-on-the-68000) | MC68000P, MC68000FN, MC68008P, MC68008FN | `mc68000/cpm68k.s19` | A: and B:, sector disk at FFFFE0H | [MC68000P](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mc68000/cpm68k_mc68000p.cast) |
| [Unix V6](#unix-v6-on-the-dcj11) | DCJ11 | `dcj11/unix_v6.hex` | rk0-rk2, RK11 | [V6](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/dcj11/unix_v6.cast) |
| [2.11BSD](#211bsd-on-the-dcj11) | DCJ11 | `dcj11/bsd211.hex` | ra0, MSCP | [2.11BSD](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/dcj11/bsd211.cast) |

## CP/M 2.2 on the 8080

[`samples/i8080/cpm80bios.inc`](../samples/i8080/cpm80bios.inc) is a CP/M 2.2 BIOS in
8080 instructions: the console on the i8251 and four drives on the debugger's sector
disk, reached through port routines its harness supplies (`cpm80io.inc`).
[`cpm80.asm`](../samples/i8080/cpm80.asm) is the cold boot that `L` loads; it reads the
system from A:'s system tracks. [`scripts/cpm-disk.py`](../scripts/cpm-disk.py) builds
and reads the disks.

The disks: A: the system, an 8 MB `hd8` disk with the CP/M tools and MBASIC; B: the
user disk, an 8 MB `hd8` with `fmandel.bas`. The BIOS has C: and D: too, for
disks mounted with `O`, such as an `ibm-3740` floppy; an empty file is a blank one.

The CP/M binaries stay out of the repository: `make cpm80` downloads them
with `debugger/i8080/tools/cpm80-fetch.py` into `samples/i8080/cpm80.fetch/`, checks
each archive's sha256, and keeps the archives in its `dl/` so later builds work
offline. It fetches CP/M 2.2 from [cpm.z80.de](http://www.cpm.z80.de/download/)
([license](http://www.cpm.z80.de/license.html)), BDS C, HI-TECH C, and Microsoft's
MBASIC 5.21 from a [textfiles.com](https://discmaster.textfiles.com/) mirror of an old
CP/M archive. Every port's downloads are in a `*.fetch/` folder,
and `make distclean` removes them with the disks. Plain `make` builds the samples
and `cpm80.hex` alone, offline. Then:

```sh
cd samples/i8080
make cpm80
make cpm80-sd
```

makes `cpm80.hex`, `cpm80a.dsk` and `cpm80b.dsk`, and puts them in `i8080/` on the SD
card of the bench the session holds, once `probe` finds one of their boards at its prompt;
B: only onto a card without one, so files saved there stay (`NEWB=1` replaces it).
`L i8080/cpm80.hex`, `R`, `G` boots to `A>`; `B>A:MBASIC FMANDEL` draws the set. The 8085, Z80, NSC800 and
KL5C80 boot the same files.

[A demo](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/i8080/mbasic_kl5c80.cast)
on the KL5C80: `L` loads `cpm80.hex` and mounts the disks beside it, CP/M boots, and
MBASIC draws the set from `fmandel.bas` on B:.

The Z180, HD64180S, Z280 and Z380 decode 16-bit ports, so `samples/z180` has their port
routines (0140H, 0150H, `OUT (C)`); `make cpm80` there puts that BIOS on a copy of
the 8080's A:, and `L z180/cpm80.hex` boots it. That boot also writes 16 to the
Z380's address mask register, so its memory wraps at 64K as CP/M's programs expect.

On a V20 or V30, `L v30/cpm80.hex` loads a native harness that enters 8080
emulation at 1000:0000 with the 8080's cold boot, and mounts `v30/cpm80a.dsk`, a copy
of the 8080's (`make cpm80` in `samples/v30`).

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
make
```

makes `cpm86.hex`, `cpm86a.dsk` and `cpm86b.dsk`; copy them into an `i8086/` folder on
the SD card. `L i8086/cpm86.hex` mounts both disks beside it; `R`, then `G` boots to `A>`.
DDT86's breakpoints and trace stop in the debugger, which owns INT 1-3.
CP/M-86's CCP has no `SAVE`; DDT86's `W` writes back what `R` read, header and all, so
`-RB:PROG.CMD`, a patch with `S` or `A`, then `-WB:PROG.CMD` keeps a loadable `.CMD`.
`R` puts the file at its offsets, so the program's addresses are 80H further on.

For BASIC, the fetch puts DRI's CBASIC compiler, `CB86.CMD`, `CB86.OR1`-`CB86.OR3`
and `CB86.L86` from
[cbasic86.zip](http://www.cpm.z80.de/download/cbasic86.zip), on A:, and DRI's Personal
BASIC, `BASIC.CMD` from [pbasic86.zip](http://www.cpm.z80.de/download/pbasic86.zip), on B:.
A: also has `cb.sub`, and the `hd8` B: has
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

For C on the 8087, A: has Manx's Aztec C86 3.2d from the
[Aztec C museum](https://www.aztecmuseum.ca/compilers.htm#cpm86) in user 1, since its
headers share names with DRI's: `CC.CMD`, `AS.CMD`, `LN.CMD`, `C.LIB`, and the math
libraries `M.LIB` (software), `M87.LIB` (the 8087) and `M87S.LIB` (the 8087 if there).
B: has `fmandel.c` in user 1 too. Its compiler reads headers from user 0 unless told:
`A>USER 1`, `A>CC -I1/A: B:FMANDEL.C`, `A>LN -O B:FMANDEL.CMD B:FMANDEL.O M87.LIB C.LIB`,
then `A>B:FMANDEL` draws the set on the 8087.

In assembly, B: has [`fmandel.a86`](../samples/i8086/fmandel.a86),
the `fmandel` of `samples/i8087` in DRI's ASM86 with its `8087.LIB` from `CPM-86.zip`,
which ASM86 reads from the source's drive: `A>ASM86 B:FMANDEL`, `A>GENCMD B:FMANDEL`, then
`A>B:FMANDEL` draws the set in seconds.
[A demo](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/i8086/cpm86_80c86_8087.cast)
on the P80C86A-2 and D8087: `U` uploads `cpm86.hex`, `O` mounts A: and B:, and CP/M-86
boots; Aztec C builds `fmandel.c` for the 8087 and DRI's C builds `imandel.c`, and each
draws the set. [The same on the P80C86 alone](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/i8086/cpm86_80c86.cast)
links `fmandel` with `M87S.LIB`, which falls back to software floating point.

## CP/M-68K on the 68000

[`samples/mc68000/cpm68kbios.asm`](../samples/mc68000/cpm68kbios.asm) is a CP/M-68K 1.3
BIOS for the MC68000 and MC68008 boards, on the ACIA and the sector disk at FFFFE0H. DRI's
CCP and BDOS, linked at 15000H, and the BIOS at 1B000H load with one `L`: no system tracks
are read. The TPA runs from 1C000H to 64K below the top of the board's address space, at
most 8 MB, which the BIOS finds at cold boot.

DRI's binaries stay out of the repository: the first `make` downloads them from [The
Unofficial CP/M Web Site](http://www.cpm.z80.de/), under the license in [its license
page](http://www.cpm.z80.de/license.html), into `samples/mc68000/cpm68k.fetch/`, with
`debugger/mc68000/tools/cpm68k-fetch.py`. They are CCP and BDOS (`DISK8/CPM15000.SR`) and
the tools of [68kv1_3.zip](http://www.cpm.z80.de/download/68kv1_3.zip), the Alcyon C of
CP/M-68K 1.2 from [68kv1_2.zip](http://www.cpm.z80.de/download/68kv1_2.zip), as the C068
of 1.3 converts floating constants wrongly, and CBASIC from
[cb68.zip](http://www.cpm.z80.de/download/cb68.zip); the command files are renamed from
`.REL` to `.68K`. Then:

```sh
cd samples/mc68000
make cpm68k     # the system and its disks; `make` alone builds the samples and the BIOS
make cpm68k-sd  # puts them in mc68000/ on the SD card of the 68000 bench
```

`make cpm68k` makes `cpm68k.s19`, `cpm68ka.dsk` and `cpm68kb.dsk`; `make cpm68k-sd`, with the
bench locked or the only one on USB and a 68000 at its prompt, puts them in `mc68000/` on its
card, B: only onto a card without one so that files saved there stay (`NEWB=1` replaces it).
A: and B: are 8 MB hd8 disks, which `L` mounts beside `cpm68k.s19`.
`L mc68000/cpm68k.s19`, `R`, then `G` boots to `A>` on the MC68000, the MC68HC001 on its
16-bit and 8-bit bus, the MC68008P and the MC68008FN. ILLEGAL stops in the debugger, which
watches its vector fetch.

On the first boot, `AS68 -I AS68INIT` at `A>` writes the assembler's `AS68SYMB.DAT` to A:.
Then at `B>`, `CBASIC FMANDEL` compiles and runs
[`fmandel.bas`](../samples/mc68000/fmandel.bas) with CBASIC, and `CCFFP FMANDEL` and
`CCIEEE FMANDEL` [`fmandel.c`](../samples/mc68000/fmandel.c) with Alcyon C, in Motorola's
fast floating point and in IEEE single precision. `CC MANDEL` builds
[`mandel.c`](../samples/mc68000/mandel.c), the integer `mandelbrot` in 16-bit `int`,
without floating point.
[A demo](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mc68000/cpm68k_mc68000p.cast)
on the MC68000P10: `U` uploads `cpm68k.s19`, `O` mounts A: and B:, and CP/M-68K boots;
Alcyon C's passes build `fmandel.c` with fast floating point and `mandel.c` without, and
each draws the set.

DRI linked `CPM15000.SR` for the 68000; on the MC68010 a program reading SR traps, as
`MOVE from SR` is privileged there. The 68010 system links on CP/M-68K itself, since DRI's
BDOS for it comes only as `CPMLIB`'s objects: with a 68000 or 68008 board at the bench,
`make cpm68k10` runs `debugger/mc68000/tools/cpm68k10-build.py`, which puts `cpm68k`'s
files on the card, boots, runs the steps of [`cpm68k10.sub`](../samples/mc68000/cpm68k10.sub)
at `A>` (LO68's `-UM68010` adds the BDOS's handler that runs such instructions as `MOVE from
CCR`), and takes `CPM68010.SYS` off A: into `cpm68k.fetch/`; the Makefile then wraps it with
the BIOS as `cpm68k10.s19` and copies A: and B: as `cpm68k10a.dsk` and `cpm68k10b.dsk`.
`make cpm68k10-sd` puts the three on the card, and `L mc68000/cpm68k10.s19`, `R`, `G` boot
the MC68010 to `A>`, where AS68 and Alcyon C run.
[A demo](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mc68000/cpm68k10_build_mc68000p.cast)
on the MC68000P10 runs that link
([`cpm68k10_build.scenario`](../samples/mc68000/cpm68k10_build.scenario)): a minute and a
half from the boot to `CPM68010.SYS` on A:.
[Another](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/mc68000/cpm68k10_mc68010p.cast)
boots that system on the MC68010P12 and builds and runs `fmandel.c` and `mandel.c` as the
MC68000P demo does.

## MS-DOS 2.0 on the 8086

[`samples/i8086/rombios.asm`](../samples/i8086/rombios.asm) is a PC-style ROM BIOS in
segment F000H: INT 10H and 16H as a teletype on the USART, INT 13H on the sector disk
with units A and B the floppies 0 and 1 and C and D the hard disks 80H and 81H, INT 1AH
from the debugger's timer as INT 8, and INT 19H, which boots A: when it is mounted and
the hard disk's MBR otherwise. The BIOS data area is 0040:0010 up: the debugger parks
the CPU at 0400H-040FH.

MS-DOS 2.0 runs on it from
[github.com/microsoft/MS-DOS](https://github.com/microsoft/MS-DOS) (MIT), whose `v2.0/bin`
has `MSDOS.SYS`, `COMMAND.COM` and the utilities but no IO.SYS, the OEM's half.
[`msdosbio.asm`](../samples/i8086/msdosbio.asm) is that half: CON on INT 10H/16H, CLOCK on
INT 1AH and the four disk units on INT 13H, modelled on the OEM kit's `SKELIO.ASM`; it
is linked with Microsoft's `SYSINIT.OBJ` and `SYSIMES.OBJ` by
`scripts/omf-link.py`, which also writes `sysinit.inc`, SYSINIT's variables.
[`msdosboot.asm`](../samples/i8086/msdosboot.asm) is the boot sector, which loads IO.SYS
and MSDOS.SYS, the first two directory entries, to 0070:0000 and after;
[`mbr.asm`](../samples/i8086/mbr.asm) the master boot record. `scripts/msdos-disk.py`
writes the FAT12 images, and lists, reads and adds to them.

Microsoft's files stay out of the repository: `make` downloads them once with
`debugger/i8086/tools/msdos-fetch.py` into `samples/i8086/msdos.fetch/`, pinned to a
commit and checked by sha256, and builds `msdos200.hex` and the disks with them:

```sh
cd samples/i8086
make
```

Copy those into an `i8086/` folder on the SD card. `msdos200.hex` is the ROM BIOS under
the name whose disks `L` mounts: `msdos200a.dsk`, a 360K `fd360` boot floppy with the system,
`COMMAND.COM` and the small utilities, as A:; `msdos200b.dsk`, a formatted blank floppy, as
B:; and `msdos200c.dsk`, the 32 MB `hd32` hard disk with every utility, as C:.
`L i8086/msdos200.hex`, `R`, then `G` boots A:, asks the date and time as a PC with no
AUTOEXEC.BAT does, and prompts `A>`; `O A` first, unmounting the floppy, boots the hard
disk to `C>`. `DATE` and `TIME` run on the tick; the clock starts at 1980-01-01 00:00.

The hard disk is 65536 sectors of 512 bytes, which the BIOS addresses as 1024
cylinders x 4 heads x 16 sectors, so its MBR's one partition runs from sector 16 through
cylinder 1021: 65392 sectors, 16 a cluster, 512 root entries, 4083 clusters, within
FAT12's 4084 and the BPB's 65535 sectors. `msdos-disk.py` lays it out on the host;
DOS 2.0's FORMAT and FDISK are OEM utilities the kit leaves out.
Demos on [the P80C86](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/i8086/msdos200_80c86.cast)
and [the P80C88](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/i8086/msdos200_80c88.cast):
`L` loads the BIOS and mounts the disks, DOS boots from A:, lists and
checks A: and C:, copies to B: and keeps time; then, with A: unmounted, it boots from
the hard disk.

### MS-DOS 2.11, assembled on the board

Microsoft's repository has no MS-DOS 2.11 binaries, so the board assembles them, under MS-DOS 2.0, with
MASM 5.10 and LINK 3.65 from the same repository's `v4.0/src/TOOLS` and 2.0's EXE2BIN.
The MSDOS.SYS kernel is [John Elliott's tree](https://www.seasip.info/DOS/) (MIT), which
completes Microsoft's with the STDIO.ASM, IO.ASM and IO2.ASM it lacks; COMMAND.COM,
SYSINIT and the ten utilities 2.0's disks have are Microsoft's 2.11 sources.
`debugger/i8086/tools/msdos-build.py` sets them up to build the generic MS-DOS that OEMs
got, rather than the IBM version, whose kernel leaves EXEC out, while keeping the UK, German, French
and Japanese country tables. It gives SYSIMES.ASM the messages 2.11's SYSINIT needs, and
works around what MASM 5.10 reads differently from the assemblers of 1983: COMMAND.ASM's
`MOV AH,WAIT`, and TDATA.ASM's line feeds with bit 7 set. SORT links with `/CP:1`, as
Microsoft's did. IO.SYS is `msdosbio.asm` again, linked with the SYSINIT.OBJ and
SYSIMES.OBJ the board assembled.

```sh
cd samples/i8086
make bld211.hex bld211c.dsk  # the ROM BIOS and the build disk: MS-DOS 2.0, tools, sources
../../scripts/bionic-bench.py lock bench1
../../debugger/i8086/tools/msdos-build.py bench 2.11 msdos211.built
make msdos211.hex msdos211a.dsk msdos211c.dsk
```

`make bld211c.dsk` fetches the sources first, pinned and checked by sha256
(`msdos-fetch.py --source`). `msdos-build.py bench` turns the debugger's disk cache off,
copies the BIOS and the build disk to the SD card, boots the disk as C:, the only drive
`L bld211.hex` mounts, types `BUILD`
([`msdos211_build.scenario`](../samples/i8086/msdos211_build.scenario)), copies the disk
back, turns the cache on again and takes MSDOS.SYS, COMMAND.COM, SYSINIT.OBJ, SYSIMES.OBJ
and the utilities off it into `msdos211.built/`: half an hour on the P80C86, and the
same bytes each time, whose sha256 the script checks.
[A demo](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/i8086/msdos211_build_80c86.cast) on the P80C86, with the
long silences cut, lists the build disk and runs BUILD, which echoes each MASM and LINK
step. BUILD skips
a module whose object is already on the disk, so a build that stopped picks up where it
left off. Copy `msdos211.hex`, `msdos211a.dsk` and `msdos211c.dsk` into the card's
`i8086/`; `L i8086/msdos211.hex` then boots 2.11 as `L i8086/msdos200.hex` boots 2.0. C:
also has MASM 5.10, LINK 3.65 and 2.0's CREF. `COUNTRY=081` in CONFIG.SYS gives Japan's
year-month-day dates, `044` the UK's day-month-year; the default, `001`, is the US's
month-day-year.
Demos on [the P80C86](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/i8086/msdos211_80c86.cast)
and [the P80C88](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/i8086/msdos211_80c88.cast): 2.11 boots from A:, lists and checks it and sorts C:'s directory through a
pipe; then it boots from the hard disk, which `COUNTRY=081` switches to year-month-day
dates.

The 2.11 hard disk also holds Microsoft C 5.10 from the 4.0 tree, with its small-model
libraries, and `fmandel.c` and `imandel.c`. After `MSC`, which sets the compiler's paths,
`CL FMANDEL.C` builds `FMANDEL.EXE`, whose floating point uses an 8087 when the board has
one and emulates it otherwise. Its `CONFIG.SYS` sets `FILES=20`: with DOS's default of 8,
the compiler stops with fatal error C1015, unable to open a file.

`bld200c.dsk` and `msdos-build.py bench 2.00 msdos200.built` do the same for 2.0's
kernel, from Elliott's 2.00 tree, which gives the release's MSDOS.SYS with MASM 4.00.
MASM 5.10 makes it 3 bytes shorter: it encodes three `AND reg,imm16` whose immediates
fit in a byte in the short form, and everything else only moves.
[A demo](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/i8086/msdos200_build_80c86.cast) on the P80C86, with the long silences cut. The 2.0 disks themselves need no board: `make` builds them on the host.

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
for ancient UNIX. `debugger/pdp11/tools/simh-fetch.py` builds open-simh's PDP-11
simulator into `samples/dcj11/simh.fetch/` from a pinned commit, and `v6-install.py`
installs the system under it, on an 11/40 with those disks on an RK11: it boots the
tape's `rkunix` single-user, builds `/unix` for the 11/40 with the `rk`, `rp` and `tm`
drivers, makes `/dev`'s nodes, boots that kernel, adds the mounts of rk1 and rk2 to
`/etc/rc`, types the DCJ11 samples into `/usr/mandel`, checks the root with `icheck`
and `dcheck`, boots the disk again from its first block, and puts the three disks in
the image's slots.

```sh
cd samples/dcj11
make unix_v6
make unix_v6-sd
```

does all of it in under a minute, and puts `unix_v6.hex` and the image in `dcj11/` on
the SD card of the bench the session holds, once `probe` finds a DCJ11 at its prompt.
The disk holds the user's files too, so it goes only onto a card without one; `NEWB=1`
replaces it.

`L dcj11/unix_v6.hex`, `R`, `G`, Enter, then `unix` at the bootstrap's `@` boots single-user to `#`; `stty
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
into `samples/dcj11/bsd211.fetch/` and checks its sha256; the system is distributed
under the BSD license and [Caldera's license](https://www.tuhs.org/Archive/Caldera-license.pdf)
for ancient UNIX. `bsd211-install.py` then installs it under the simulator V6's
install uses, as 2.11BSD's setup
document has it, on an 11/73 with an RQDX3 and an RD54 (159 MB: a root of 7.8 MB,
8.2 MB of swap, the rest `/usr` with the sources), with the GENERIC kernel, cuts
`/etc/dtab` to the MSCP controller, so that the kernel's autoconfig looks for nothing
the board has not, and puts the DCJ11 samples in `/usr/root` from a tape of its own: [`bsd211/fmandel.s`](../samples/dcj11/bsd211/fmandel.s),
V6's `fmandel.s` with the system calls as 2.11BSD's `as` and kernel want them, and
`samples/arith`'s `fmandel.c`. No tape is emulated on the board: it boots the
installed disk.

```sh
cd samples/dcj11
make bsd211
make bsd211-sd
```

does it in a few minutes, and puts `bsd211.hex` and the image in `dcj11/` on the card as
`make unix_v6-sd` does V6's, kept likewise unless `NEWB=1`.
Plain `make` builds the samples and both loaders alone, offline; `make clean` undoes it,
and `make distclean` removes the disks, the tapes and the simulator.

`L dcj11/bsd211.hex`, `R`, `I DL11 177` (the console sends each character with a parity bit, as V6's
does), `G`: the bootstrap reads block 0, rauboot prints
`73Boot from ra(0,0,0) at 0172150` and its `: ` prompt, and Enter loads
`ra(0,0,0)unix`, the GENERIC kernel, which comes up single-user at `#`. From `#`,
`^D` runs `/etc/rc` without the disk checks (the operator is taken to have done them:
`fsck -p` takes 24 s on a root left dirty) to `login:`; `root` has no password. On the
bench rauboot's banner comes a second after `G`, the kernel's at 40 s, `#` at 2 minutes
and `login:` at 3 min 20 s, with the kernel's own MMU aborts, which it handles,
reported as `?#ABORT` while the debugger is verbose. The halt switch stops the
running system, in the kernel or in a user program, and `G` continues it. `halt` syncs
the disks and executes HALT, which the board's power-up configuration traps to 4; the
debugger takes that trap as the halt and stops at its prompt, with the kernel's PC
(000014) and registers. A HALT in a user program goes on to the kernel, which ends the
program with `Bus error`. `ra(0,0,0)unix -m`, the boot straight to multi-user, is
refused by `/etc/rc` as installed (`rc.conf` is not configured); configured, it checks
the root, which no halt left marked clean, marks it and reboots, which is that same
HALT, stopping at the prompt. So multi-user is reached from `#`. Single-user, `mount /usr`
reaches `/usr/root`, where `as fmandel.s; ./a.out` and `cc -O -o fmandel fmandel.c;
./fmandel` each draw the set.
[A demo](https://tgtakaoka.github.io/retro-bionic/play.html?cast=samples/dcj11/bsd211.cast):
`U` uploads the bootstrap and `O` mounts the disk alone, 2.11BSD boots single-user,
`^D` takes it to multi-user and root logs in; its `cc` builds `fmandel`, which draws
the set, `banner` and `hangman` from `/usr/games` follow, and `halt` stops it. The
long silences are cut, each to a comment line saying what took the time. It's also on
[asciinema.org](https://asciinema.org/a/0WgNmnQ790X4SB6m).

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
