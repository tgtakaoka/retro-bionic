#!/usr/bin/env python3
"""Regression suite for the Bionic debugger board.

Target agnostic: the `?` banner names the target, and that name picks the
sample directory (`samples/<target>/`).  Every case reports pass or fail
with the firmware timing parameters, and a case that leaves the board
stuck is recovered before the next one runs -- one bad case must not take
the rest of the suite with it.

Cases, named on the command line and run in this order:

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

A regress.toml-style file (see drive_table()) is mandatory and always
first: which arch/variant this run exercises is not inferred from the
target's own banner, it comes entirely from that file's own name (e.g.
samples/<target>/bench/regress_<variant>.toml) and from the human's
choice of which one to name. At least one case must be named too; a
file with none names what it drives instead of guessing "every case"
is meant.

  bionic-regress.py REGRESS.toml
       # no case named: lists the cases and the file's own samples,
       # runs nothing
  bionic-regress.py REGRESS.toml reset step samples haltgo break gountil disasm
       # every case, named explicitly
  bionic-regress.py REGRESS.toml haltgo     # one case
  bionic-regress.py REGRESS.toml haltgo=20  # with a repeat count
  bionic-regress.py samples/<target>/bench/regress_<other-variant>.toml samples
       # samples case, for a different board configuration of the same
       # CPU -- only a human can switch between those, so the file to
       # use is always named explicitly.
"""
import os
import re
import sys
import time
import tomllib
from importlib.machinery import SourceFileLoader

HERE = os.path.dirname(os.path.abspath(__file__))
bc = SourceFileLoader('bc', os.path.join(HERE, 'bionic-control.py')).load_module()
PROJ = bc.PROJ


def samples(target):
    d = os.path.join(PROJ, 'samples', target.lower())
    if not os.path.isdir(d):
        return []
    return sorted(os.path.join(d, f) for f in os.listdir(d)
                  if f.endswith('.hex'))


def ensure_prompt(board, what):
    if board.identify() is not None:
        return True
    print('  ! %s left the board stuck; recovering' % what)
    return board.recover() is not None


# --------------------------------------------------------------------- cases
def case_reset(board, n=4):
    """R must report the same reset PC every time."""
    seen = []
    for _ in range(n):
        seen.append(bc.pc_from(board.send(b'R')))
    ok = len(set(seen)) == 1 and seen[0] is not None
    return ok, 'reset PC %s' % (seen[0] if ok else seen)


def case_step(board, n=4):
    """S must advance the PC, and never repeat or go nowhere."""
    hexes = samples(board.who)
    if not hexes:
        return None, 'no samples to step through'
    board.send(b'R')
    board.upload_file(hexes[0])
    start = bc.pc_from(board.send(b'R'))
    seen = []
    for _ in range(n):
        seen.append(bc.pc_from(board.send(b'S')))
    ok = all(seen) and len(set(seen)) == len(seen)
    return ok, 'from %s: %s' % (start, ' '.join(str(s) for s in seen))


def drive_table(regress=None):
    """|regress|, an explicit regress.toml-style file describing how each
    sample has to be driven. Running them all the same way is wrong: an
    echo sample blocks waiting for input and would look like a failure,
    and a drawing sample never ends on its own -- but which samples need
    that, and how, is not this script's business to assume. Different
    targets share sample names (echo/echoir/echoitr/mandelbrot are common
    across many, but not all, and not always driven identically), so
    that knowledge lives in the file, not a built-in default here.

    No default path: bionic-regress.py does not infer which arch/variant
    config applies from the target name, or guess a samples/<target>/
    bench/regress.toml location for it. That mapping -- which file goes
    with which board configuration -- lives in the file itself (its name
    carries the variant, see [[sample-variant-naming]]) and in the
    human's choice of which one to name on the command line (this
    script's own first argument): a target with more than one board
    configuration of the same CPU (e.g. cache enabled vs. disabled) keeps
    one regress.toml-style file per configuration, and switching between
    them means physically reconfiguring the board, which only a human can
    do. With no file named, every sample gets the same default: run to
    completion, expect some output.

    TOML rather than JSON so the file can carry comments -- worth it for
    a hand-maintained, per-target/variant file like this.

    Keyed by sample basename, one table per sample:
    [name]\nfeed = "irq\r"\nexpect = "irq", or frames = 1, or
    silent = true\ncap = 180.
    """
    table = {}
    if regress:
        with open(regress, 'rb') as f:
            for name, how in tomllib.load(f).items():
                how = dict(how)
                if 'feed' in how:
                    how['feed'] = how['feed'].encode('latin-1')
                table[name] = how
    return table


def drive_sample(board, path, regress=None):
    """Run one sample the way it expects; returns (ok, note)."""
    name = os.path.basename(path).rsplit('.', 1)[0]
    how = drive_table(regress).get(name, {})
    board.send(b'R')
    board.upload_file(path)

    if 'feed' in how:
        board.send(b'G', wait=2.0, idle=0.5, delay=1.0)
        got = ''
        for ch in how['feed']:
            got += board.send(bytes([ch]), wait=2.0, idle=0.5,
                              delay=0.3).decode('ascii', 'replace')
        tail = board.send(b'\x00', wait=10.0, idle=1.0).decode(  # the samples exit on NUL
                'ascii', 'replace')
        ok = how['expect'] in got.replace('\r', '')
        return ok, '%s echoed %r' % ('' if ok else 'expected %r,' % how['expect'],
                                     got.replace('\r', '')[:32])

    if how.get('frames'):
        state, raw, marks = board.go(cap=300.0, frames=how['frames'], out=None)
        board.abort()
        board.send(wait=8.0, idle=1.0, delay=0.8)
        ok = bool(marks)
        return ok, ('%d iteration(s), %.2fs each' % (len(marks), marks[-1] / len(marks))
                    if marks else 'no iteration completed (%s)' % state)

    if how.get('silent'):
        # No output until the break, so a stall means nothing; only
        # reaching the prompt within the cap does.
        state, raw, _ = board.go(cap=how['cap'], stall=how['cap'], out=None)
        if state != 'prompt':
            board.abort()
            board.send(wait=8.0, idle=1.0, delay=0.8)
        return state == 'prompt', '%s, silent' % state

    state, raw, _ = board.go(cap=120.0, out=None)
    if state != 'prompt':
        board.abort()
        board.send(wait=8.0, idle=1.0, delay=0.8)
    txt = raw.decode('ascii', 'replace')
    if 'expect' in how:
        # Verifies the sample's own computed output, not just that some
        # output appeared -- a wrong result must fail, not just silence.
        ok = state == 'prompt' and how['expect'] in txt.replace('\r', '')
        return ok, '%s, %s' % (state, 'matched' if ok else 'expected output not found')
    ok = state == 'prompt' and txt.count('\n') > 2
    return ok, '%s, %d lines' % (state, txt.count('\n'))


def case_samples(board, n=0, regress=None):
    """Every sample in samples/<target>/ must run and behave."""
    hexes = samples(board.who)
    if not hexes:
        return None, 'no samples'
    bad = []
    for path in hexes:
        name = os.path.basename(path)
        if not ensure_prompt(board, name):
            bad.append('%s(stuck before)' % name)
            print('    %-16s SKIP  board stuck' % name)
            continue
        ok, note = drive_sample(board, path, regress=regress)
        if not ok:
            bad.append(name)
        print('    %-16s %-4s %s' % (name, 'ok' if ok else 'BAD', note))
        sys.stdout.flush()
    return not bad, ('%d samples' % len(hexes) if not bad
                     else 'failed: %s' % ', '.join(bad))


def case_haltgo(board, n=10):
    """Halt a run with the halt port, continue with G, and check both.

    The PC has to be checked, not just that output resumed: a corrupted
    resume still emits bytes for a while, and only a later continue fails.
    """
    hexes = [p for p in samples(board.who) if 'mandel' in os.path.basename(p)]
    if not hexes:
        hexes = samples(board.who)
    if not hexes:
        return None, 'no samples'
    path = hexes[0]
    board.send(b'R')
    board.upload_file(path)
    lo, hi = 0x0000, 0x8000          # a sane PC stays in the loaded image
    board.send(b'G', wait=3.0, idle=1.0, delay=2.0)
    bad, drift = 0, []
    for i in range(n):
        board.abort()
        halted = board.send(wait=8.0, idle=1.0, delay=0.6)
        pc = bc.pc_from(halted)
        if not bc.at_prompt(halted):
            if not board.recover():
                bad += 1
                drift.append('%d:unrecovered' % i)
                break
        inside = pc is not None and lo <= int(pc, 16) < hi
        got = len(board.send(b'G', wait=4.0, idle=1.2, delay=2.0))
        if not inside or got < 200:
            bad += 1
            drift.append('%d:PC=%s,%dB' % (i, pc, got))
    board.abort()
    board.send(wait=6.0, idle=1.0, delay=0.8)
    return bad == 0, ('%d/%d halt+continue ok' % (n - bad, n)
                      + ('; %s' % ' '.join(drift) if drift else ''))


def live_pc(board, secs=1.5):
    """An address the program was actually executing.

    Discovered rather than hardcoded, so the case stays target agnostic: a
    halt lands on a real instruction boundary inside whatever loop the
    program is in, which is exactly what a breakpoint needs.
    """
    board.send(b'G', wait=2.0, idle=0.6, delay=secs)
    board.abort()
    txt = board.send(wait=8.0, idle=1.0, delay=0.6)
    return bc.pc_from(txt)


def run_until_stop(board, cap=30.0):
    """G, then wait for the CLI to come back; returns (pc, text)."""
    how, raw, _ = board.go(cap=cap, out=None)
    txt = raw.decode('ascii', 'replace').replace('\r', '')
    if how != 'prompt':
        board.abort()
        txt += board.send(wait=8.0, idle=1.0, delay=0.8).decode('ascii', 'replace')
        return None, txt
    return bc.pc_from(txt), txt


def _load_looping(board, prefer=('echo', 'mandel')):
    """Load a sample that loops, so a breakpoint in it is reached again.

    `echo` first: it sits in a tight polling loop, so almost any address in
    it recurs within milliseconds.  A drawing sample passes through each of
    its addresses once per frame, which makes a breakpoint there slow to
    confirm and easy to miss.
    """
    hexes = []
    for want in prefer:
        hexes += [p for p in samples(board.who) if want in os.path.basename(p)]
    hexes = hexes or samples(board.who)
    if not hexes:
        return None
    board.send(b'R')
    board.upload_file(hexes[0])
    return hexes[0]


def case_break(board, n=3):
    """Stop at a breakpoint, continue, and stop there again.

    Continuing has to hit the *same* breakpoint n times running: a
    breakpoint that is consumed on the first hit, or whose patched opcode is
    not put back, passes a single-hit test and fails this one.
    """
    if _load_looping(board) is None:
        return None, 'no samples'
    board.clear_breaks()
    # The address cannot simply be taken from a halt: the PC a halt reports
    # is itself unreliable, so a bad one makes this case fail for a reason
    # that has nothing to do with breakpoints.  Take a candidate, prove a
    # breakpoint there is reached at all, and only then test re-hitting it.
    addr, tried = None, []
    for _ in range(5):
        cand = live_pc(board)
        if cand is None or cand in tried:
            continue
        tried.append(cand)
        board.clear_breaks()
        if 'set' not in board.set_break(cand):
            continue
        pc, _txt = run_until_stop(board, cap=20.0)
        if pc is not None and pc.lstrip('0') == cand.lstrip('0'):
            addr = cand
            break
    if addr is None:
        board.clear_breaks()
        return False, ('no halt-derived address was ever reached: tried %s '
                       '-- the halt PC, not the breakpoint, is suspect'
                       % ' '.join(tried))
    hits, notes = 1, []
    for i in range(1, n):                # the proving hit above counts as one
        pc, txt = run_until_stop(board)
        if pc is None:
            notes.append('%d:no stop' % i)
            break
        if pc.lstrip('0') != addr.lstrip('0'):
            notes.append('%d:stopped at %s' % (i, pc))
            break
        hits += 1
        if addr.lstrip('0') not in board.list_breaks().replace(' ', '').upper():
            notes.append('%d:breakpoint gone from the list' % i)
            break
    board.clear_breaks()
    ok = hits == n and not notes
    return ok, 'break at %s hit %d/%d%s' % (addr, hits, n,
                                            '; ' + ' '.join(notes) if notes else '')


def case_gountil(board, n=2):
    """`g` runs to a one-shot address, and leaves no breakpoint behind.

    It sets a *temp* breakpoint, so afterwards the list must be empty --
    otherwise a go-until silently leaves a trap in the program.
    """
    if _load_looping(board) is None:
        return None, 'no samples'
    board.clear_breaks()
    addr = live_pc(board)
    if addr is None:
        return False, 'could not find a live PC to run to'
    notes = []
    for i in range(n):
        how, raw, _ = board.go(cmd=b'g' + addr.encode() + b'\r', cap=30.0, out=None)
        txt = raw.decode('ascii', 'replace').replace('\r', '')
        if how != 'prompt':
            board.abort()
            board.send(wait=8.0, idle=1.0, delay=0.8)
            notes.append('%d:no stop' % i)
            break
        pc = bc.pc_from(txt)
        if pc is None or pc.lstrip('0') != addr.lstrip('0'):
            notes.append('%d:stopped at %s' % (i, pc))
            break
        left = board.list_breaks()
        if addr.lstrip('0') in left.replace(' ', '').upper():
            notes.append('%d:temp breakpoint left behind' % i)
            break
    board.clear_breaks()
    return not notes, 'go until %s, %d times%s' % (
        addr, n, '; ' + ' '.join(notes) if notes else '')


