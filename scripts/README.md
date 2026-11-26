# scripts

Host tools for the Bionic boards. Each script's docstring is its full
usage.

## Bench

| Script | What it does |
|---|---|
| `bionic-bench.py` | Names each bench (`bench1`, `bench2`, ... by Teensy serial), watches power, flashes and the target seated, and locks a bench or a logic analyzer for one session, with a queue (`lock`, `reserve`, `unlock`, `status`, `watch`, `peek`, `daemon`). |
| `bionic-control.py` | Drives one bench: `flash`, `reset`, `send`, `upload`, `run`, `abort`, `probe`, `recover`, and `sd put|sync|get|rm` for files on the SD card (YMODEM through lrzsz). Talks to the bench the session holds; the other bench tools build on it. |
| `bionic-regress.py` | Runs a target's regression suite from `samples/<dir>/bench/regress.toml`: reset, step, samples, haltgo, breakpoint, gountil, disasm, backtrace. |
| `record-scenario.py` | Plays a `.scenario` console demo live, or records it as an asciinema cast with `--save`. |
| `mandelbrot-timing.py` | Times the mandelbrot frames in recorded casts, as `docs/mandelbrot.md` rows. |
| `retime-cast.py` | Moves a cast's first output to 0.5 s and holds its last screen 3 s, in place; `record-scenario.py --save` runs it on each recording. |
| `cut-cast.py` | Writes a copy of a cast with each long silence (a boot, a compile) cut to 1.5 s and a `# [38 s cut: ...]` comment line where it was; `--list` shows the silences to write notes for. |
| `record-cycles.py` | Records bus cycles per instruction pattern on the profile image and checks them against the debugger's cycle tables. |
| `record-rings.py` | Records the bus cycles leading to breakpoints, as fixtures for the matcher's host tests. |

A session working on a bench:

```sh
scripts/bionic-bench.py reserve bench2   # or lock; returns once it is yours
scripts/bionic-control.py flash          # programs bench2's Teensy only
scripts/bionic-regress.py samples/mc68hc12/bench/regress.toml reset samples
scripts/bionic-bench.py unlock bench2
```

`scripts/bionic-bench.py peek bench2` follows what bench2 prints while
another session drives it. The daemon logs every power change, flash,
target, CrashReport, lock and command in
`/tmp/bionic-bench/bench-events.log`, and its own start and stop.
`scripts/bionic-bench.py watch --mine` prints each state change of the
benches the session holds, one line per event, for a Claude session's
Monitor; `watch` alone covers every bench, and `--deadline SECONDS` ends
it so a re-arm is explicit.

A cast for a sample, then a shorter copy to share:

```sh
scripts/record-scenario.py samples/dcj11/bsd211.scenario --save samples/dcj11/bsd211.cast
scripts/mandelbrot-timing.py samples/dcj11/bsd211.cast    # docs/mandelbrot.md rows
scripts/cut-cast.py samples/dcj11/bsd211.cast --list
scripts/cut-cast.py samples/dcj11/bsd211.cast bsd211_cut.cast \
    --note "20.3=the bootstrap loads /unix from the disk"
```

A cast may be committed cut, as `samples/dcj11/bsd211.cast` is: the cuts
shorten only silences, never a frame's drawing, so `mandelbrot-timing.py`
reads the same rows from it.

## Logic analyzer

| Script | What it does |
|---|---|
| `logic-analyzer.py` | The analyzer interface; exports the newest capture as a CSV labeled from `samples/<target>/bench/channels.toml`. Captures on the analyzer the session holds, at the fastest sample rates Logic 2 allows for the channels: analog first when some are analog. A channel whose entry has `analog = [high, low]` is sampled analog, its row spanning those volts, and digital as well with `digital = true`. |
| `saleae-logic-pro.py` | Dumps a Logic 2 preset's channel names, or a saved capture's names, trigger and Simple Parallel decoders, as a `channels.toml` table; names a saved capture from one. |

## Build and boards

| Script | What it does |
|---|---|
| `asm-includes.py` | Lists the files an assembler source includes, for `samples/deps.mk`. |
| `msdos-disk.py` | Builds the FAT12 floppy and 32 MB hard disk images MS-DOS boots from on the sector disk, and lists, extracts and adds their files. |
| `omf-link.py` | Links Microsoft's SYSINIT.OBJ and SYSIMES.OBJ after the flat IO.SYS BIOS, and prints their symbols as EQUs. |
| `cpm-disk.py` | Builds the 8" CP/M 2.2 disk images the debugger's sector disk mounts from the SD card, lists, extracts, adds and erases their files, and turns CP/M-86's CPM.SYS into HEX. |
| `route-length.py` | Compares routed track lengths in a KiCad board with straight-line pad distances. |
| `kicad/` | Draws a new board's first schematic, PCB, `<chip>_bionic.toml` and a QFP CPU's STEP model, square or rectangular, marked as in a photo of the chip; see [kicad/README.md](kicad/README.md). |
| `setup_compiledb.py` | PlatformIO extra script that refreshes `compile_commands.json` for clangd. |
