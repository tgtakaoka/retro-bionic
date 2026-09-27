#!/usr/bin/env python3
"""Regression suite for the Bionic debugger board.

Target agnostic: the `?` banner names the target, and that name picks the
sample directory (`samples/<target>/`).  Every case reports pass or fail
with the firmware timing parameters, and a case that leaves the board
stuck is recovered before the next one runs -- one bad case must not take
the rest of the suite with it.

Cases, run in this order unless named on the command line:

  reset    R reports the reset PC, repeatably
  step     S advances the PC through a loaded program
  samples  every samples/<target>/*.hex, each driven as it expects:
           input fed to the echo samples, iterations counted for one
           that loops for ever, run to completion for the rest
  haltgo   halt a running program with the halt port, then continue with
           G -- repeatedly.  This one exists because it caught a real bug
           that none of the others see: the continue appeared to work
           while the PC had been corrupted, so a later continue ran
           garbage.  A continue counts as passing only if output flows
           again *and* the PC stayed inside the program.

  bionic-regress.py                 # every case
  bionic-regress.py haltgo          # one case
  bionic-regress.py haltgo=20       # with a repeat count
"""
import json
import os
import re
import sys
import time
from importlib.machinery import SourceFileLoader

HERE = os.path.dirname(os.path.abspath(__file__))
bc = SourceFileLoader('bc', os.path.join(HERE, 'bionic-control.py')).load_module()
PROJ = bc.PROJ

# Timing constants worth recording alongside a result; absent ones are skipped.
PARAM_KEYS = ('xtali_lo_ns', 'xtali_hi_ns', 'clk_delay_ns', 'addr_delay_ns',
              'status_delay_ns', 'reset_lo_ns', 'reset_hi_ns')


def params(target):
    """The timing constants the firmware was built with, for the log."""
    src = os.path.join(PROJ, 'debugger', target.lower(),
                       'pins_%s.cpp' % target.lower())
    try:
        text = open(src).read()
    except OSError:
        return ''
    out = []
    for k in PARAM_KEYS:
        m = re.search(r'constexpr auto %s = (\d+);' % k, text)
        if m:
            out.append('%s=%s' % (k[:-3], m.group(1)))
    return ' '.join(out)


def samples(target):
    d = os.path.join(PROJ, 'samples', target.lower())
    if not os.path.isdir(d):
        return []
    return sorted(os.path.join(d, f) for f in os.listdir(d)
                  if f.endswith('.hex'))


def regs(fd):
    """The register line, or None if the CLI did not answer."""
    txt = bc._drain(fd, 8.0, 0.8).decode('ascii', 'replace').replace('\r', '')
    for line in txt.split('\n'):
        if line.startswith(('PC=', 'IP=')):
            return line
    return None


def ask_regs(fd):
    os.write(fd, b'r')
    return regs(fd)


def pc_of(line):
    return line.split()[0].split('=')[1] if line else None


def ensure_prompt(fd, what):
    if bc.identify(fd) is not None:
        return True
    print('  ! %s left the board stuck; recovering' % what)
    return bc.recover(fd) is not None


# --------------------------------------------------------------------- cases
def case_reset(fd, target, n=4):
    """R must report the same reset PC every time."""
    seen = []
    for _ in range(n):
        os.write(fd, b'R')
        seen.append(pc_of(regs(fd)))
    ok = len(set(seen)) == 1 and seen[0] is not None
    return ok, 'reset PC %s' % (seen[0] if ok else seen)


def case_step(fd, target, n=4):
    """S must advance the PC, and never repeat or go nowhere."""
    hexes = samples(target)
    if not hexes:
        return None, 'no samples to step through'
    os.write(fd, b'R')
    regs(fd)
    bc.upload_file(fd, hexes[0])
    os.write(fd, b'R')
    start = pc_of(regs(fd))
    seen = []
    for _ in range(n):
        os.write(fd, b'S')
        seen.append(pc_of(regs(fd)))
    ok = all(seen) and len(set(seen)) == len(seen)
    return ok, 'from %s: %s' % (start, ' '.join(str(s) for s in seen))


# How each sample has to be driven.  Running them all the same way is wrong:
# the echo samples block waiting for input and would look like failures, and
# mandelbrot never ends on its own.  Keyed by basename, which the sample sets
# share across targets.
DRIVE = {
    'echo': dict(feed=b'ok\r', expect='ok'),
    'echoir': dict(feed=b'irq\r', expect='irq'),
    'echoitr': dict(feed=b'z\r', expect='0b01111010'),
    'mandelbrot': dict(frames=1),
    # prints nothing and takes about a minute to reach its break
    'mmu': dict(silent=True, cap=180.0),
    # anything else: run to completion and expect some output
}


