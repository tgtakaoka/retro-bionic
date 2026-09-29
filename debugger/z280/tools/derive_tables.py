#!/usr/bin/env python3
"""Turn the cycles_z280.py recording into the z280-PAGExx.txt tables.

Reads z280-profile.jsonl.zst, writes one table per opcode page (in the
parent debugger/z280/ directory, alongside inst_z280.awk) and
z280-profile-report.txt (prefetch depth, anomalies). The tables are
hand-maintained afterwards; regenerate only to compare.

Legend, shared with inst_z280.awk:
  1-6  instruction byte: a word read at that byte address (1 marks the fetch)
  ~    prefetch: a word read at the next unfetched address; greedy, capped
  B R  read byte / word (memory)        Y W  write byte / word (memory)
  b r  read byte / word (I/O)           y w  write byte / word (I/O)
  a    interrupt acknowledge            h    halt
  { }  repeat group (block instructions)
  --- control transfer: the read at the target ends the fetch stream
  j    next+disp8      J  absolute nn     k  next+disp16    i  unknown target
  A    target == last word read (byte-swapped)   C  target == opcode & 38H
  F    flush without transfer: the next address is fetched again
  S    trap: target == the PC word read from the vector table
  @    taken@not-taken

    derive_tables.py [--check]      write the tables, or only the report
"""
import argparse
import json
import os
import re
import sys
from compression import zstd   # Python 3.14+
from collections import Counter, defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
Z280 = os.path.dirname(HERE)   # z280-PAGExx.txt live here, with inst_z280.awk
IN = os.path.join(HERE, 'z280-profile.jsonl.zst')
REPORT = os.path.join(HERE, 'z280-profile-report.txt')
GEN = os.path.join(HERE, 'gen_z280.lst.zst')   # libasm's, kept here
PAGES = ['00', 'CB', 'ED', 'DD', 'FD', 'DDCB', 'FDCB', 'DDED', 'FDED']

ORG = 0x100
POPPED = 0x0020       # the stack prefill
TRAP_PC = 0x0040      # every vector table entry
INDIRECT = {'HL': 0x2000, 'DE': 0x2100, 'IX': 0x2200, 'IY': 0x2300, 'BC': 0x2400}
IVT = (0x1000, 0x1400)
DIVIDE = {'DIV', 'DIVU', 'DIVW', 'DIVUW'}
BLOCK = {'LDIR', 'LDDR', 'CPIR', 'CPDR', 'INIR', 'INDR', 'OTIR', 'OTDR',
         'INIRW', 'INDRW', 'OTIRW', 'OTDRW'}
ST_REFRESH, ST_HALT, ST_INTA = 1, 3, (4, 5, 6, 7)


def s8(b):
    return b - 256 if b & 0x80 else b


def s16(w):
    return w - 65536 if w & 0x8000 else w


def transfer_code(rec, target, popped, popped_from):
    """Which code explains the observed target: (code, note)."""
    code = bytes.fromhex(rec['bytes'])
    end = ORG + rec['len']
    cands = []
    if rec['len'] >= 3 and target == code[-2] | code[-1] << 8:
        cands.append('J')
    if rec['len'] >= 2 and target == (end + s8(code[-1])) & 0xFFFF:
        cands.append('j')
    if rec['len'] >= 3 and target == (end + s16(code[-2] | code[-1] << 8)) & 0xFFFF:
        cands.append('k')
    if popped is not None and target == popped:
        cands.append('S' if IVT[0] <= popped_from < IVT[1] else 'A')
    if rec['page'] == '00' and (code[0] & 0xC7) == 0xC7 and target == code[0] & 0x38:
        cands.append('C')
    if any(abs(target - v) <= 1 for v in INDIRECT.values()):
        cands.append('i')
    if not cands:
        return 'i', 'target %04X unexplained' % target
    return cands[0], '' if len(cands) == 1 else 'ambiguous %s' % ''.join(cands)


