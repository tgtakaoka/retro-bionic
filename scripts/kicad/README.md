# scripts/kicad

Python helpers that draw the first draft of a Bionic board: the schematic,
an unrouted PCB with the footprints placed or lined up beside it, the
`<chip>_bionic.toml` and a STEP model of the CPU. A board's script uses
them once; after that the KiCad files are edited in KiCad and are the
record.

| Module | What it does |
|---|---|
| `sexp.py` | Parses KiCad s-expressions, cuts a block out of a file, reads a symbol's pins, re-indents a file the way KiCad writes it (`sexp.py <file>`). |
| `symbol.py` | `CpuSymbol`: draws the CPU symbol from pin lists (signals left and right, supplies top and bottom, optionally hidden pins) and checks every pin number appears once. |
| `schematic.py` | `Schematic`: places symbols, wires, labels and power symbols on one A4 sheet. Copies the stock symbols from `schematics/mc68hc16z`. Has the parts every board shares: J1/J2 with their VCC/GND stubs, the 11AA010 on P57, the LED and power flags, decoupling caps. |
| `pcb.py` | `new_board()`: a `.kicad_pcb` with the stackup and rules of MC68HC12B (2 layers) or Z380 (4 layers), the common outline, every footprint from the netlist with its nets, and the plane zones. `pcb.py pours <board> strip\|add [out.dsn]` strips or re-adds the pours, or exports a DSN for freerouting. |
| `bypass.py` | Before routing, gives each supply pin a locked stub towards its bypass capacitor on a private net, and tie pins thin private nets; `bypass.py merge <board>` afterwards closes the gaps, folds them back into VCC and checks each pin is fed only through its capacitor. |
| `stitch.py` | `stitch.py <board> [drc.rpt]` joins GND pour pieces to the main pour with vias, and gives the GND pads a DRC report names a stub to their own via. |
| `straighten_tracks.py` | `straighten_tracks.py <board>` joins track ends that stop microns short of a via or another end, moves vias and junctions onto their tracks' axes, rebuilds every run from horizontal, vertical and 45° segments with pad ends on the pad's centre line, drops micro jogs and segments of no length, inside a pad or on another track, and undoes any change DRC rejects. `python3 -m unittest test_straighten_tracks` in this directory runs its tests. |
| `bionic_toml.py` | Writes `<chip>_bionic.toml` from the netlist and `pinout/<chip>.toml`. `bionic_toml.py map <netlist>` prints the Teensy pin to net table. |
| `qfp_step.py` | STEP model of a square or rectangular gull-wing QFP with its top marking. Needs build123d, so run it with `~/.venvs/cad/bin/python`. |

## A board script

```python
import sys
sys.path.insert(0, 'scripts/kicad')
from symbol import CpuSymbol
from schematic import Schematic

cpu = CpuSymbol('MC9S12DG128', 112, LEFT, RIGHT, TOP, BOTTOM, HIDDEN,
                footprint='bionic:LQFP-112_20x20mm_P0.65mm')
s = Schematic('bionic-mc9s12d')
s.place(s.add_cpu(cpu), 'U1', cpu.name, 48.26, 40.64, 0, cpu.footprint)
s.supply_rows('U1', cpu)                     # VDD pins to VCC, VSS pins to GND
s.label_pins('U1', cpu, LABELS, SHAPES)      # global labels; the rest no-connect
s.connectors(TEENSY, SHAPES)                 # {'P10': 'AD0', ...}
s.id_eeprom()
s.led_and_flags()
s.cap_row(['22u'] + ['0.1u'] * 6)
s.write('schematics/mc9s12d/bionic-mc9s12d.kicad_sch', 'BionicMC9S12D', '2026-10-18')
```

Then, in the board directory:

1. Write `cpu.kicad_sym` from `cpu.library()` (through `sexp.format_text`),
   and copy the library tables, `bionic`/`microchip`/`rhom` symbols and
   footprints, and the `.kicad_pro` from a neighbouring board.
2. `kicad-cli sch erc` must be clean.
3. `kicad-cli sch export netlist`, then `bionic_toml.py map` and compare it
   with the board's column in the Bionic Connector sheet, read live, not
   from a copy.
4. `pcb.new_board()` with the placements known so far; place the rest in
   KiCad. `kicad-cli pcb drc` should show only unconnected items.
5. `bionic_toml.write()` for `schematics/<board>/<chip>_bionic.toml`.

## Routing

Route only after the placement is agreed. Parts never go on the back
between J1 and J2: they cannot be soldered there. Bypass capacitors sit
at their supply pins, on the front where the pin rows leave room.

freerouting treats every zone in a DSN as a plane and never models the
fill, so:

- Export with the inner planes only (`pours strip`, then the planes
  `new_board()` made), route, then `pours add` and refill.
- Remove `(net GND ...)` and GND from the `(class ...)` list in the DSN:
  the pours carry GND. Then `stitch.py` with the DRC report.
- `bypass.split()` before export and `bypass.py merge` after import keep
  each supply pin behind its capacitor.
- Power tracks are 0.3 mm, signal and tie tracks 0.15 mm; freerouting
  necks tracks down between fine-pitch pins, widen them back.
- Last, `straighten_tracks.py`, then line up parts in rows and on their
  pins' axes and centre each reference on its part, one line per row.
- One freerouting pass; a second pass over a DSN with fixed wires loses
  them.
- In pcbnew scripts use `Board.Delete()`, not `Board.Remove()`, which
  crashes later.