def drive_table(target):
    """DRIVE plus samples/<target>/regress.json, whose entries override it.

    The file holds the same keys: {"name": {"feed": "irq\r", "expect": "irq"}},
    {"frames": 1} or {"silent": true, "cap": 180}. A target keeps its own
    samples' drives there instead of here.
    """
    table = dict(DRIVE)
    path = os.path.join(PROJ, 'samples', target.lower(), 'regress.json')
    if os.path.exists(path):
        for name, how in json.load(open(path)).items():
            how = dict(how)
            if 'feed' in how:
                how['feed'] = how['feed'].encode('latin-1')
            table[name] = how
    return table


def drive_sample(fd, target, path):
    """Run one sample the way it expects; returns (ok, note)."""
    name = os.path.basename(path).rsplit('.', 1)[0]
    how = drive_table(target).get(name, {})
    os.write(fd, b'R')
    regs(fd)
    bc.upload_file(fd, path)

    if 'feed' in how:
        os.write(fd, b'G')
        time.sleep(1.0)
        bc._drain(fd, 2.0, 0.5)
        got = ''
        for ch in how['feed']:
            os.write(fd, bytes([ch]))
            time.sleep(0.3)
            got += bc._drain(fd, 2.0, 0.5).decode('ascii', 'replace')
        os.write(fd, b'\x00')            # the samples exit on NUL
        tail = bc._drain(fd, 10.0, 1.0).decode('ascii', 'replace')
        ok = how['expect'] in got.replace('\r', '')
        return ok, '%s echoed %r' % ('' if ok else 'expected %r,' % how['expect'],
                                     got.replace('\r', '')[:32])

    if how.get('frames'):
        os.write(fd, b'G')
        state, raw, marks = bc.wait_run(fd, cap=300.0, frames=how['frames'],
                                        out=None)
        bc.abort()
        time.sleep(0.8)
        bc._drain(fd, 8.0, 1.0)
        ok = bool(marks)
        return ok, ('%d iteration(s), %.2fs each' % (len(marks), marks[-1] / len(marks))
                    if marks else 'no iteration completed (%s)' % state)

    if how.get('silent'):
        # No output until the break, so a stall means nothing; only
        # reaching the prompt within the cap does.
        os.write(fd, b'G')
        state, raw, _ = bc.wait_run(fd, cap=how['cap'], stall=how['cap'], out=None)
        if state != 'prompt':
            bc.abort()
            time.sleep(0.8)
            bc._drain(fd, 8.0, 1.0)
        return state == 'prompt', '%s, silent' % state

    os.write(fd, b'G')
    state, raw, _ = bc.wait_run(fd, cap=120.0, out=None)
    if state != 'prompt':
        bc.abort()
        time.sleep(0.8)
        bc._drain(fd, 8.0, 1.0)
    txt = raw.decode('ascii', 'replace')
    ok = state == 'prompt' and txt.count('\n') > 2
    return ok, '%s, %d lines' % (state, txt.count('\n'))


def case_samples(fd, target, n=0):
    """Every sample in samples/<target>/ must run and behave."""
    hexes = samples(target)
    if not hexes:
        return None, 'no samples'
    bad = []
    for path in hexes:
        name = os.path.basename(path)
        if not ensure_prompt(fd, name):
            bad.append('%s(stuck before)' % name)
            print('    %-16s SKIP  board stuck' % name)
            continue
        ok, note = drive_sample(fd, target, path)
        if not ok:
            bad.append(name)
        print('    %-16s %-4s %s' % (name, 'ok' if ok else 'BAD', note))
        sys.stdout.flush()
    return not bad, ('%d samples' % len(hexes) if not bad
                     else 'failed: %s' % ', '.join(bad))