def sequence(rec):
    """The recorded run as sequence letters; (letters, notes)."""
    L = rec['len']
    end = ORG + L
    notes = []
    seq = []
    fetched = set()
    flushes = 0
    prefetch = end            # the next sequential address expected
    popped = None
    popped_from = 0
    target = rec['exit_pushed'] - 1 if rec.get('exit_pushed') else None
    transferred = None
    for kind, addr, data, st, bw in rec['cycles']:
        if st == ST_REFRESH:
            continue
        if st == ST_HALT:
            if 'h' not in seq:
                seq.append('h')
            continue
        if st in ST_INTA:
            seq.append('a')
            continue
        if kind == 'R':
            if transferred is not None:
                if addr == transferred:
                    transferred += 1     # prefetch at the target: dropped
                    continue
            elif ORG <= addr < end:
                if addr in fetched and rec['mnemo'] not in BLOCK:
                    notes.append('refetch of own byte %04X' % addr)
                    continue
                fetched.add(addr)
                seq.append(str(addr - ORG + 1))
                continue
            elif addr == prefetch:
                seq.append('~')
                prefetch += 1
                continue
            elif end <= addr < prefetch:
                # A re-fetch: a stall or a flush repeats what the
                # prefetch already read. Noise the matcher absorbs.
                flushes += 1
                continue
            elif target is not None and addr == target and target != end:
                code, note = transfer_code(rec, target, popped, popped_from)
                if note:
                    notes.append(note)
                seq.append(code)
                transferred = addr + 1
                continue
            seq.append('R' if bw == 0 else 'B')
            if bw == 0:
                popped = int(data[2:4], 16) << 8 | int(data[0:2], 16)
                popped_from = addr
        elif kind == 'W':
            seq.append('W' if bw == 0 else 'Y')
        elif kind == 'r':
            seq.append('r' if bw == 0 else 'b')
        elif kind == 'w':
            seq.append('w' if bw == 0 else 'y')
    if target is not None and target != end and transferred is None \
            and rec['end'] == 'exit':
        notes.append('transfer to %04X not seen as a fetch' % target)
    # A division that overflowed or divided by zero trapped: the harness
    # operands did that, a program's usually do not, so keep the
    # instruction's own cycles and drop the trap's.
    if rec['mnemo'] in DIVIDE and seq[-5:] == ['W', 'W', 'R', 'R', 'S']:
        seq = seq[:-5]
        notes.append('division trap dropped')
    # Prefetch after the last data cycle is implied; the FF's own fetch
    # and whatever ran ahead of the exit are not the pattern's.
    while seq and seq[-1] == '~':
        seq.pop()
    if flushes:
        notes.append('%d re-fetch%s' % (flushes, 'es' if flushes > 1 else ''))
    return seq, notes


def fold_parity(a, b):
    """One entry for both parities.

    A byte load is a word read at an even address but a byte read at an
    odd one: B. A word load or store at an odd address is two byte
    transactions: R, W. Which run was the even one depends on the
    operand gen chose, so both orders are tried.
    """
    if a == b:
        return a, ''
    for even, odd in ((a, b), (b, a)):
        out = []
        i = j = 0
        ok = True
        while i < len(even) and j < len(odd):
            if even[i] == odd[j]:
                out.append(even[i])
                i += 1
                j += 1
            elif even[i] == 'R' and odd[j:j + 2] == ['B', 'B']:
                out.append('R')
                i += 1
                j += 2
            elif even[i] == 'W' and odd[j:j + 2] == ['Y', 'Y']:
                out.append('W')
                i += 1
                j += 2
            elif even[i] == 'R' and odd[j] == 'B':
                out.append('B')
                i += 1
                j += 1
            elif even[i] == '~':
                i += 1
            elif odd[j] == '~':
                j += 1
            else:
                ok = False
                break
        if ok and i == len(even) and j == len(odd):
            return out, ''
    return a, 'parity differs: %s / %s' % (''.join(a), ''.join(b))


def fold_repeat(once, twice):
    """A{G}Z from one and two iterations.

    Prefetch is left out of the comparison. The second iteration of a
    byte move runs at the other parity, where a byte load is a byte
    read instead of a word read: the group takes B for those.
    """
    once = [c for c in once if c != '~']
    twice = [c for c in twice if c != '~']
    same = lambda x, y: x == y or {x, y} == {'R', 'B'}
    n1, n2 = len(once), len(twice)
    for g in range(1, n1 + 1):
        if n2 != n1 + g:
            continue
        for a in range(0, n1 - g + 1):
            g1, g2 = once[a:a + g], twice[a + g:a + 2 * g]
            if twice[:a + g] == once[:a + g] and twice[a + 2 * g:] == once[a + g:] \
                    and all(same(x, y) for x, y in zip(g1, g2)):
                group = ['B' if {x, y} == {'R', 'B'} else x for x, y in zip(g1, g2)]
                return once[:a] + ['{'] + group + ['}'] + once[a + g:], ''
    return once, 'repeat differs: %s / %s' % (''.join(once), ''.join(twice))


def load():
    recs = defaultdict(dict)
    if not os.path.exists(IN):
        sys.exit('no recordings: %s' % IN)
    for line in zstd.open(IN, 'rt'):
        try:
            rec = json.loads(line)
        except ValueError:
            continue
        recs[(rec['page'], rec['opc'])][rec['variant']] = rec
    return recs


OPCODES = os.path.join(HERE, 'z280-opcodes.txt.zst')


