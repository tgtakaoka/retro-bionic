#!/usr/bin/env python3
"""Record a CPU's bus cycles one instruction pattern at a time, and check
them against the debugger's own cycle tables.

Needs the profile image (-D PROFILE_CYCLES, the teensy41-profile
environment: BIONIC_ENV=teensy41-profile bionic-control.py flash), whose run
loop prints every bus cycle without consulting its tables, stops at the trap
ending each pattern, and holds the debug pin active for the run's span.

    record-cycles.py PLUGIN fill                  fill memory with the trap
    record-cycles.py PLUGIN run --record FILE     record every run not yet in
                                                  FILE (JSON lines; .zst for
                                                  zstd, compacted at the end)
    record-cycles.py PLUGIN status --record FILE  what is recorded, what is not
    record-cycles.py PLUGIN check --record FILE   compare FILE with the tables
      --only KEY        only this run
      --redo            rerun runs already recorded
      --from N          skip the first N runs
      --limit N         stop after N runs
      --repeat N        also rerun every tenth pattern N times (if the plugin
                        makes such runs)
      --channels TOML:SET
                        the logic analyzer's channel names, a channels.toml
                        file and the set in it
      --capture CH[:EDGE]
                        also capture each run on the logic analyzer, triggered
                        by channel CH (a number or a --channels name) on EDGE
                        (rise, the default, or fall), and check the firmware's
                        cycles against it
      --capture-every N capture only every Nth run (default 1: all)
      --before TIME     kept before the trigger (default 20us)
      --after TIME      kept after it (default 200us)
      --glitch TIME     ignore trigger pulses shorter than this (default 50ns)

All arch knowledge lives in the PLUGIN, a Python file such as
debugger/mc6805/tools/cycles_mc68hc08.py:
    NAME            the board's identity, e.g. 'MC68HC08AZ0', or a tuple of them
    RADIX           the CLI's number radix (optional, default 16)
    ORG             where patterns are written
    FILL            [(addr, count, values)] for the C command
    BASE            [(register, value)] for every run, before its own regs
    RUN_CAP         seconds a run may take before it counts as stuck
                    (optional, default 2: a run takes well under a second)
    setup(b)        once the board is open (optional)
    patterns(args)  the runs, in the order the recording keeps: dicts with
                    'key', 'bytes', and optionally 'regs' {register: value}
                    and 'seed' (addr, values) to write first, or
                    'data_seed' to write to the data space with m
    schedule(runs)  the order to run them in (optional)
    cycles(txt)     the printed backtrace as cycle lists (optional; the
                    default parses 'R A=addr D=data' lines)
    record(run, cycles, ok)
                    the record to keep: run's fields plus at least 'end'
    span(cycles)    the cycles inside the debug pin's span, as (kind, data),
                    data None where nothing drives the bus, or None where
                    they can't be told (only for --capture)
    bus_cycles(rows, col)
                    a capture's rows inside the span as [(kind, data)];
                    col(name) is a row's index for a --channels name (only
                    for --capture)
    restore(run, cycles)
                    [(addr, values)] to put back what a run wrote
    after(b, rec)   whatever a run needs undone (optional; the default
                    resets the CPU unless the run ended in the trap)
    check(rec)      None, or a line describing a mismatch with the tables
The board passed to setup() and after() has cmd(), write(), fill(),
set_reg(), set_regs(), forget_regs() and run().
"""
import argparse
import csv
import glob
import importlib.machinery
import json
import os
import re
import select
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
bc = importlib.machinery.SourceFileLoader(
    'bc', os.path.join(HERE, 'bionic-control.py')).load_module()
la = importlib.machinery.SourceFileLoader(
    'la', os.path.join(HERE, 'logic-analyzer.py')).load_module()
CAP_DIR = os.path.join(la.RUN_DIR, 'record-cycles')


def plugin(path):
    name = os.path.splitext(os.path.basename(path))[0]
    return importlib.machinery.SourceFileLoader(name, path).load_module()


def seconds(text):
    m = re.fullmatch(r'([0-9.]+)\s*(s|ms|us|ns)?', text)
    if not m:
        raise argparse.ArgumentTypeError('bad time %r' % text)
    return float(m.group(1)) * {'s': 1, 'ms': 1e-3, 'us': 1e-6, 'ns': 1e-9}[m.group(2) or 's']