def case_haltgo(fd, target, n=10):
    """Halt a run with the halt port, continue with G, and check both.

    The PC has to be checked, not just that output resumed: a corrupted
    resume still emits bytes for a while, and only a later continue fails.
    """
    hexes = [p for p in samples(target) if 'mandel' in os.path.basename(p)]
    if not hexes:
        hexes = samples(target)
    if not hexes:
        return None, 'no samples'
    path = hexes[0]
    os.write(fd, b'R')
    regs(fd)
    bc.upload_file(fd, path)
    lo, hi = 0x0000, 0x8000          # a sane PC stays in the loaded image
    os.write(fd, b'G')
    time.sleep(2.0)
    bc._drain(fd, 3.0, 1.0)
    bad, drift = 0, []
    for i in range(n):
        bc.abort()
        time.sleep(0.6)
        halted = bc._drain(fd, 8.0, 1.0).decode('ascii', 'replace')
        line = None
        for ln in halted.replace('\r', '').split('\n'):
            if ln.startswith(('PC=', 'IP=')):
                line = ln
        pc = pc_of(line)
        if not bc.at_prompt(halted.encode('ascii', 'replace')):
            if not bc.recover(fd):
                bad += 1
                drift.append('%d:unrecovered' % i)
                break
        inside = pc is not None and lo <= int(pc, 16) < hi
        os.write(fd, b'G')
        time.sleep(2.0)
        got = len(bc._drain(fd, 4.0, 1.2))
        if not inside or got < 200:
            bad += 1
            drift.append('%d:PC=%s,%dB' % (i, pc, got))
    bc.abort()
    time.sleep(0.8)
    bc._drain(fd, 6.0, 1.0)
    return bad == 0, ('%d/%d halt+continue ok' % (n - bad, n)
                      + ('; %s' % ' '.join(drift) if drift else ''))


def live_pc(fd, secs=1.5):
    """An address the program was actually executing.

    Discovered rather than hardcoded, so the case stays target agnostic: a
    halt lands on a real instruction boundary inside whatever loop the
    program is in, which is exactly what a breakpoint needs.
    """
    os.write(fd, b'G')
    time.sleep(secs)
    bc._drain(fd, 2.0, 0.6)
    bc.abort()
    time.sleep(0.6)
    txt = bc._drain(fd, 8.0, 1.0).decode('ascii', 'replace').replace('\r', '')
    line = None
    for ln in txt.split('\n'):
        if ln.startswith(('PC=', 'IP=')):
            line = ln
    return pc_of(line)


def break_list(fd):
    """The breakpoint list, with the 'clear?' prompt cancelled."""
    os.write(fd, b'b')
    txt = bc._drain(fd, 6.0, 0.8).decode('ascii', 'replace').replace('\r', '')
    if 'clear?' in txt:
        os.write(fd, b'\x03')          # cancel: an empty line clears index 0
        txt += bc._drain(fd, 4.0, 0.6).decode('ascii', 'replace')
    return txt


def clear_breaks(fd, tries=6):
    for _ in range(tries):
        os.write(fd, b'b')
        txt = bc._drain(fd, 6.0, 0.8).decode('ascii', 'replace')
        if 'clear?' not in txt:
            return True
        os.write(fd, b'0\r')
        bc._drain(fd, 4.0, 0.6)
    return False


def set_break(fd, addr):
    os.write(fd, b'B' + addr.encode() + b'\r')
    return bc._drain(fd, 6.0, 0.8).decode('ascii', 'replace').replace('\r', '')


def run_until_stop(fd, cap=30.0):
    """G, then wait for the CLI to come back; returns (pc, text)."""
    os.write(fd, b'G')
    how, raw, _ = bc.wait_run(fd, cap=cap, out=None)
    txt = raw.decode('ascii', 'replace').replace('\r', '')
    if how != 'prompt':
        bc.abort()
        time.sleep(0.8)
        txt += bc._drain(fd, 8.0, 1.0).decode('ascii', 'replace')
        return None, txt
    line = None
    for ln in txt.split('\n'):
        if ln.startswith(('PC=', 'IP=')):
            line = ln
    return pc_of(line), txt


def _load_looping(fd, target, prefer=('echo', 'mandel')):
    """Load a sample that loops, so a breakpoint in it is reached again.

    `echo` first: it sits in a tight polling loop, so almost any address in
    it recurs within milliseconds.  A drawing sample passes through each of
    its addresses once per frame, which makes a breakpoint there slow to
    confirm and easy to miss.
    """
    hexes = []
    for want in prefer:
        hexes += [p for p in samples(target) if want in os.path.basename(p)]
    hexes = hexes or samples(target)
    if not hexes:
        return None
    os.write(fd, b'R')
    regs(fd)
    bc.upload_file(fd, hexes[0])
    return hexes[0]


