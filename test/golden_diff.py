#!/usr/bin/env python3
"""Classifies the changes between two matcher goldens of the same rings.

    test/golden_diff.py [--fetch nonzero|one] [--all] RINGS OLD NEW

RINGS is the test's <set>.cycles.zst, OLD and NEW <set>.marks.zst written by
MATCH_GOLDEN=<dir> pio test -e native. A recorded run starts with its
instruction at its first cycle, and with a stop PC its last instruction
is the RST 38H there. A joined scenario a+b can't be walked through a's
exit, so only b's start is required. A change is right when no required
start loses its fetch mark and it marks no more fetches that nothing
proves than before, or when it only drops what the old matcher's start
scan left: marks below the start it settled on, or a start that marks no
fetch; or when each fetch it marks was marked before or is proven. Where a and b start at one address with
other code, the test's memory holds only b's, so only what follows b's
start is compared. Exits non-zero when any change isn't.

A golden of walks (the Z380's: each ring's start and the instructions
walked) is compared walk by walk, every change listed for review.

--fetch says which marks are fetches: any nonzero one (the 68xx, tlcs90,
mc6809 and ins8070 goldens), or only 1 (i8096, z280), whose other marks
are the matcher's own and aren't compared. --all lists every change, not
four of each kind.
"""

import argparse
import collections
import sys
from compression import zstd   # Python 3.14+


def records(path):
    """The tab-separated records of a fixture, test/match_harness.h's."""
    with zstd.open(path, 'rt') as f:
        return [l.rstrip('\n').split('\t') for l in f if l.strip() and l[0] != '#']


def load_rings(path):
    runs = {}
    for f in records(path):
        cycles = []
        for c in f[2].split():
            kind, addr, data = c.split(',')[:3]
            cycles.append((' ' if kind == '_' else kind, int(addr, 16), int(data, 16)))
        runs[f[0]] = (cycles, None if f[1] == '-' else int(f[1], 16))
    return runs


def load_golden(path):
    return {f[0]: (int(f[1]), f[2].split()) for f in records(path) if len(f) == 3}


def proven(runs, name):
    """The cycles proven an instruction's first fetch: (required, stops)."""
    if '+' in name:
        a, b = name.split('+')
        parts = [(b, len(runs[a][0]))]
    elif name.endswith('-1') and name[:-2] in runs:
        parts = [(name[:-2], -1)]
    else:
        parts = [(name, 0)]
    starts, stops = set(), set()
    for key, off in parts:
        cycles, stop = runs[key]
        if off >= 0:
            starts.add(off)
        base = max(off, 0)
        for j, (kind, addr, _) in enumerate(cycles[1 if off < 0 else 0:]):
            if kind == 'R' and stop is not None and addr == stop:
                stops.add(base + j)
                break
    return starts, stops


def leftovers_dropped(old, new):
    """Whether |new| is |old| without the marks below its start, or with
    no walk where |old|'s start marked nothing."""
    (old_start, old_marks), (new_start, new_marks) = old, new
    kept = [m if i >= old_start else '.' for i, m in enumerate(old_marks)]
    if old_start >= 0 and all(m == '.' for m in kept):
        return new_start == -1 and all(m == '.' for m in new_marks)
    return new_start == old_start and new_marks == kept


def b_kept_at_one_address(runs, name, old, new):
    """Whether joined a+b start at one address with other code, and |new|
    walks on from b's start as |old| does."""
    if '+' not in name:
        return False
    a, b = (runs[n][0] for n in name.split('+'))
    reads = {addr: data for kind, addr, data in b if kind == 'R'}
    if a[0][1] != b[0][1] or all(reads.get(addr, data) == data
                                 for kind, addr, data in a if kind == 'R'):
        return False
    at = len(a)
    return 0 <= new[0] <= at and new[1][at:] == old[1][at:]


def load_walks(path):
    return {f[0]: f[1] for f in records(path) if len(f) == 2}


def diff_walks(old, new):
    """Every walk that changed; non-zero if any did."""
    changed = 0
    for name in sorted(set(old) | set(new)):
        if old.get(name) != new.get(name):
            changed += 1
            print('%s:\n    old %s\n    new %s' % (name, old.get(name), new.get(name)))
    print('%6d  walks changed, %d the same' % (changed, len(old) - changed))
    return 1 if changed else 0


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--fetch', choices=('nonzero', 'one'), default='nonzero')
    p.add_argument('--all', action='store_true')
    p.add_argument('rings')
    p.add_argument('old')
    p.add_argument('new')
    args = p.parse_args()
    if not load_golden(args.old) and load_walks(args.old):
        sys.exit(diff_walks(load_walks(args.old), load_walks(args.new)))
    runs = load_rings(args.rings)
    old, new = load_golden(args.old), load_golden(args.new)
    if args.fetch == 'one':
        for golden in (old, new):
            for name, (start, marks) in golden.items():
                golden[name] = (start, [m if m == '1' else '.' for m in marks])
    if set(old) != set(new):
        sys.exit('the goldens name different scenarios')
    if args.fetch == 'one':
        is_fetch = lambda m: m == '1'
    else:
        is_fetch = lambda m: m != '.'
    tally = collections.Counter()
    examples = collections.defaultdict(list)
    for name in old:
        if old[name] == new[name]:
            tally['same'] += 1
            continue
        starts, stops = proven(runs, name)
        o = {i for i, m in enumerate(old[name][1]) if is_fetch(m)}
        n = {i for i, m in enumerate(new[name][1]) if is_fetch(m)}
        bonus = {0} if '+' in name else set()  # a's start, not required
        unproven = lambda f: f - starts - stops - bonus
        if (o & starts) - n:
            kind = 'WORSE: lost a proven start'
        elif len(unproven(n)) > len(unproven(o)):
            kind = 'WORSE: more fetches nothing proves'
        elif (n & starts) > (o & starts):
            kind = 'better: more proven starts'
        elif (n & stops) > (o & stops):
            kind = 'better: marks the stop instruction'
        elif leftovers_dropped(old[name], new[name]):
            kind = 'better: drops the old start scan\'s leftovers'
        elif b_kept_at_one_address(runs, name, old[name], new[name]):
            kind = 'same b: a and b at one address with other code'
        elif not unproven(n) - o:
            kind = 'better: marks no fetch it didn\'t, but proven ones'
        else:
            kind = 'OTHER: needs review'
        tally[kind] += 1
        if args.all or len(examples[kind]) < 4:
            examples[kind].append('%s: old %s %s, new %s %s' % (
                name, old[name][0], ' '.join(old[name][1]),
                new[name][0], ' '.join(new[name][1])))
    for kind, count in sorted(tally.items()):
        print('%6d  %s' % (count, kind))
        for e in examples[kind]:
            print('          ' + e[:150])
    sys.exit(1 if any(k.startswith(('WORSE', 'OTHER')) for k in tally) else 0)


if __name__ == '__main__':
    main()

# Local Variables:
# mode: python
# End:
