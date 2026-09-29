# Z280 debugging tools

Everything here is offline-analysis or bench-measurement tooling for the
Z280, kept out of the generic `scripts/` layer (which is meant to stay
arch-agnostic). All three import `scripts/bionic-control.py` or run
standalone; none of them are used by `bionic-regress.py`.

- **`record_cycles.py`** -- drives the real chip, one instruction pattern
  at a time, and records its bus cycles. Needs firmware built with
  `-D Z280_PROFILE` and Python 3.14 (`compression.zstd`).
  ```
  record_cycles.py fill      # fill memory, once per flash
  record_cycles.py run       # record every pattern not yet recorded
  record_cycles.py status    # what is recorded, what is not
  record_cycles.py check     # cross-check a normal build's samples
  ```
  Its `Board` class subclasses `bc.Board` (`fd`/`send`/`abort`/`recover`/
  `upload_file`/`close` inherited directly) and adds pattern-isolation
  methods (`write`/`dump`/`set_reg`/`reset`/`run`/`unstick`). Note: its
  `run()` and `unstick()` are deliberately *not* named `go()`/`recover()`
  -- those names are already `bc.Board` methods with different signatures
  and semantics (`bc.Board.recover()` in particular is what `bc.Board.
  open()` itself calls internally; shadowing it would break that).
  Writes/reads `z280-profile.jsonl.zst`, `z280-opcodes.txt.zst`,
  `gen_z280.lst.zst` (libasm's pattern list, kept here) -- all in this
  directory, alongside the script.

- **`derive_tables.py`** -- turns `record_cycles.py`'s recording into the
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
cd debugger/z280/tools
python3 record_cycles.py run       # only if extending the recording
python3 derive_tables.py           # writes ../z280-PAGExx.txt
```
The tables are hand-maintained after that; regenerating is for comparison,
not a build step.
