# Demos

Recorded terminal sessions of real chips being debugged. Each one boots, dumps registers,
disassembles, single-steps with the bus trace visible, and then runs the Mandelbrot program.

The full profile is at [asciinema.org/~tgtakaoka](https://asciinema.org/~tgtakaoka).

## Recordings

| CPU | Recording | Video | Date |
|---|---|---|---|
| Z86C91 | [a/656654](https://asciinema.org/a/656654) | | 2024-04-27 |
| Z88C00 | [a/656655](https://asciinema.org/a/656655) | | 2024-04-27 |
| Z80 | [a/661752](https://asciinema.org/a/661752) | | 2024-05-30 |
| TMS7000 (TMS7002) | [a/691809](https://asciinema.org/a/691809) | [shorts](https://www.youtube.com/shorts/hNlnmbQgTaM) | 2024-11-24 |
| NSC800 | [a/693783](https://asciinema.org/a/693783) | [shorts](https://www.youtube.com/shorts/VR8pdkmnnt4) | 2024-12-07 |
| MC68HC05C0 | [a/695831](https://asciinema.org/a/695831) | [shorts](https://www.youtube.com/shorts/NFmAp0el6kg) | 2024-12-23 |
| TMS9981 | [a/697735](https://asciinema.org/a/697735) | [shorts](https://www.youtube.com/shorts/3ToRp99UohU) | 2025-01-10 |
| TMS9995 | [a/697736](https://asciinema.org/a/697736) | [shorts](https://www.youtube.com/shorts/OAq7bxscwEU) | 2025-01-12 |
| TMS99105 | [a/697737](https://asciinema.org/a/697737) | [shorts](https://www.youtube.com/shorts/HN6jB54tI-s) | 2025-01-12 |
| MN1613 (MN1613A) | [a/702189](https://asciinema.org/a/702189) | | 2025-02-08 |
| IM6100 | [a/703143](https://asciinema.org/a/703143) | | 2025-02-13 |
| HD6120 | [a/705921](https://asciinema.org/a/705921) | | 2025-03-01 |
| P8080 | [a/706881](https://asciinema.org/a/706881) | | 2025-03-07 |
| MC68HC08AZ0 | [a/716746](https://asciinema.org/a/716746) | | 2025-04-27 |
| TMS9900 | [a/721668](https://asciinema.org/a/721668) | | 2025-06-02 |
| TMS320C15 | [a/723753](https://asciinema.org/a/723753) | | 2025-06-19 |
| KL5C80A12 | [a/757524](https://asciinema.org/a/757524) | | 2025-11-22 |
| Z180 | [a/758967](https://asciinema.org/a/758967) | | 2025-11-30 |
| TMS370Cx5x | [a/783176](https://asciinema.org/a/783176) | | 2026-02-08 |
| P8095BH | [a/859146](https://asciinema.org/a/859146) | | 2026-03-21 |

## The GIFs in these docs

The animations in [the README](../README.md) and [debugger.md](debugger.md) are rendered from
those recordings with [`agg`](https://github.com/asciinema/agg):

```sh
curl -O https://asciinema.org/a/721668.cast
agg --theme github-dark --idle-time-limit 1 --speed 2 --font-size 14 \
    721668.cast docs/images/bionic-demo.gif
```

## What to watch for

- **The banner names the chip the firmware found in the socket**, read from the adapter
  board's identity EEPROM rather than configured by hand — `* BionicTMS9900 * 1.0` in
  [a/721668](https://asciinema.org/a/721668).
- **Some banners report a detected variant in parentheses**: `* BionicMN1613 (CPU: MN1613A) *`
  in [a/702189](https://asciinema.org/a/702189), and
  `* BionicTMS7000 (CPU: TMS7002) *` in [a/691809](https://asciinema.org/a/691809).
- **In verbose mode each stepped instruction is followed by the bus cycles it actually
  performed** — `R A=0100 D=0002` for a read the controller answered, `W` for a write it
  captured. Clear in [a/721668](https://asciinema.org/a/721668) and, on an 8-bit bus where
  the cycle count is much higher, [a/697735](https://asciinema.org/a/697735) (TMS9981).
- **The Mandelbrot render is the same program on every architecture**, so the speed
  difference is directly visible: compare [a/697737](https://asciinema.org/a/697737)
  (TMS99105, hardware multiply and divide) against
  [a/706881](https://asciinema.org/a/706881) (P8080, neither).
- **Prefetching CPUs need a different stepping strategy** — see
  [a/859146](https://asciinema.org/a/859146), where the P8095BH reads up to four bytes ahead
  and the bus trace is reconstructed by pattern matching.