class Board(bc.Board):
    radix = 16
    regs = {}

    def num(self, v):
        return ('%o' if self.radix == 8 else '%d' if self.radix == 10 else '%X') % v

    def until_prompt(self, cap=20.0):
        """Read until the reply ends at the prompt, returning as soon as it
        does: waiting for quiet costs a fifth of a second per command."""
        buf = b''
        end = time.time() + cap
        while time.time() < end:
            r, _, _ = select.select([self.fd], [], [], 0.05)
            if r:
                try:
                    buf += os.read(self.fd, 65536)
                except BlockingIOError:
                    continue
                if bc.at_prompt(buf):
                    return buf.decode('ascii', 'replace').replace('\r', ''), True
        return buf.decode('ascii', 'replace').replace('\r', ''), False

    def cmd(self, s, cap=5.0):
        self._write(s.encode())
        r, ok = self.until_prompt(cap)
        if not ok:
            # Once: work back to the prompt and send it again.
            print('no prompt after %r: %r; recovering' % (s, r[-80:]))
            if self.recover() is None:
                raise RuntimeError('no prompt after %r: %r' % (s, r[-200:]))
            self.forget_regs()
            self._write(s.encode())
            r, ok = self.until_prompt(cap)
            if not ok:
                raise RuntimeError('no prompt after %r: %r' % (s, r[-200:]))
        if s.startswith('R'):
            self.forget_regs()
        return r

    def write(self, addr, data, prog=True):
        """M, or m to reach a target's data space (e.g. an 8096's
        register file)."""
        for i in range(0, len(data), 16):
            self.cmd('%s%s %s\r' % ('M' if prog else 'm', self.num(addr + i),
                                      ' '.join(self.num(v) for v in data[i:i + 16])))

    def fill(self, addr, count, values):
        self.cmd('C%s %s %s\r' % (self.num(addr), self.num(count),
                                   ' '.join(self.num(v) for v in values)), 60.0)

    def set_reg(self, name, value):
        if '?Reg' in self.cmd('=%s %s\r' % (name, self.num(value))):
            raise RuntimeError('register %s rejected' % name)
        self.regs[name] = value

    def set_regs(self, regs):
        """Set the registers whose value is not known to be set already."""
        for name, value in regs:
            if self.regs.get(name) != value:
                self.set_reg(name, value)

    def forget_regs(self):
        self.regs = {}

    def learn_regs(self, txt, names):
        """The values the register dump at the end of |txt| shows, for those
        of |names| it prints in hex (a CC printed as flags is left out)."""
        dump = [l for l in txt.splitlines() if re.search(r'\bPC=', l)]
        if dump:
            for name, value in re.findall(r'\b([A-Z]+)=([0-9A-F]+)\b', dump[-1]):
                if name in names:
                    self.regs[name] = int(value, 16)

    def run(self, cap=2.0):
        """G, then wait for the pattern to stop. Any run changes registers."""
        self._write(b'G\r')
        self.forget_regs()
        return self.until_prompt(cap)

    def unstick(self):
        self.abort()
        time.sleep(0.5)
        if self.recover() is None:
            raise RuntimeError('board will not return to its prompt')
        self.forget_regs()


def default_cycles(txt):
    return [[m.group(1), int(m.group(2), 16), int(m.group(3), 16)]
            for m in re.finditer(r'^([RW]) A=([0-9A-F]+) D=([0-9A-F]+)', txt, re.M)]


def default_after(b, rec):
    if rec['end'] != 'trap':
        b.cmd('R', 30.0)