DISASM = re.compile(r'^[0-9A-F]{4,6}: ([0-9A-F]{2} )+ +\S+')


def case_disasm(board, n=10):
    """A run's cycle dump is disassembled: one line per instruction.

    Verbose off, a looping sample, a short run and a halt: the dump that
    follows must hold at least n lines of the form `addr: bytes mnemonic`,
    which is what every target's disassembleCycles() prints and what a
    target that only prints raw cycles never does.
    """
    if _load_looping(board) is None:
        return None, 'no samples'
    txt = board.send(b'V', wait=2.0, idle=0.4).decode('ascii', 'replace')
    if 'Verbose ON' in txt:
        board.send(b'V', wait=2.0, idle=0.4)
    board.send(b'G', wait=0.5, idle=0.2, delay=1.5)
    board.abort()
    how, raw, _ = board.wait_run(cap=40.0, out=None)
    txt = raw.decode('ascii', 'replace').replace('\r', '')
    if how != 'prompt':
        return False, 'no prompt after the halt'
    lines = [l for l in txt.split('\n') if DISASM.match(l)]
    # The last such line is the register dump's own.
    got = len(lines) - 1
    return got >= n, '%d instructions disassembled in the dump (need %d)' % (got, n)


CASES = (('reset', case_reset), ('step', case_step),
         ('samples', case_samples), ('haltgo', case_haltgo),
         ('break', case_break), ('gountil', case_gountil),
         ('disasm', case_disasm))


def main():
    args = sys.argv[1:]
    # Mandatory and first: which arch/variant this run is exercising is
    # not something to infer from the target's own banner -- it comes
    # entirely from this file (see drive_table()).
    if not args or not args[0].endswith('.toml'):
        sys.exit('usage: bionic-regress.py REGRESS.toml [case[=n] ...]\n'
                  'cases: %s' % ' '.join(name for name, _ in CASES))
    regress = args.pop(0)
    if not args:
        # A file with nothing named to run is more likely a "what's in
        # here" check than a "run everything" request -- show what the
        # file itself drives instead of guessing which one it means.
        with open(regress, 'rb') as f:
            samples_in_file = ' '.join(tomllib.load(f))
        sys.exit('usage: bionic-regress.py %s case[=n] ...\n'
                  'cases: %s\n'
                  'samples in %s: %s' % (regress, ' '.join(name for name, _ in CASES),
                                          regress, samples_in_file))
    want = {}
    for a in args:
        name, _, cnt = a.partition('=')
        want[name] = int(cnt) if cnt else None
    with bc.Board.open() as board:
        print('target %s' % board.who)
        failed, skipped = [], []
        for name, fn in CASES:
            if want and name not in want:
                continue
            if not ensure_prompt(board, 'previous case'):
                print('%-8s SKIP  board will not return to its prompt' % name)
                failed.append(name)
                continue
            t0 = time.time()
            kw = {} if want.get(name) is None else {'n': want[name]}
            if name == 'samples':
                kw['regress'] = regress
            ok, note = fn(board, **kw)
            took = time.time() - t0
            mark = 'SKIP' if ok is None else ('ok' if ok else 'FAIL')
            print('%-8s %-4s %6.1fs  %s' % (name, mark, took, note))
            if ok is False:
                failed.append(name)
            elif ok is None:
                skipped.append(name)
        target = board.who
    # A skip is not a pass: say so, so an empty suite cannot read as green.
    summary = 'FAILED: %s' % ' '.join(failed) if failed else \
        'passed' if not skipped else 'passed, but SKIPPED: %s' % ' '.join(skipped)
    if skipped and not failed:
        summary += '  (set BIONIC_PROJ to the tree holding samples/%s/)' % target.lower()
    print('\n%s' % summary)
    sys.exit(1 if failed else 0)


if __name__ == '__main__':
    main()
