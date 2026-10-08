#!/usr/bin/env python3
"""The Z380's test rings, from its recordings, to stdout: the host test's
test/z380/test_inst_z380/z380.cycles.zst, which
test/recordings_to_fixtures.py compresses. The test walks each and
writes z380.walks.zst (MATCH_GOLDEN=<dir>). A ring is a record of
tab-separated fields: its name, xm, lw, stop, cycles, memory and starts.

The rings are the bench halts of the samples (z380-rings.json.zst), each
with where the sample's listing has an instruction start, and a share of
the recorded runs (z380-profile.jsonl.zst). Every instruction the test
walks in a bench ring must start a line of the listing.

    debugger/z380/tools/rings_z380.py
"""
import json
import os
import re
import sys
from compression import zstd   # Python 3.14+

HERE = os.path.dirname(os.path.abspath(__file__))
Z380 = os.path.dirname(HERE)
PROJ = os.path.dirname(os.path.dirname(Z380))
PAGES = ['00', 'CB', 'ED', 'EDCB', 'DD', 'FD', 'DDCB', 'FDCB']
ORG = 0x100


def load_tables():
    """(page, opc) -> its sequence."""
    tab = {}
    for page in PAGES:
        for line in open(os.path.join(Z380, 'z380-PAGE%s.txt' % page)):
            f = line.split()
            if len(f) >= 5 and re.match(r'^[0-9A-F]{2}$', f[0]):
                tab[(page, int(f[0], 16))] = f[4]
    return tab


def listing(name, cap=None):
    """Where the sample's instructions start: as the ring was captured, if
    it says, else as the listing is now."""
    if cap and 'starts' in cap:
        return set(cap['starts'])
    starts = set()
    for line in open(os.path.join(PROJ, 'samples', 'z380', name + '.lst')):
        m = re.match(r'^(?:\(\d+\))?\s+([0-9A-F]+) : [0-9A-F]{2}', line)
        if m:
            starts.add(int(m.group(1), 16))
    return starts


def hex_memory(name, cap=None):
    if cap and 'image' in cap:
        return {int(a): b for a, b in cap['image'].items()}
    mem = {}
    for line in open(os.path.join(PROJ, 'samples', 'z380', name + '.hex')):
        line = line.strip()
        if line.startswith(':'):
            b = bytes.fromhex(line[1:])
            if b[3] == 0:
                for i in range(b[0]):
                    mem[(b[1] << 8 | b[2]) + i] = b[4 + i]
    return mem


def read_into(mem, cyc, keep):
    """What the reads showed: over |mem|, or only where it has nothing if
    |keep|."""
    for c in cyc:
        if c[0] != 'R':
            continue
        a, d = c[1] & 0xFFFFFF, c[2]
        if c[4] == 1:
            pairs = [(a & ~1, int(d[0:2], 16)), (a | 1, int(d[2:4], 16))]
        else:
            pairs = [(a, int(d[2:4] if a & 1 else d[0:2], 16))]
        for k, v in pairs:
            if keep:
                mem.setdefault(k, v)
            else:
                mem[k] = v


def ring(name, cyc, mem, mode, stop, starts=()):
    """One ring's record: its cycles, the memory it decodes, the listing."""
    # the bytes any instruction start could decode: around the fetches
    near = set()
    for c in cyc:
        if c[0] == 'R':
            near.update(range((c[1] & 0xFFFFFF) - 8, (c[1] & 0xFFFFFF) + 10))
    mem = {a: b for a, b in mem.items() if a in near}
    fields = [name, '1' if mode == 'xm' else '0', '1' if mode == 'lw' else '0',
              '%X' % stop]
    # kind, address, =data; . for a byte transfer
    fields.append(' '.join('%s%X=%s%s' % (
        c[0], c[1], c[2].replace('.', '0').replace(' ', '0'),
        '' if c[4] == 1 else '.') for c in cyc))
    addrs = sorted(mem)
    runs, i = [], 0
    while i < len(addrs):   # contiguous byte runs
        j = i
        while j + 1 < len(addrs) and addrs[j + 1] == addrs[j] + 1:
            j += 1
        runs.append((addrs[i], bytes(mem[a] for a in addrs[i:j + 1]).hex().upper()))
        i = j + 1
    fields.append(' '.join('%X:%s' % r for r in runs))
    fields.append(' '.join('%X' % a for a in sorted(s for s in starts if s in near)))
    return '\t'.join(fields)


def main():
    sys.path.insert(0, HERE)
    import cycles_z380
    tab = load_tables()
    cases = []
    for cap in json.load(zstd.open(os.path.join(HERE, 'z380-rings.json.zst'), 'rt')):
        cyc = cycles_z380.cycles(cap['text'])
        stop = int(re.search(r'PC=([0-9A-F]+)', cap['text']).group(1), 16)
        mem = hex_memory(cap['sample'], cap)
        read_into(mem, cyc, False)   # what the reads showed wins, as on the board
        cases.append(ring('%s-%d' % (cap['sample'], cap['halt']), cyc, mem, 'n', stop,
                          listing(cap['sample'], cap)))
    recs = [json.loads(l) for l in zstd.open(os.path.join(HERE, 'z380-profile.jsonl.zst'), 'rt')]
    for n, r in enumerate(recs):
        if r['end'] != 'exit' or r.get('anomaly') or not r.get('exit_pushed'):
            continue    # no stop to walk to
        seq = tab.get((r['page'].split('/')[-1], r['opc']), '')
        interesting = '/' in r['page'] or r['key'].endswith((':xm', ':lw')) or (
            re.search(r'[J?{]', seq) is not None)
        if n % (5 if interesting else 23):
            continue
        cyc = r['cycles']
        mem = {ORG + i: b for i, b in enumerate(bytes.fromhex(r['bytes']))}
        read_into(mem, cyc, True)
        mode = 'xm' if r['key'].endswith(':xm') else 'lw' if r['key'].endswith(':lw') else 'n'
        cases.append(ring(r['key'], cyc, mem, mode, r['exit_pushed'] - 1))
    sys.stdout.write('# Generated by: debugger/z380/tools/rings_z380.py\n'
                     '# The bench rings, with the listing\'s instruction starts,\n'
                     '# and a share of the recorded runs.\n')
    sys.stdout.write('\n'.join(cases) + '\n')


if __name__ == '__main__':
    main()