def case_break(fd, target, n=3):
    """Stop at a breakpoint, continue, and stop there again.

    Continuing has to hit the *same* breakpoint n times running: a
    breakpoint that is consumed on the first hit, or whose patched opcode is
    not put back, passes a single-hit test and fails this one.
    """
    if _load_looping(fd, target) is None:
        return None, 'no samples'
    clear_breaks(fd)
    # The address cannot simply be taken from a halt: the PC a halt reports
    # is itself unreliable, so a bad one makes this case fail for a reason
    # that has nothing to do with breakpoints.  Take a candidate, prove a
    # breakpoint there is reached at all, and only then test re-hitting it.
    addr, tried = None, []
    for _ in range(5):
        cand = live_pc(fd)
        if cand is None or cand in tried:
            continue
        tried.append(cand)
        clear_breaks(fd)
        if 'set' not in set_break(fd, cand):
            continue
        pc, _txt = run_until_stop(fd, cap=20.0)
        if pc is not None and pc.lstrip('0') == cand.lstrip('0'):
            addr = cand
            break
    if addr is None:
        clear_breaks(fd)
        return False, ('no halt-derived address was ever reached: tried %s '
                       '-- the halt PC, not the breakpoint, is suspect'
                       % ' '.join(tried))
    hits, notes = 1, []
    for i in range(1, n):                # the proving hit above counts as one
        pc, txt = run_until_stop(fd)
        if pc is None:
            notes.append('%d:no stop' % i)
            break
        if pc.lstrip('0') != addr.lstrip('0'):
            notes.append('%d:stopped at %s' % (i, pc))
            break
        hits += 1
        if addr.lstrip('0') not in break_list(fd).replace(' ', '').upper():
            notes.append('%d:breakpoint gone from the list' % i)
            break
    clear_breaks(fd)
    ok = hits == n and not notes
    return ok, 'break at %s hit %d/%d%s' % (addr, hits, n,
                                            '; ' + ' '.join(notes) if notes else '')


def case_gountil(fd, target, n=2):
    """`g` runs to a one-shot address, and leaves no breakpoint behind.

    It sets a *temp* breakpoint, so afterwards the list must be empty --
    otherwise a go-until silently leaves a trap in the program.
    """
    if _load_looping(fd, target) is None:
        return None, 'no samples'
    clear_breaks(fd)
    addr = live_pc(fd)
    if addr is None:
        return False, 'could not find a live PC to run to'
    notes = []
    for i in range(n):
        os.write(fd, b'g' + addr.encode() + b'\r')
        how, raw, _ = bc.wait_run(fd, cap=30.0, out=None)
        txt = raw.decode('ascii', 'replace').replace('\r', '')
        if how != 'prompt':
            bc.abort()
            time.sleep(0.8)
            bc._drain(fd, 8.0, 1.0)
            notes.append('%d:no stop' % i)
            break
        line = None
        for ln in txt.split('\n'):
            if ln.startswith(('PC=', 'IP=')):
                line = ln
        pc = pc_of(line)
        if pc is None or pc.lstrip('0') != addr.lstrip('0'):
            notes.append('%d:stopped at %s' % (i, pc))
            break
        left = break_list(fd)
        if addr.lstrip('0') in left.replace(' ', '').upper():
            notes.append('%d:temp breakpoint left behind' % i)
            break
    clear_breaks(fd)
    return not notes, 'go until %s, %d times%s' % (
        addr, n, '; ' + ' '.join(notes) if notes else '')


CASES = (('reset', case_reset), ('step', case_step),
         ('samples', case_samples), ('haltgo', case_haltgo),
         ('break', case_break), ('gountil', case_gountil))


def main():
    want = {}
    for a in sys.argv[1:]:
        name, _, cnt = a.partition('=')
        want[name] = int(cnt) if cnt else None
    fd, target = bc.open_board()
    print('target %s   %s' % (target, params(target)))
    failed, skipped = [], []
    for name, fn in CASES:
        if want and name not in want:
            continue
        if not ensure_prompt(fd, 'previous case'):
            print('%-8s SKIP  board will not return to its prompt' % name)
            failed.append(name)
            continue
        t0 = time.time()
        kw = {} if want.get(name) is None else {'n': want[name]}
        ok, note = fn(fd, target, **kw)
        took = time.time() - t0
        mark = 'SKIP' if ok is None else ('ok' if ok else 'FAIL')
        print('%-8s %-4s %6.1fs  %s' % (name, mark, took, note))
        if ok is False:
            failed.append(name)
        elif ok is None:
            skipped.append(name)
    os.close(fd)
    # A skip is not a pass: say so, so an empty suite cannot read as green.
    summary = 'FAILED: %s' % ' '.join(failed) if failed else \
        'passed' if not skipped else 'passed, but SKIPPED: %s' % ' '.join(skipped)
    if skipped and not failed:
        summary += '  (set BIONIC_PROJ to the tree holding samples/%s/)' % target.lower()
    print('\n%s' % summary)
    sys.exit(1 if failed else 0)


if __name__ == '__main__':
    main()
