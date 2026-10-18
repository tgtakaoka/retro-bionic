# scripts

Host tools for the Bionic boards. Each script's docstring is its full
usage.

## Bench

| Script | What it does |
|---|---|
| `bionic-bench.py` | Names each bench (`bench1`, `bench2`, ... by Teensy serial), watches power, flashes and the target seated, and locks a bench or a logic analyzer for one session, with a queue (`lock`, `reserve`, `unlock`, `status`, `watch`, `peek`, `daemon`). |
| `bionic-control.py` | Drives one bench: `flash`, `reset`, `send`, `upload`, `run`, `abort`, `probe`, `recover`. Talks to the bench the session holds; the other bench tools build on it. |
| `bionic-regress.py` | Runs a target's regression suite from `samples/<dir>/bench/regress.toml`: reset, step, samples, haltgo, breakpoint, gountil, disasm. |
| `record-scenario.py` | Plays a `.scenario` console demo live, or records it as an asciinema cast with `--save`. |
| `mandelbrot-timing.py` | Times the mandelbrot frames in recorded casts, as `docs/mandelbrot.md` rows. |
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

## Logic analyzer

| Script | What it does |
|---|---|
| `logic-analyzer.py` | The analyzer interface; exports the newest capture as a CSV labeled from `samples/<target>/bench/channels.toml`. Captures on the analyzer the session holds. |
| `saleae-logic-pro.py` | Dumps a Logic 2 preset's channel names as a `channels.toml` table. |

## Build and boards

| Script | What it does |
|---|---|
| `asm-includes.py` | Lists the files an assembler source includes, for `samples/deps.mk`. |
| `route-length.py` | Compares routed track lengths in a KiCad board with straight-line pad distances. |
| `kicad/` | Draws a new board's first schematic, PCB, `<chip>_bionic.toml` and CPU STEP model; see [kicad/README.md](kicad/README.md). |
| `setup_compiledb.py` | PlatformIO extra script that refreshes `compile_commands.json` for clangd. |
