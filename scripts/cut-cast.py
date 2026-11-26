#!/usr/bin/env python3
"""Cut the long silences out of an asciinema cast.

    scripts/cut-cast.py CAST --list [--gap SEC]
    scripts/cut-cast.py CAST OUT [--gap SEC] [--keep SEC] [--note T=TEXT]...

--list prints each silence longer than --gap seconds (default 5) with
its start time and the output around it. Otherwise each such silence in
CAST (v2) shrinks to --keep seconds (default 1.5), and halfway through
it a cyan comment line shows what was cut, "# [38 s cut]" or, with
--note T=TEXT for the silence starting at T as listed, "# [38 s cut:
TEXT]". Later events move earlier by the time removed; the result goes
to OUT.
"""
import argparse
import json
import sys


def load(path):
    with open(path) as f:
        header, *lines = f.read().splitlines()
    return header, [json.loads(line) for line in lines if line]


def silences(events, gap):
    """(index, start, length) of each silence longer than |gap| seconds."""
    prev = 0.0
    for i, e in enumerate(events):
        if e[0] - prev > gap:
            yield i, prev, e[0] - prev
        prev = e[0]


def show(events, gap):
    for i, start, length in silences(events, gap):
        before = ''.join(e[2] for e in events[max(0, i - 20):i])[-60:]
        after = ''.join(e[2] for e in events[i:i + 20])[:40]
        print(f'{start:8.1f}  {length:5.1f} s  {before!r} | {after!r}')


def note_of(notes, start):
    for t, text in notes.items():
        if abs(t - start) < 0.05:
            return t, text
    return None, None


def cut(events, gap, keep, notes):
    out = []
    shift = 0.0
    used = set()
    found = {i: (start, length) for i, start, length in silences(events, gap)}
    for i, e in enumerate(events):
        if i in found:
            start, length = found[i]
            t, text = note_of(notes, start)
            used.add(t)
            label = f'{round(length)} s cut' + (f': {text}' if text else '')
            out.append([round(start + keep / 2 - shift, 6), 'o',
                        f'\x1b[36m# [{label}]\x1b[0m\r\n'])
            shift += length - keep
        out.append([round(e[0] - shift, 6)] + e[1:])
    unused = set(notes) - used
    if unused:
        sys.exit('no silence starts at ' +
                 ', '.join(f'{t:.1f}' for t in sorted(unused)))
    return out


def main():
    p = argparse.ArgumentParser(usage=__doc__.strip().splitlines()[2])
    p.add_argument('cast')
    p.add_argument('out', nargs='?')
    p.add_argument('--list', action='store_true')
    p.add_argument('--gap', type=float, default=5.0)
    p.add_argument('--keep', type=float, default=1.5)
    p.add_argument('--note', action='append', default=[])
    args = p.parse_args()
    header, events = load(args.cast)
    if args.list:
        show(events, args.gap)
        return
    if not args.out:
        sys.exit(__doc__.strip())
    notes = {}
    for n in args.note:
        t, sep, text = n.partition('=')
        if not sep:
            sys.exit(f'--note {n}: expected T=TEXT')
        notes[float(t)] = text
    events = cut(events, args.gap, args.keep, notes)
    with open(args.out, 'w') as f:
        f.write(header + '\n')
        for e in events:
            f.write(json.dumps(e, ensure_ascii=False) + '\n')


if __name__ == '__main__':
    main()