def shape(operands):
    """Operands with registers, numbers and conditions classed, so that
    encodings that differ only in a register or a bit number share a
    sequence."""
    s = operands.replace(' ', '')
    s = re.sub(r'<[^>]*>', 'rel', s)
    s = re.sub(r'\((HL|IX|IY)\+(IX|IY)\)', '(bx)', s)
    s = re.sub(r'\((IX|IY|SP|PC|HL)[+-][0-9A-F]+H?\)', '(idx)', s)
    s = re.sub(r'\((HL|BC|DE)\)', '(ind)', s)
    s = re.sub(r'\([0-9A-F]+H\)', '(nn)', s)
    s = re.sub(r"\b(AF'|AF|BC|DE|HL|IX|IY|SP|USP|DEHL)\b", 'rr', s)
    s = re.sub(r'\b(IXH|IXL|IYH|IYL|A|B|C|D|E|H|L)\b', 'r', s)
    s = re.sub(r'\b(NZ|Z|NC|C|PO|PE|P|M)\b', 'cc', s)
    s = re.sub(r'\b[0-9][0-9A-F]*H?\b', 'n', s)
    return s


def all_opcodes():
    """(page, opc) -> (mnemo, operands, len) for every opcode, from libasm."""
    out = {}
    for line in zstd.open(OPCODES, 'rt'):
        page, opc, length, code, mnemo, operands = (line.rstrip('\n').split(' ', 5) + [''])[:6]
        if int(length):
            out[(page, int(opc, 16))] = (mnemo, operands.strip(), int(length))
    return out


def fill_holes(rows, pats, report):
    """A sequence for every opcode the pattern list has no row for, from
    a recorded opcode on the same page with the same mnemonic, operand
    shape and length."""
    every = all_opcodes()
    by_shape = {}
    for (page, opc), (mnemo, operands, length) in pats.items():
        if (page, opc) in rows:
            by_shape.setdefault((page, mnemo, shape(operands), length), rows[(page, opc)])
    # A prefix byte followed by filler decodes as something; it is no row.
    PREFIX = {('00', 0xCB), ('00', 0xED), ('00', 0xDD), ('00', 0xFD),
              ('DD', 0xCB), ('DD', 0xED), ('FD', 0xCB), ('FD', 0xED)}
    SIBLING = {'DD': 'FD', 'FD': 'DD', 'DDCB': 'FDCB', 'FDCB': 'DDCB'}
    filled = unfilled = 0
    for key, (mnemo, operands, length) in sorted(every.items()):
        if key in PREFIX:
            continue
        if key not in pats:
            pats[key] = (mnemo, operands, length)
        if key in rows:
            continue
        seq = by_shape.get((key[0], mnemo, shape(operands), length))
        if seq is None and key[0] in SIBLING:
            # IX and IY pages mirror each other.
            seq = by_shape.get((SIBLING[key[0]], mnemo, shape(operands), length))
        if seq is None:
            unfilled += 1
            report.append('%s:%02X %s %s: no recorded opcode of that shape' % (key[0], key[1], mnemo, operands))
            continue
        rows[key] = seq
        filled += 1
    report.insert(3, '%d opcodes filled by shape from a recorded one, %d left' % (filled, unfilled))


def gen_patterns():
    """(page, opc) -> (mnemo, operands, len) for every pattern of gen_z280.lst."""
    out = {}
    for line in zstd.open(GEN, 'rt'):
        m = re.match(r'^\s*([0-9A-F]+) : ((?:[0-9A-F]{2} )+)\s*(\S+)\s*(.*)$', line)
        if not m:
            continue
        code = bytes.fromhex(m.group(2).replace(' ', ''))
        mnemo, operands = m.group(3), m.group(4).strip()
        if mnemo in ('CPU', 'ORG'):
            continue
        b0 = code[0]
        if b0 in (0xCB, 0xED):
            page, opc = '%02X' % b0, code[1]
        elif b0 in (0xDD, 0xFD):
            if code[1] == 0xCB:
                page, opc = '%02XCB' % b0, code[3]
            elif code[1] == 0xED:
                page, opc = '%02XED' % b0, code[2]
            else:
                page, opc = '%02X' % b0, code[1]
        else:
            page, opc = '00', b0
        out[(page, opc)] = (mnemo, operands, len(code))
    return out


