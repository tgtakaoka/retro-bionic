#!/usr/bin/env python3
"""Run the Z380 samples on a normal build and compare every disassembled
line of the backtrace with samples/z380/*.lst, to check the matcher
built from the z380-PAGExx.txt tables.

    debugger/z380/tools/check_samples.py
"""
import importlib.machinery
import os
import re
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
PROJ = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
rc = importlib.machinery.SourceFileLoader(
    'rc', os.path.join(PROJ, 'scripts', 'record-cycles.py')).load_module()
cz = importlib.machinery.SourceFileLoader(
    'cz', os.path.join(HERE, 'cycles_z380.py')).load_module()
CYC = cz.CYC

SAMPLES = os.path.join(PROJ, 'samples', 'z380')
# How each sample is driven: what to type, or None to halt it after a while.
CHECKS = [('arith', None), ('mandelbrot', None), ('echo', b'ok'),
          ('echoir', b'irq'), ('echoitr', b'az')]


def listing(name):
    """address -> hex bytes of every instruction in the sample's listing,
    a DDIR with the instruction it modifies, as the debugger shows them."""
    starts = {}
    last = ddir = None
    for line in open(os.path.join(SAMPLES, name + '.lst')):
        m = re.match(r'^(?:\(\d+\))?\s*([0-9A-F]+) : ((?:[0-9A-F]{2} )+)\s*(\S*)', line)
        if not m:
            continue
        addr, code, mnemo = int(m.group(1), 16), m.group(2).replace(' ', ''), m.group(3).lower()
        if not mnemo and last is not None:     # more bytes of the line above
            starts[last] += code
        elif mnemo == 'ddir':
            ddir = (addr, code)
        elif mnemo not in ('org', 'db', 'dw', 'ds', 'equ', 'include', 'end'):
            if ddir:
                addr, code = ddir[0], ddir[1] + code
                ddir = None
            starts[addr] = code
            last = addr
    return starts


def main():
    b = rc.Board.open()
    if b.who != cz.NAME:
        sys.exit('target is %s, not %s' % (b.who, cz.NAME))
    # Verbose: the raw cycles are kept for the host.
    if 'Verbose OFF' in b.cmd('V'):
        b.cmd('V')
    failed = 0
    for name, feed in CHECKS:
        b.cmd('R')
        b.upload_file(os.path.join(SAMPLES, name + '.hex'))
        b._write(b'G\r')
        if name == 'arith':
            r, ok = b.until_prompt(60.0)     # runs to its own exit
        else:
            time.sleep(1.5)
            b.send(wait=0.5, idle=0.2)
            if feed:
                for ch in feed:
                    os.write(b.fd, bytes([ch]))
                    time.sleep(0.2)
                b.send(wait=0.5, idle=0.2)
                os.write(b.fd, b'\x00')  # the samples exit on NUL
                r, ok = b.until_prompt(20.0)
            else:
                b.abort()
                r, ok = b.until_prompt(40.0)
        with open(os.path.join(rc.la.RUN_DIR, 'z380-check-%s.txt' % name), 'w') as f:
            f.write(r)          # for the host harness, when a line is wrong
        starts = listing(name)
        # An instruction longer than six bytes goes on in a line of its
        # own, with no mnemonic.
        lines = []
        for l in r.splitlines():
            m = re.match(r'^([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(\S*)', l)
            if not m:
                continue
            if not m.group(3) and lines:
                lines[-1][1] += m.group(2).replace(' ', '')
            else:
                lines.append([l, m.group(2).replace(' ', '')])
        wrong = []
        for l, code in lines[:-1]:      # the last one is the register dump's
            if starts.get(int(l[:8], 16)) != code:
                wrong.append(l[:48].strip())
        lines = [l for l, _ in lines]
        raw = sum(1 for l in r.splitlines() if CYC.match(l) and not l.lstrip().startswith(tuple('0123456789')) or CYC.match(l))
        # A cut instruction at the ring's start may decode as a few
        # lines: allow that at the top, nowhere else.
        last_wrong = max((k for k, l in enumerate(lines[:-1]) if l[:48].strip() in wrong), default=-1)
        ok = len(wrong) <= 5 and last_wrong < 6 and len(lines) > 10
        failed += not ok
        print('%-11s %-4s %3d instructions, %d wrong, %3d raw cycles%s' % (
            name, 'ok' if ok else 'BAD', len(lines) - 1, len(wrong), raw,
            ('  ' + ' | '.join(wrong[:3])) if wrong else ''))
    b.close()
    sys.exit(1 if failed else 0)


if __name__ == '__main__':
    main()
