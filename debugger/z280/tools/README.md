# Z280 debugging tools

Everything here is offline-analysis or bench-measurement tooling for the
Z280, kept out of the generic `scripts/` layer (which is meant to stay
arch-agnostic). None of them are used by `bionic-regress.py`.

- **`cycles_z280.py`** -- the Z280 plugin for `scripts/record-cycles.py`,
  which drives the real chip one instruction pattern at a time and
  records its bus cycles. Needs the profile image (`-D PROFILE_CYCLES`) and
  Python 3.14 (`compression.zstd`). Run from the repository root:
  ```
  P=debugger/z280/tools/cycles_z280.py
  R=debugger/z280/tools/z280-profile.jsonl.zst
  scripts/record-cycles.py $P fill                  # once per flash
  scripts/record-cycles.py $P run --record $R       # every run not yet recorded
  scripts/record-cycles.py $P status --record $R    # what is recorded, what is not
  ```
  Reads `z280-opcodes.txt.zst` and `gen_z280.lst.zst` (libasm's pattern
  list, kept here).

- **`check_samples.py`** -- runs the samples on a normal build and
  cross-checks every disassembled line of the backtrace against
  `samples/z280/*.lst`.

- **`derive_tables.py`** -- turns the `cycles_z280.py` recording into the
  `z280-PAGExx.txt` sequence tables `inst_z280.awk` consumes. Those
  tables are written to the *parent* `debugger/z280/` directory (where
  `inst_z280.awk` and the generated `inst_z280.cpp` live), not here --
  this directory only holds the raw recording and the derivation report.
  ```
  derive_tables.py            # write the tables and the report
  derive_tables.py --check    # report only, don't touch the tables
  ```

- **`decode_capture.py`** -- offline decoder, no live board. `report(txt)`
  decodes already-captured bus-cycle text (e.g. `bionic-control.py reset`'s
  output, or `$BIONIC_OUT`); `logic_report(path)` decodes a Logic-analyzer
  capture CSV already relabeled by `scripts/logic-analyzer.py`'s export.
  Both hardcode this directory's own Z280 knowledge (`STATUS`/`MEM`/
  `REFRESH`) directly -- no target parameter, since everything here is
  already Z280-scoped by being in this directory.

## Regenerating the PAGExx.txt tables

```
scripts/record-cycles.py $P run --record $R    # only if extending the recording
debugger/z280/tools/derive_tables.py           # writes debugger/z280/z280-PAGExx.txt
```
The tables are hand-maintained after that; regenerating is for comparison,
not a build step.
