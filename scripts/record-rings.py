#!/usr/bin/env python3
"""Record bench rings: the bus cycles a sample ran up to a breakpoint, for
a matcher's host test.

Runs on the normal image, verbose: its backtrace prints every cycle of
the ring, in order. Each sample
stops at breakpoints set at labels of its listing: an interrupt sample at
its handler's first, which puts the interrupt's entry at the ring's end,
and at its main loop's, which continuing from the handler next reaches
just after the return. Each stop is one ring.

    record-rings.py PLUGIN --record FILE     record every ring not yet in
                                             FILE (JSON lines; .zst for zstd)
      --stops N        stops at each breakpoint (default 3)
      --only SAMPLE    only this sample
      --samples DIR    where the samples are (default samples/<arch>, the
                       plugin's debugger/<arch>)
      --redo           record again what FILE already has

PLUGIN is the target's scripts/record-cycles.py plugin: its NAME and
cycles(txt). A record is the Z380's capture format, {sample, halt, text,
starts, image}, with key, the stop PC as stop, and the cycles parsed;
test/match_rings.py --bench makes the test's rings of them.
"""
import argparse
import importlib.machinery
import os
import re
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
PROJ = os.path.dirname(HERE)
rc = importlib.machinery.SourceFileLoader(
    'record_cycles', os.path.join(HERE, 'record-cycles.py')).load_module()
rg = importlib.machinery.SourceFileLoader(
    'bionic_regress', os.path.join(HERE, 'bionic-regress.py')).load_module()

# Where a sample stops, by its listing's labels: the first one found of
# each group.
INTERRUPT = [('isr_irq', 'isr_intr', 'isr_int', 'isr_inta', 'isr_intr_rx',
              'isr_sci', 'isr_firq', 'isr'),
             ('receive_loop', 'loop')]
STOPS = {'echoir': INTERRUPT, 'echoitr': INTERRUPT,
         'mmu_echoir': INTERRUPT,
         'sciir': INTERRUPT, 'sciitr': INTERRUPT,
         'uartir': INTERRUPT, 'uartitr': INTERRUPT,
         'mandelbrot': [('loop_x',), ('loop_y',)]}
# What an interrupt sample is fed, a character per interrupt.
FEED = b'abcdefghijklmnopqrstuvwxyz'


def listing(lst):
    """The listing's instruction starts and labels."""
    starts, labels = set(), {}
    for line in open(lst):
        m = re.match(r'^(?:\(\d+\))?\s*([0-9A-F]+) : ((?:[0-9A-F]{2} )+)', line)
        if m:
            starts.add(int(m.group(1), 16))
        m = re.match(r'^(?:\(\d+\))?\s*([0-9A-F]+) :(?: [0-9A-F]{2})*\s+([A-Za-z_][A-Za-z_0-9]*):', line)
        if m:
            labels.setdefault(m.group(2), int(m.group(1), 16))
    return starts, labels


def image(path):
    """The bytes an Intel HEX or S-record file loads."""
    mem = {}
    for line in open(path):
        line = line.strip()
        if line.startswith(':'):
            b = bytes.fromhex(line[1:])
            if b[3] == 0:
                for i in range(b[0]):
                    mem[(b[1] << 8 | b[2]) + i] = b[4 + i]
        elif line[:2] in ('S1', 'S2', 'S3'):
            b = bytes.fromhex(line[2:])
            n = {'S1': 2, 'S2': 3, 'S3': 4}[line[:2]]
            addr = int.from_bytes(b[1:1 + n], 'big')
            for i, v in enumerate(b[1 + n:-1]):
                mem[addr + i] = v
    return mem


def select_io(b, name):
    """Route the console through device |name| (the I command); the one
    enabled before, None if |name| didn't take."""
    lists = b.cmd('I%s\r' % name).replace('\r', '').split('which?')
    def enabled(part):
        found = re.findall(r'^(\S+) .* at [0-9A-Fa-f]+$', part, re.M)
        return found[0] if found else None
    after = enabled(lists[1]) if len(lists) > 1 else None
    if after is None or after.upper() != name.upper():
        return None
    return enabled(lists[0])