def derive(recs, report):
    """(page, opc) -> (seq string, data count); notes go to the report."""
    rows = {}
    refetch = Counter()
    for (page, opc), by_var in sorted(recs.items()):
        head = '%s:%02X' % (page, opc)
        seqs = {}
        for var, rec in by_var.items():
            if rec['end'] == 'timeout':
                report.append('%s:%s timeout' % (head, var))
                continue
            s, notes = sequence(rec)
            seqs[var] = s
            for n in notes:
                if n.endswith('re-fetch') or n.endswith('re-fetches'):
                    refetch[rec['mnemo']] += 1
                else:
                    report.append('%s:%s %s' % (head, var, n))
            if rec.get('anomaly'):
                report.append('%s:%s anomaly %s' % (head, var, rec['anomaly']))
        if not seqs:
            continue
        rec = next(iter(by_var.values()))

        def pair(a, b, fold):
            if a in seqs and b in seqs:
                s, note = fold(seqs[a], seqs[b])
                if note:
                    report.append('%s %s' % (head, note))
                return s
            return seqs.get(a, seqs.get(b))

        if 'f00' in seqs or 'fff' in seqs:
            f00 = pair('f00', 'f00odd', fold_parity)
            fff = pair('fff', 'fffodd', fold_parity)
            taken = [s for s in (f00, fff) if s and any(c in 'JjkAiS' for c in s)]
            not_taken = [s for s in (f00, fff) if s and not any(c in 'JjkAiS' for c in s)]
            if taken and not_taken:
                seq = taken[0] + ['@'] + not_taken[0]
            else:
                seq = (f00 or fff)
                report.append('%s conditional: taken %s, not taken %s' % (
                    head, ''.join(f00 or []), ''.join(fff or [])))
        elif 'b1' in seqs:
            seq = (seqs.get('b2') or []) + ['@'] + seqs['b1'] if 'b2' in seqs else seqs['b1']
        elif 'c1' in seqs:
            seq = pair('c1', 'c2', fold_repeat)
        else:
            seq = pair('', 'odd', fold_parity)
        for var in ('rep1', 'rep2', 'rep3'):
            base = seqs.get('') if rec['mnemo'] not in BLOCK else None
            if var in seqs and base is not None and seqs[var] != base:
                report.append('%s %s differs: %s / %s' % (head, var, ''.join(base), ''.join(seqs[var])))
        data = sum(1 for c in seq if c in 'BRYWbrywa')
        rows[(page, opc)] = (':'.join(seq), data)
    report.append('runs with re-fetches (a stall or flush repeating a prefetched word), by mnemonic: ' +
                  ' '.join('%s:%d' % kv for kv in sorted(refetch.items())))
    return rows


def prefetch_stats(recs, report):
    before, at_exit = Counter(), Counter()
    for by_var in recs.values():
        for rec in by_var.values():
            if rec['end'] != 'exit':
                continue
            before[rec['ahead_before_data']] += 1
            at_exit[rec['ahead_at_exit']] += 1
    report.insert(0, 'prefetch ahead of the pattern before its last data cycle: %s' %
                  ' '.join('%d:%d' % kv for kv in sorted(before.items())))
    report.insert(1, 'prefetch ahead of the pattern at the exit (incl. the RST fetch): %s' %
                  ' '.join('%d:%d' % kv for kv in sorted(at_exit.items())))


def write_tables(rows, pats):
    for page in PAGES:
        path = os.path.join(Z280, 'z280-PAGE%s.txt' % page)
        with open(path, 'w') as f:
            f.write('op  mnemo   operands           #  ~  sequence\n')
            f.write('--  -----   --------           -  -  --------\n')
            for opc in range(256):
                mnemo, operands, length = pats.get((page, opc), ('-', '-', '-'))
                seq, data = rows.get((page, opc), ('-', '-'))
                if mnemo == '-':
                    f.write('%02X  -\n' % opc)
                    continue
                f.write('%02X  %-7s %-18s %s  %s  %s\n' % (
                    opc, mnemo, operands.replace(' ', '') or '-', length,
                    data if data not in ('-', 0) else '-', seq))


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--check', action='store_true', help='report only')
    args = ap.parse_args()
    recs = load()
    pats = gen_patterns()
    report = []
    rows = derive(recs, report)
    # RST 38H is the harness's own exit: its vector fetch is what the
    # debugger drops from the ring, so its recording ends early. It is
    # the same restart as the others.
    if ('00', 0xC7) in rows:
        rows[('00', 0xFF)] = rows[('00', 0xC7)]
    prefetch_stats(recs, report)
    fill_holes(rows, pats, report)
    missing = [k for k in pats if k not in rows]
    report.insert(2, '%d of %d patterns derived, %d missing' % (len(rows), len(pats), len(missing)))
    with open(REPORT, 'w') as f:
        f.write('\n'.join(report) + '\n')
    print('\n'.join(report[:3]))
    print('%d notes in %s' % (len(report) - 3, REPORT))
    if not args.check:
        write_tables(rows, pats)
        print('tables written: z280-PAGE{%s}.txt' % ','.join(PAGES))


if __name__ == '__main__':
    main()