class Capture:
    def __init__(self, args):
        path, _, name = args.channels.rpartition(':')
        self.names = la.load_channels(path, name)
        index = {v: k for k, v in self.names.items()}
        ch, _, edge = args.capture.partition(':')
        self.trigger = int(ch) if ch.isdigit() else index[ch]
        self.edge = edge or 'rise'
        self.args = args
        self.la = la.SaleaeLogicPro16()

    def start(self):
        a = self.args
        self.handle = self.la.start_capture(
                list(self.names), self.trigger, self.edge, a.before, a.after, a.glitch)

    def finish(self, p):
        """The bus cycles inside the trigger channel's span."""
        path = self.la.finish_capture(self.handle, CAP_DIR)
        try:
            rows = csv.reader(open(path))
            head = next(rows)
            index = {self.names.get(int(h.split()[-1])): i
                     for i, h in enumerate(head) if h.startswith('Channel ')}
            trig = head.index('Channel %d' % self.trigger)
            active = '1' if self.edge == 'rise' else '0'
            span, seen = [], False
            for row in rows:
                if row[trig] == active:
                    seen = True
                    span.append(row)
                elif seen:
                    break
            return p.bus_cycles(span, index.__getitem__)
        finally:
            for f in glob.glob(os.path.join(CAP_DIR, '*')):
                os.remove(f)


class Recording:
    """FILE as JSON lines, keyed by 'key'; a .zst FILE is written one
    zstd frame per record, so a killed run loses nothing."""

    def __init__(self, path):
        self.path = path
        self.zst = path.endswith('.zst')
        self.cut_short = ()
        if self.zst:
            from compression import zstd   # Python 3.14+
            self.zstd = zstd
            self.cut_short = (EOFError, zstd.ZstdError)

    def load(self):
        done = {}
        if os.path.exists(self.path):
            f = self.zstd.open(self.path, 'rt') if self.zst else open(self.path)
            with f:
                try:
                    for line in f:
                        try:
                            rec = json.loads(line)
                        except ValueError:
                            continue
                        done[rec['key']] = rec
                except self.cut_short:
                    pass            # a frame cut short by a killed run: keep the rest
        return done

    def append(self, rec):
        line = json.dumps(rec) + '\n'
        if self.zst:
            with self.zstd.open(self.path, 'ab') as f:
                f.write(line.encode())
        else:
            with open(self.path, 'a') as f:
                f.write(line)

    def compact(self, order):
        """One record per key, in |order|'s order, then the rest."""
        done = self.load()
        rank = {k: i for i, k in enumerate(order)}
        recs = sorted(done.values(), key=lambda r: (rank.get(r['key'], len(rank)), r['key']))
        # Into a new file, then over the old one: a kill mid-way keeps it.
        tmp = self.path + '.tmp'
        f = self.zstd.open(tmp, 'wt', level=19) if self.zst else open(tmp, 'w')
        with f:
            for r in recs:
                f.write(json.dumps(r) + '\n')
        os.replace(tmp, self.path)


def merged(writes):
    """Contiguous (addr, values) writes as one each."""
    out = []
    for addr, values in sorted(writes):
        if out and out[-1][0] + len(out[-1][1]) == addr:
            out[-1][1].extend(values)
        else:
            out.append((addr, list(values)))
    return out


def problems(p, rec):
    problem = p.check(rec)
    if rec.get('logic_ok') is False:
        problem = (problem + '; ' if problem else '') + 'logic analyzer disagrees'
    if rec.get('logic_error'):
        problem = (problem + '; ' if problem else '') + 'capture failed'
    return problem


