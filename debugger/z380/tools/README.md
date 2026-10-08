# Z380 debugging tools

Bench-measurement and offline-analysis tooling for the Z380, kept out of
the generic `scripts/` layer. None of it is used by `bionic-regress.py`.
Needs Python 3.14 (`compression.zstd`). Run from the repository root.

- **`cycles_z380.py`** -- the Z380 plugin for `scripts/record-cycles.py`,
  which runs every opcode pattern on the chip and records its bus cycles.
  Needs the profile image (`-D PROFILE_CYCLES`).
  ```
  P=debugger/z380/tools/cycles_z380.py
  R=debugger/z380/tools/z380-profile.jsonl.zst
  scripts/record-cycles.py $P fill                  # once per flash
  scripts/record-cycles.py $P run --record $R       # every run not yet recorded
  Z380_MODE=xm scripts/record-cycles.py $P fill     # Extended mode: its own stack fill
  Z380_MODE=xm scripts/record-cycles.py $P run --record $R
  Z380_MODE=lw scripts/record-cycles.py $P run --record $R   # after a plain fill
  ```
  Reads `z380-opcodes.txt.zst` and `gen_z380.lst.zst` (libasm's pattern
  list, kept here). Pitfalls it works around: `LDIRW` and the word I/O
  blocks count bytes in BC, so an odd count runs over all of memory;
  `POP SR` loads LW from the stack fill, so the run after it resets; an
  Extended push needs the 4-byte stack fill and far branch targets filled.

- **`derive_tables.py`** -- turns the recording into the `z380-PAGExx.txt`
  tables in the parent directory, in `debugger/match_legend.md`'s tokens,
  which `inst_z380.awk` makes into `inst_z380.cpp`; its docstring has
  what is measured.

- **`rings_z380.py`** -- prints the rings of
  `test/z380/test_inst_z380/z380.cycles.zst` from `z380-rings.json.zst`,
  halts of the samples with their listings' instruction starts, and a
  share of the recorded runs. The host test walks each, holds the walks
  to `z380.walks.zst`, and checks every instruction walked in a bench
  ring starts a line of its listing.

- **`check_samples.py`** -- runs the samples on a normal build and
  cross-checks every disassembled line of the backtrace against
  `samples/z380/*.lst`.
