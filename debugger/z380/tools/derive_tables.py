#!/usr/bin/env python3
"""Turn the cycles_z380.py recording into the z380-PAGExx.txt tables.

Reads z380-profile.jsonl.zst and writes one table per opcode page into the
parent debugger/z380/ directory, alongside inst_z380.awk.

The Z380 fetches code as a stream of aligned words, up to 5 bytes ahead,
so a table row has no bus sequence: the walker (inst_z380.cpp, and
walk_z380.py on the host) follows the fetch stream and needs per opcode
only its length, how it transfers control and how many data bytes it
moves. Columns, shared with inst_z380.awk and walk_z380.py:

  #       length; a DDIR directive in front adds its immediate bytes (IB 1,
          IW 2) when x is +
  c       control transfer, lower case when conditional:
            -  none            A  absolute: the operand's last 2+IB/IW bytes
            L  relative: the operand bytes after the opcode, from the next
               instruction    I  indirect: wherever the next fetch goes
            T  RST: opcode & 38H              H  halt until an interrupt
            B  block repeat: the iteration in the data column
            X  trap: the PC pushed, then a fetch at 0000H
  x       + when a DDIR IB or IW widens its operand
  data    bytes moved, executed or taken: R W read/write memory, r w
          read/write I/O, 0 none, ? not measured. A block's iteration
          is its transfers in order, each with its address step (. any)
  not     a conditional's bytes when not taken
  lw xm   the bytes in Long Word and in Extended mode, - if the same

    derive_tables.py
"""
import json
import os
import re
from compression import zstd   # Python 3.14+
from collections import Counter, defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
Z380 = os.path.dirname(HERE)   # z380-PAGExx.txt live here, with inst_z380.awk
IN = os.path.join(HERE, 'z380-profile.jsonl.zst')
OPCODES = os.path.join(HERE, 'z380-opcodes.txt.zst')
PAGES = ['00', 'CB', 'ED', 'EDCB', 'DD', 'FD', 'DDCB', 'FDCB']

ORG = 0x100
KINDS = 'RWrw'
CONDS = {'NZ', 'Z', 'NC', 'C', 'PO', 'PE', 'P', 'M'}
# A block instruction's iteration: its transfers and their address steps.
BLOCK = {
    'LDIR': 'R+1W+1', 'LDDR': 'R-1W-1', 'CPIR': 'R+1', 'CPDR': 'R-1',
    'LDIRW': 'R+2W+2', 'LDDRW': 'R-2W-2',
    'INIR': 'r.W+1', 'INDR': 'r.W-1', 'OTIR': 'R+1w.', 'OTDR': 'R-1w.',
    'INIRW': 'r.W+2', 'INDRW': 'r.W-2', 'OTIRW': 'R+2w.', 'OTDRW': 'R-2w.',
    # their writes go to on-chip I/O, off the bus
    'OTIMR': 'R+1', 'OTDMR': 'R-1',
}
# Left out: RETB's run jumped where nothing was filled.
UNMEASURED = {'RETB'}


def classify(mnemo, operands):
    """The c column."""
    cond = operands.split(',')[0] in CONDS and mnemo in ('JP', 'JR', 'CALL', 'CALR', 'RET')
    if mnemo in ('JP', 'CALL') and not operands.startswith('('):
        c = 'A'
    elif mnemo in ('JR', 'CALR'):
        c = 'L'
    elif mnemo == 'DJNZ':
        c, cond = 'L', True
    elif mnemo in ('RET', 'RETI', 'RETN', 'RETB') or mnemo == 'JP':
        c = 'I'
    elif mnemo == 'RST':
        c = 'T'
    elif mnemo in ('HALT', 'SLP'):
        c = 'H'
    elif mnemo in BLOCK:
        c = 'B'
    else:
        c = '-'
    return c.lower() if cond else c


EXIT = 0x38


def exit_of(rec):
    """Where the run's exit RST 38H was, and its cycles without that RST's
    push if the debugger missed it: after an I/O write the RST fetches
    0038H before it pushes, and the break was taken one RST later."""
    exit_at, cycles = rec['exit_pushed'] - 1, rec['cycles']
    if exit_at == EXIT and rec['mnemo'] != 'RST':
        at = next(i for i, c in enumerate(cycles) if c[0] == 'R' and c[1] == EXIT)
        push = 4 if rec['key'].endswith(':xm') else 2
        out = []
        for i, c in enumerate(cycles):
            if push > 0 and c[0] == 'W' and abs(i - at) <= 2:
                push -= 2 if c[4] == 1 else 1
                continue
            if c[0] == 'R' and EXIT <= c[1] < EXIT + 6:
                continue   # that RST's own fetches
            out.append(c)
        return rec['exit_pushed'] - 1 if push else ORG + rec['len'], out
    return exit_at, cycles


def data_of(rec):
    """The run's data cycles: all but the pattern's and the exit's fetches."""
    exit_at, cycles = exit_of(rec)
    out = []
    for c in cycles:
        if c[0] == 'R' and ORG <= c[1] < ORG + rec['len'] + 6:
            continue
        if c[0] == 'R' and exit_at - 1 <= c[1] < exit_at + 6 and c[1] != ORG:
            continue
        out.append(c)
    return out