def run(b, p, args, rec_file):
    cap = Capture(args) if args.capture else None
    parse = getattr(p, 'cycles', default_cycles)
    after = getattr(p, 'after', default_after)
    done = rec_file.load()
    runs = list(p.patterns(args))
    keys = [r['key'] for r in runs]
    if hasattr(p, 'schedule'):
        runs = p.schedule(runs)
    todo = [r for r in runs[args.start:]
            if (not args.only or r['key'].upper() == args.only.upper())
            and (args.redo or r['key'] not in done)]
    if args.limit:
        todo = todo[:args.limit]
    print('%d runs to do' % len(todo))
    t0 = time.time()
    for n, pat in enumerate(todo, 1):
        if pat.get('seed'):
            b.write(*pat['seed'])
        if pat.get('data_seed'):
            b.write(*pat['data_seed'], prog=False)
        b.write(p.ORG, pat['bytes'])
        b.set_regs(list(p.BASE) + sorted(pat.get('regs', {}).items()))
        capturing = cap and (n - 1) % args.capture_every == 0
        if capturing:
            try:
                cap.start()
            except RuntimeError as e:
                print('capture not started: %s' % str(e)[:120])
                capturing = False
        r, ok = b.run(getattr(p, 'RUN_CAP', 2.0))
        try:
            if not ok:
                try:
                    b.unstick()
                except RuntimeError:
                    sys.exit('%s left the board stuck: power-cycle it, fill, and '
                             'run again; the runs before it are recorded' % pat['key'])
            else:
                b.learn_regs(r, [name for name, _ in p.BASE])
            cyc = parse(r)
            rec = p.record(pat, cyc, ok)
        except BaseException:
            if capturing:               # close the armed capture before leaving
                try:
                    cap.finish(p)
                except RuntimeError:
                    pass
            raise
        if not ok:
            rec['raw'] = r[-1500:]
        if capturing:
            try:
                logic = cap.finish(p)   # closes the capture, whatever the run did
            except RuntimeError as e:
                rec['logic_error'] = str(e)[:200]
            else:
                want = p.span(cyc) if ok else None
                if ok:
                    rec['logic'] = logic
                if want is not None:
                    rec['logic_ok'] = len(logic) == len(want) and all(
                        k == wk and (wd is None or d == wd)
                        for (k, d), (wk, wd) in zip(logic, want))
        print('%4d/%d %-12s %s  %.0fs' % (n, len(todo), rec['key'],
                                          problems(p, rec) or 'ok', time.time() - t0))
        sys.stdout.flush()
        rec_file.append(rec)
        for addr, values in merged(p.restore(pat, cyc)):
            b.write(addr, values)
        after(b, rec)
    rec_file.compact(keys)


def status(p, args, rec_file):
    done = rec_file.load()
    keys = [r['key'] for r in p.patterns(args)]
    missing = [k for k in keys if k not in done]
    ends = {}
    for rec in done.values():
        ends[rec['end']] = ends.get(rec['end'], 0) + 1
    print('%d runs wanted, %d recorded, %d missing' % (len(keys), len(done), len(missing)))
    for k, v in sorted(ends.items()):
        print('  %-24s %d' % (k, v))
    if missing:
        print('missing:', ' '.join(missing[:20]), '...' if len(missing) > 20 else '')


def check(p, rec_file):
    bad = 0
    for key, rec in sorted(rec_file.load().items()):
        problem = problems(p, rec)
        if problem:
            bad += 1
            print('%-12s %s' % (key, problem))
    print('%d mismatches' % bad)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('plugin')
    ap.add_argument('what', choices=('fill', 'run', 'status', 'check'))
    ap.add_argument('--record')
    ap.add_argument('--only')
    ap.add_argument('--redo', action='store_true')
    ap.add_argument('--from', dest='start', type=int, default=0)
    ap.add_argument('--limit', type=int, default=0)
    ap.add_argument('--repeat', type=int, default=0)
    ap.add_argument('--channels')
    ap.add_argument('--capture')
    ap.add_argument('--capture-every', type=int, default=1)
    ap.add_argument('--before', type=seconds, default=20e-6)
    ap.add_argument('--after', type=seconds, default=200e-6)
    ap.add_argument('--glitch', type=seconds, default=50e-9)
    args = ap.parse_args()
    if args.what != 'fill' and not args.record:
        ap.error('%s needs --record' % args.what)
    if args.capture and not args.channels:
        ap.error('--capture needs --channels')
    p = plugin(args.plugin)
    if args.capture and not hasattr(p, 'bus_cycles'):
        ap.error('%s does not support --capture' % args.plugin)
    rec_file = Recording(args.record) if args.record else None
    if args.what == 'check':
        return check(p, rec_file)
    if args.what == 'status':
        return status(p, args, rec_file)
    with Board.open() as b:
        names = p.NAME if isinstance(p.NAME, tuple) else (p.NAME,)
        if b.who not in names:
            sys.exit('target is %s, not %s' % (b.who, ' or '.join(names)))
        b.radix = getattr(p, 'RADIX', 16)
        if hasattr(p, 'setup'):
            p.setup(b)
        if args.what == 'fill':
            b.cmd('R', 30.0)
            for addr, count, values in p.FILL:
                b.fill(addr, count, values)
            b.cmd('R', 30.0)
        else:
            run(b, p, args, rec_file)


if __name__ == '__main__':
    main()
