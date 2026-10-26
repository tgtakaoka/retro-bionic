# Demos

Recorded terminal sessions of real chips being debugged. Each one boots, dumps registers,
disassembles, single-steps with the bus trace visible, and then runs the Mandelbrot program.

The full profile is at [asciinema.org/~tgtakaoka](https://asciinema.org/~tgtakaoka).

## Recordings

Every CPU's recording is listed in [mandelbrot.md](mandelbrot.md), with its
Mandelbrot timing, its cast and its listing; the uploads are on the profile
above.

Some also have a short video: [TMS7000 (TMS7002)](https://www.youtube.com/shorts/hNlnmbQgTaM),
[NSC800](https://www.youtube.com/shorts/VR8pdkmnnt4),
[MC68HC05C0](https://www.youtube.com/shorts/NFmAp0el6kg),
[TMS9981](https://www.youtube.com/shorts/3ToRp99UohU),
[TMS9995](https://www.youtube.com/shorts/OAq7bxscwEU) and
[TMS99105](https://www.youtube.com/shorts/HN6jB54tI-s).

## The GIFs in these docs

The animations in [the README](../README.md) and [debugger.md](debugger.md) are rendered
with [`agg`](https://github.com/asciinema/agg); the README's comes from the TMS9900 cast in
the repository:

```sh
agg --theme github-dark --idle-time-limit 1 --speed 2 --font-size 14 \
    samples/tms9900/mandelbrot.cast docs/images/bionic-demo.gif
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