def stop_pc(txt):
    dump = [l for l in txt.splitlines() if re.search(r'\bPC=', l)]
    m = re.search(r'\bPC=([0-9A-F]+)', dump[-1]) if dump else None
    return int(m.group(1), 16) if m else None


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('plugin')
    ap.add_argument('--record', required=True)
    ap.add_argument('--stops', type=int, default=3)
    ap.add_argument('--only')
    ap.add_argument('--redo', action='store_true')
    ap.add_argument('--samples')
    args = ap.parse_args()
    p = rc.plugin(args.plugin)
    parse = getattr(p, 'cycles', rc.default_cycles)
    rec_file = rc.Recording(args.record)
    done = rec_file.load()
    with rc.Board.open() as b:
        names = p.NAME if isinstance(p.NAME, tuple) else (p.NAME,)
        if b.who not in names:
            sys.exit('target is %s, not %s' % (b.who, ' or '.join(names)))
        b.radix = getattr(p, 'RADIX', 16)
        # verbose: the backtrace prints every cycle of the ring
        if 'Verbose OFF' in b.cmd('V'):
            b.cmd('V')
        made = 0
        arch = os.path.basename(os.path.dirname(os.path.dirname(
                os.path.abspath(args.plugin))))
        rg.SAMPLES_DIR = args.samples or os.path.join(PROJ, 'samples', arch)
        for path in rg.samples(b.who):
            sample = os.path.basename(path).rsplit('.', 1)[0]
            if sample not in STOPS or (args.only and sample != args.only):
                continue
            starts, labels = listing(path.rsplit('.', 1)[0] + '.lst')
            at = []
            for group in STOPS[sample]:
                found = [l for l in group if l in labels]
                if found:
                    at.append((found[0], labels[found[0]]))
            if not at:
                print('%s: none of its labels' % sample)
                continue
            keys = ['%s-%d' % (sample, n) for n in range(args.stops * len(at))]
            if not args.redo and all(k in done for k in keys):
                continue
            rg.load(b, path)
            # a sample on a device of its own, as the regress runs it
            table = rg.drive_table(os.path.join(rg.SAMPLES_DIR, 'bench',
                    'regress.toml'))
            io = table.get(sample, {}).get('io')
            # where the PC is at a stop: one below it on SC/MP
            offset = table.get('breakpoint', {}).get('pc_offset', 0)
            before = select_io(b, io) if io else None
            if io and before is None:
                print('%s: no %s; built without it?' % (sample, io))
                continue
            mem = {str(a): v for a, v in image(path).items()}
            feeding = STOPS[sample] is INTERRUPT
            n = 0
            # one breakpoint at a time: with the main loop's set too, the
            # spinning loop would stop before an interrupt came
            for k in range(len(keys)):
                label, addr = at[k % len(at)]
                key = '%s-%d' % (sample, k)
                b.clear_breaks()
                b.set_break('%X' % addr)
                b._write(b'G\r')
                r, ok = '', False
                for _ in range(10):
                    # fed until it stops: a character sent before the
                    # sample enables its interrupt is lost
                    if feeding and k % len(at) == 0:
                        time.sleep(0.3)
                        os.write(b.fd, FEED[n % len(FEED):][:1])
                        n += 1
                    more, ok = b.until_prompt(3.0)
                    r += more
                    if ok:
                        break
                if not ok:
                    b.unstick()
                    print('%s: no stop at %s' % (key, label))
                    break
                stop = stop_pc(r)
                if stop is not None:
                    stop -= offset
                if stop != addr:
                    print('%s: stopped at %s, not at %s %X: not kept' % (
                        key, '%X' % stop if stop is not None else '?', label, addr))
                    continue
                if key in done and not args.redo:
                    continue  # recorded already: only the missing ones
                rec = dict(key=key, sample=sample, halt=k, label=label, text=r,
                           stop=stop, starts=sorted(starts), image=mem,
                           cycles=parse(r))
                print('%-14s at %-12s %3d cycles' % (key, label, len(rec['cycles'])))
                rec_file.append(rec)
                made += 1
            if before:
                select_io(b, before)
            b.clear_breaks()
        print('%d rings recorded' % made)


if __name__ == '__main__':
    main()
