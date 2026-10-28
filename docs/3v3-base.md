# A 3.3 V base board (planned)

Not built. The base board runs at 5 V, and that rail is what keeps every low-voltage part in
the inventory out of reach. A second base board would run its CPU side from 3.3 V, the
Teensy's own level.

## What changes

- **The `TXS0108E`s stay, at 3.3 V on both sides.** A 3.3 V target needs no translation, but
  they still buffer and protect the Teensy's pins. Both ports at 3.3 V is within the
  datasheet's `VCCA ≤ VCCB`.
- **A different bus holder.** The `SN74ACT1071` is 5 V only, and there is no low-voltage
  version of it. At 3.3 V the bus-hold cells built into `LVC` inputs do the same job — the
  `H` in `SN74LVCH…` — with the outputs left disabled. One cell is too weak behind a
  `TXS0108E`, though: each `TXS0108E` pin has a pull-up on both ports, 40 kΩ while the line
  is low, and its pass gate joins the two while the line is low, so holding a low at the
  0.8 V input limit takes 2.5 V / 20 kΩ = 125 µA. A bus-hold cell guarantees only 75 µA
  there. So each of the 16 data lines on `P1x` and `P2x` gets **four** bus-hold inputs in
  parallel, four `SN74LVCH16244A`s with every output disabled, on the CPU side where the
  `SN74ACT1071`s are today:
  - holding: at least 4 × 75 µA = 300 µA, the `SN74ACT1071`'s strength, against 125 µA;
  - overdriving: at most 4 × 500 µA = 2 mA for a new value to flip the line, which the CPU
    supplies directly and the Teensy through the `TXS0108E`'s pass gate and edge
    accelerators. Check that one on the bench.
- **Dedicated adapters.** A 3.3 V target needs its own adapter board. Nothing in the 5 V
  family carries over.

## Keying, so neither family reaches the wrong base

Seating a 3.3 V CPU in the 5 V base would destroy it, and the connector already carries the
spare positions needed to prevent that. Four of the 48 positions — `E0`, `E1`, `E2` and `E3`
at pins 10, 19, 30 and 39 — hold no pin on any adapter: `Bionic-P135_THT` and
`Bionic-P245_THT` have 22 pads each, 44 of 48, and the four they skip are exactly those. They
are unconnected on the base as well.

The trick is to make the power connection itself the key. `VCC` today is pins 24 and 25; a
3.3 V board would take its rail from the spare positions instead:

| | 5 V base | 3.3 V base |
|---|---|---|
| pins 24, 25 | 5 V rail | **plugged** |
| `E2`, `E3` (pins 19, 30) | **plugged** | 3.3 V rail |

and correspondingly, a 5 V adapter populates 24 and 25 but not `E2`/`E3`, while a 3.3 V
adapter populates `E2` and `E3` but leaves 24 and 25 empty. GND stays on pins 1 and 48 for
both. `E2` and `E3` face each other across the package, one pin per row, as `VCC` does at 24
and 25, leaving `E0` and `E1` (pins 10 and 39) spare as a second facing pair.

That blocks both directions. A 5 V adapter over a 3.3 V base drives pins 24 and 25 into the
plugs; a 3.3 V adapter over a 5 V base drives its `E2` and `E3` pins into the plugs there.
Neither can go down, and all 49 existing adapters are untouched — they have empty holes at
`E2` and `E3` already, and a base already built is retrofitted by pushing the plugs in. No
PCB revision on either side.

Better still, the arrangement is fail-safe rather than merely interlocked: because a 3.3 V
adapter has no pin at all at the 5 V positions, a 5 V base cannot deliver 5 V to it even if
the mechanical key were somehow defeated. The keying and the power path are the same feature.

The adapter's identity EEPROM has to run from the new rail, which it does — the `11AA010` is
specified from 1.8 V to 5.5 V (it is the `11AA` part, not the 2.5 V `11LC`).

Pick plugs the ZIF lever captures rather than ones that work loose, and that do not damage
the socket contact beneath.

## CPUs in hand it would bring into range

From the inventory, the parts on the shelf that the 5 V base cannot drive:

| arch | Part | Voltage |
|---|---|---|
| `mc68000` | `MC68040FE25V` | 3.3 V |
| `tlcs900/h1` | `TMP92CM22FG` | 3.3 V |
| `tlcs900/l1` | `TMP91C829F` | 3.3 V |

Every one of them is QFP, so each needs an adapter it can be soldered to before it can be
confirmed at all — see [supported-cpus.md](supported-cpus.md) for what is targeted today.
