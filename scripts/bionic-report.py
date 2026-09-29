#!/usr/bin/env python3
"""Decode captured bus-cycle text and Logic-analyzer CSVs.

Offline: takes already-captured text or a CSV path, not a live board.
Bus-cycle text (`Signals::print()`) and its status-nibble meaning vary by
target, so this is where that per-target knowledge lives, kept out of
bionic-control.py.
"""
import collections
import re

# Flags, index, direction, address and data are common to every target;
# the status nibble and the byte/word and read/write bits are printed only
# by those that have them.
# Some targets print the slot number and the inject/capture flags in
# front of each cycle, some only in a profiling build: both parse.
CYC = re.compile(r'^(?:([ic ]?[ic ]?)\s*(\d+)\s+)?([RW]) A=([0-9A-F]+) D=\s*([0-9A-F]+)'
                 r'(?: S=([0-9A-F]))?(?: b=(\d))?(?: r=(\d))?')

# Bus status decode, per target. Absent target: print the raw nibble.
STATUS = {
    'Z280': {0: 'Resv', 1: 'Refresh', 2: 'I/O', 3: 'HALT', 4: 'INTA-A',
             5: 'NMIA', 6: 'INTA-B', 7: 'INTA-C', 8: 'MEM', 9: 'MEMnc',
             10: 'EPUmem', 12: 'EPUopr', 13: 'EPUopc', 14: 'EPUcpu',
             15: 'LOCK'},
}
# Which status codes mean a memory transaction / a refresh cycle, per
# target -- same shape as STATUS, since both are the same kind of
# per-target status-code knowledge.
MEM_STATUS = {'Z280': (8, 9)}
REFRESH_STATUS = {'Z280': (1,)}


def status_names(target):
    return STATUS.get(target, {})


def parse(txt):
    out = []
    for line in txt.split('\n'):
        m = CYC.match(line)
        if m:
            f, n, rw, a, d, st, b, r = m.groups()
            out.append(dict(flag=(f or '').strip(), n=int(n) if n else len(out), rw=rw, addr=int(a, 16),
                            data=int(d, 16),
                            st=int(st, 16) if st else None,
                            bw=int(b) if b else None, line=line.rstrip()))
    return out


def rle(vals):
    if not vals:
        return ''
    parts, cur, n = [], vals[0], 1
    for v in vals[1:]:
        if v == cur:
            n += 1
        else:
            parts.append('%d%s' % (cur, 'x%d' % n if n > 1 else ''))
            cur, n = v, 1
    parts.append('%d%s' % (cur, 'x%d' % n if n > 1 else ''))
    return ' '.join(parts)


def report(txt, target='', head=14, full=False, out_path=None):
    name = status_names(target)
    cy = parse(txt)
    if not cy:
        print('no cycle lines (%d bytes)\n%s' % (len(txt), txt[:400]))
        return
    line = 'cycles=%d  injected=%d  captured=%d' % (
        len(cy), sum(1 for x in cy if 'i' in x['flag']),
        sum(1 for x in cy if 'c' in x['flag']))
    c = collections.Counter(x['st'] for x in cy if x['st'] is not None)
    if c:
        line += '   ' + '  '.join('%s=%d' % (name.get(k, hex(k)), v)
                                  for k, v in sorted(c.items()))
    print(line)
    # Reads that are memory, for targets that say so; otherwise all reads.
    mem = MEM_STATUS.get(target) if c else None
    rd = [x['addr'] for x in cy if x['rw'] == 'R'
          and (mem is None or x['st'] in mem)]
    if len(rd) > 1:
        print('read addr deltas: %s' % rle([rd[i + 1] - rd[i]
                                            for i in range(len(rd) - 1)])[:300])
    w = [x for x in cy if x['rw'] == 'W']
    print('writes=%d' % len(w))
    for x in w[:12]:
        print('   ' + x['line'])
    if len(w) > 12:
        print('   ... %d more' % (len(w) - 12))
    print('--- first %d ---' % head)
    for x in (cy if full else cy[:head]):
        print(x['line'])
    tail = [l for l in txt.split('\n') if re.match(r'^(PC|IP|IX|A|W)=', l)]
    if tail:
        print('\n'.join(tail))
    if out_path:
        print('(full text: %s)' % out_path)


def logic_report(path, target=''):
    """`path` is a CSV already relabeled by logic_analyzer.py's export."""
    import csv
    name = status_names(target)
    r = csv.DictReader(open(path))
    f = r.fieldnames

    def col(want, default_ch):
        """The CSV column for a named signal -- relabeled, or Channel N."""
        return want if want in f else 'Channel %d' % default_ch

    ST = [c for c in f if c.startswith('ST')] or \
        [col('ST%d' % i, 4 + i) for i in range(4)]
    AS = col('#AS', 10)
    RS = col('#RESET', 13)
    WT = col('#WAIT', 12)
    prev, ev, wait, last, rst = None, [], [], 0.0, None
    for row in r:
        t = float(row['Time [s]'])
        last = t
        if prev is not None:
            if prev[RS] == '0' and row[RS] == '1':
                rst = t
            if prev[AS] == '0' and row[AS] == '1':
                ev.append((t, sum(int(row[c]) << i for i, c in enumerate(ST))))
            if prev[WT] != row[WT]:
                wait.append((t, int(row[WT])))
        prev = row
    c = collections.Counter(s for _, s in ev)
    print('span %.3fs  reset@%s  transactions=%d  #WAIT edges=%d' % (
        last, rst, len(ev), len(wait)))
    for st, n in sorted(c.items()):
        print('   %X %-8s %7d  %5.1f%%' % (st, name.get(st, '?'), n,
                                           100.0 * n / len(ev)))
    mem = [t for t, s in ev if s in MEM_STATUS.get(target, ())]
    ref = [t for t, s in ev if s in REFRESH_STATUS.get(target, ())]
    if mem:
        print('MEM     %.5f..%.5f s (n=%d)' % (mem[0], mem[-1], len(mem)))
    if ref:
        print('Refresh %.5f..%.5f s (n=%d)' % (ref[0], ref[-1], len(ref)))
        if mem:
            print('refresh after last MEM: %d'
                  % len([t for t in ref if t > mem[-1]]))
    print('#WAIT: ' + ' '.join('%.1f%s' % (t * 1e6, 'H' if v else 'L')
                               for t, v in wait[:14]))