def count(cycles):
    n = Counter()
    for c in cycles:
        n[c[0]] += 2 if c[4] == 1 else 1
    return ''.join('%s%d' % (k, n[k]) for k in KINDS if n[k]) or '0'


def mode_of(key):
    return 'xm' if key.endswith(':xm') else 'lw' if key.endswith(':lw') else 'n'


def load_opcodes():
    """(page, opc) -> length, mnemonic, operands, as libasm decodes them."""
    rows = {}
    for line in zstd.open(OPCODES, 'rt'):
        page, opc, length, _, mnemo, operands = (line.rstrip('\n').split(' ', 5) + [''])[:6]
        if int(length):
            rows[(page, int(opc, 16))] = dict(
                len=int(length), mnemo=mnemo, operands=operands.strip().replace(', ', ','))
    return rows


def derive(recs):
    rows = load_opcodes()
    for r in recs:   # what the recording ran wins
        if '/' not in r['page']:
            rows[(r['page'], r['opc'])] = dict(len=r['len'], mnemo=r['mnemo'],
                                               operands=r['operands'])
    for row in rows.values():
        row.update(c=classify(row['mnemo'], row['operands']), x='-',
                   data=defaultdict(dict))
    for r in recs:
        # CALR M,$ taken calls itself until the ring is full: 'leading'
        if r['end'] != 'exit' or r.get('anomaly'):
            continue
        if '/' in r['page']:
            base = rows.get((r['page'].split('/')[1], r['opc']))
            if base is not None and r['len'] > 2 + base['len']:
                base['x'] = '+'
            continue
        row = rows[(r['page'], r['opc'])]
        exit_at = exit_of(r)[0]
        taken = exit_at != ORG + r['len']
        if row['c'] == '-' and exit_at == 0:
            row['c'] = 'X'
        row['data'][mode_of(r['key'])].setdefault(taken, Counter())[count(data_of(r))] += 1
    # RST 38H's own push is always stripped as the exit's: as any RST.
    rows[('00', 0xFF)]['data'] = rows[('00', 0xF7)]['data']
    # A conditional whose taken run was unusable: as its siblings.
    for (page, opc), row in rows.items():
        if row['c'].islower() and True not in row['data']['n']:
            for (p2, _), sib in sorted(rows.items()):
                if p2 == page and sib['mnemo'] == row['mnemo'] and True in sib['data']['n']:
                    for m in ('n', 'lw', 'xm'):
                        if True in sib['data'][m]:
                            row['data'][m][True] = sib['data'][m][True]
                    break
    return rows


def columns(row):
    """data, not, lw, xm."""
    if row['c'] == 'B':
        return BLOCK[row['mnemo']], '-', '-', '-'
    d = row['data']
    pick = lambda m, t: d[m][t].most_common(1)[0][0] if t in d[m] else None
    cond = row['c'].islower()
    if row['c'] == 'H':
        return '0', '-', '-', '-'
    if cond:
        # not taken, none moves data: also where only the taken ran
        taken, nott = pick('n', True), pick('n', False) or '0'
    else:
        runs = [t for t in (True, False) if t in d['n']]
        taken, nott = (pick('n', runs[0]) if runs else None), None
    same = lambda v: '-' if v is None or v == taken else v
    t = True if cond else (next(iter(d['n']), True))
    return (taken or '?', (nott or '?') if cond else '-',
            same(pick('lw', t)), same(pick('xm', t)))


def write_tables(rows):
    for page in PAGES:
        path = os.path.join(Z380, 'z380-PAGE%s.txt' % page)
        with open(path, 'w') as f:
            f.write('op  mnemo   operands           #  c  x  data      not   lw    xm\n')
            f.write('--  -----   --------           -  -  -  ----      ---   --    --\n')
            for opc in range(256):
                row = rows.get((page, opc))
                if row is None or row['mnemo'] in UNMEASURED or (page == '00' and opc in (0xCB, 0xED, 0xDD, 0xFD)) \
                        or (page in ('DD', 'FD', 'ED') and opc == 0xCB):
                    f.write('%02X  -\n' % opc)
                    continue
                data, nott, lw, xm = columns(row)
                f.write('%02X  %-7s %-18s %d  %s  %s  %-9s %-5s %-5s %s\n' % (
                    opc, row['mnemo'], row['operands'] or '-', row['len'], row['c'],
                    row['x'], data, nott, lw, xm))
        print('wrote', path)


def main():
    recs = [json.loads(line) for line in zstd.open(IN, 'rt')]
    rows = derive(recs)
    # Where variants disagree: the most common count is kept.
    for (page, opc), row in sorted(rows.items()):
        if row['c'] == 'B':
            continue
        for m, by in row['data'].items():
            for t, n in by.items():
                if len(n) > 1:
                    print('%s:%02X %s %s %s' % (page, opc, m, 'taken' if t else 'not', dict(n)))
    write_tables(rows)


if __name__ == '__main__':
    main()
