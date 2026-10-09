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
  sample:name[,name...]
           just those samples, by their file names without .hex
  haltgo   halt a running program with the halt port, then continue with
           G -- repeatedly.  This one exists because it caught a real bug
           that none of the others see: the continue appeared to work
           while the PC had been corrupted, so a later continue ran
           garbage.  A continue counts as passing only if output flows
           again *and* the PC stayed inside the program.
  breakpoint  a persistent B at a named label, hit n times running,
           from [breakpoint] in the regress.toml file; needs
           program/break_at there, so it is a no-op without them
  gountil  `g` a named label repeatedly, from [gountil] in the
           regress.toml file; needs program/break_at there too

A regress.toml-style file (see drive_table()) is mandatory and always
first: which arch/variant this run exercises is not inferred from the
target's own banner, it comes entirely from that file's own name (e.g.
samples/<target>/bench/regress_<variant>.toml) and from the human's
choice of which one to name. Its top-level `chips` lists the chips it is
for, as their banners name them; on any other chip the run is refused,
naming the files that are for it. At least one case must be named too; a
file with none names what it drives instead of guessing "every case"
is meant.

  bionic-regress.py REGRESS.toml
       # no case named: lists the cases and the file's own samples,
       # runs nothing
  bionic-regress.py REGRESS.toml reset step samples haltgo breakpoint gountil disasm
       # every case, named explicitly
  bionic-regress.py REGRESS.toml haltgo     # one case
  bionic-regress.py REGRESS.toml haltgo=20  # with a repeat count
  bionic-regress.py samples/<target>/bench/regress_<other-variant>.toml samples
       # samples case, for a different board configuration of the same
       # CPU -- only a human can switch between those, so the file to
       # use is always named explicitly.
"""
import glob
import os
import random
import re
import sys
import time
import tomllib
from importlib.machinery import SourceFileLoader

HERE = os.path.dirname(os.path.abspath(__file__))
bc = SourceFileLoader('bc', os.path.join(HERE, 'bionic-control.py')).load_module()
mt = SourceFileLoader('mt', os.path.join(HERE, 'mandelbrot-timing.py')).load_module()
PROJ = bc.PROJ

# Shortest run a `lines` sample gets unless it completes a frame first.
MIN_RUN = 20.0

# The dump's label for the program counter, from [reset]'s
# program_counter -- set once in main(); None keeps bc's PC default.
PROGRAM_COUNTER = None

# The radix the target prints and reads addresses in, from [reset]'s
# radix -- set once in main(); 8 for the PDP-8.
RADIX = 16


def pc_from(buf):
    return bc.pc_from(buf, PROGRAM_COUNTER)


def address(text):
    return int(text, RADIX)


# The samples the regress file covers: samples/<dir>/ for a file at
# samples/<dir>/bench/*.toml -- set once in main(). The board's own name
# need not match <dir> (a P8051 runs samples/i8051), so it is only the
# fallback for a file kept elsewhere.
SAMPLES_DIR = None


def samples_dir_of(regress):
    bench = os.path.dirname(os.path.abspath(regress))
    if os.path.basename(bench) != 'bench':
        return None
    d = os.path.dirname(bench)
    return d if os.path.basename(os.path.dirname(d)) == 'samples' else None


def samples(target):
    d = SAMPLES_DIR or os.path.join(PROJ, 'samples', target.lower())
    if not os.path.isdir(d):
        return []
    return sorted(os.path.join(d, f) for f in os.listdir(d)
                  if f.endswith(('.hex', '.s19', '.s28', '.s37')))


def label_address(lst_path, label):
    """The address `label` is defined at, in an assembler listing:
    `label:`, or `label,` in PDP-8 syntax.

    Looked up there rather than hand-typed into regress.toml, so a
    rebuild that moves the label cannot leave a stale address silently
    wrong.
    """
    pattern = re.compile(r'^\(?\d*\)?\s*([0-9A-Fa-f]+)\s*:\s*%s[:,]' % re.escape(label))
    for line in open(lst_path):
        m = pattern.match(line)
        if m:
            return m.group(1)
    return None


def _merge(spans):
    spans.sort()
    merged = []
    for lo, hi in spans:
        if merged and lo <= merged[-1][1]:
            merged[-1] = (merged[-1][0], max(merged[-1][1], hi))
        else:
            merged.append((lo, hi))
    return merged


def _intel_hex_ranges(path):
    spans = []
    base = 0
    for line in open(path):
        line = line.strip()
        if not line.startswith(':'):
            continue
        length = int(line[1:3], 16)
        addr = int(line[3:7], 16)
        rectype = int(line[7:9], 16)
        if rectype == 0 and length:
            spans.append((base + addr, base + addr + length))
        elif rectype == 2:               # Extended Segment Address
            base = int(line[9:13], 16) * 16
        elif rectype == 4:                # Extended Linear Address
            base = int(line[9:13], 16) << 16
    return spans


_SREC_ADDR_DIGITS = {'1': 4, '2': 6, '3': 8}  # S1/S2/S3: 16/24/32-bit address


def _srec_ranges(path):
    spans = []
    for line in open(path):
        line = line.strip()
        digits = _SREC_ADDR_DIGITS.get(line[1:2])
        if not line.startswith('S') or digits is None:
            continue
        count = int(line[2:4], 16)        # address + data + checksum bytes
        addr = int(line[4:4 + digits], 16)
        data_len = count - digits // 2 - 1
        if data_len > 0:
            spans.append((addr, addr + data_len))
    return spans


def load_ranges(path):
    """Merged (lo, hi) address spans an Intel HEX or Motorola S-record
    file's data records actually load, disjoint gaps kept as gaps, in
    the file's own byte-addressed terms.

    A gap matters: a reserved-but-uninitialized region (e.g. `org
    *+size`, never itself in a data record) has no code, so a single
    min/max span would wrongly call a PC landed there "inside the
    image" too. Byte-addressed only -- see case_haltgo()'s
    `address_unit` for a word/longword target's own PC.
    """
    first = next((l.strip() for l in open(path) if l.strip()), '')
    if first.startswith(':'):
        return _merge(_intel_hex_ranges(path))
    if first.startswith('S'):
        return _merge(_srec_ranges(path))
    return []


def image_bytes(path):
    """{address: byte} an Intel HEX or Motorola S-record file loads."""
    mem = {}
    base = 0
    for line in open(path):
        line = line.strip()
        if line.startswith(':'):
            length, addr, rectype = int(line[1:3], 16), int(line[3:7], 16), int(line[7:9], 16)
            if rectype == 0:
                for i in range(length):
                    mem[base + addr + i] = int(line[9 + 2 * i:11 + 2 * i], 16)
            elif rectype == 2:
                base = int(line[9:13], 16) * 16
            elif rectype == 4:
                base = int(line[9:13], 16) << 16
        elif line.startswith('S') and line[1:2] in _SREC_ADDR_DIGITS:
            digits = _SREC_ADDR_DIGITS[line[1:2]]
            count = int(line[2:4], 16)
            addr = int(line[4:4 + digits], 16)
            data = line[4 + digits:4 + digits + 2 * (count - digits // 2 - 1)]
            for i in range(len(data) // 2):
                mem[addr + i] = int(data[2 * i:2 * i + 2], 16)
    return mem


def skip_reason(how, who):
    """Why a sample's table `how` keeps it off chip `who`, or None."""
    if how.get('skip'):
        return 'skip = true'
    chips = how.get('chips')
    if chips and (who or '').upper() not in (c.upper() for c in chips):
        return 'only on %s' % ', '.join(chips)
    return None


def ensure_prompt(board, what):
    if board.identify() is not None:
        return True
    print('  ! %s left the board stuck; recovering' % what)
    return board.recover() is not None


# --------------------------------------------------------------------- cases
def case_reset(board, n=4, regress=None):
    """R must report the same reset PC every time.

    `[reset]` in `regress` also names registers R must reset to a
    documented value, e.g. `PC = "0000"` -- each checked as its own
    substring of the last repeat's dump, so column spacing or width
    doesn't matter.

    A CPU that fetches its PC from a reset vector names the vector's
    bytes instead, most significant first, e.g. `PC = "[FFFE FFFF]"`
    (big-endian) or `PC = "[FFFD FFFC]"` (6502): a sample is loaded
    (`program = "name"`, else the first one not skipped) and the PC
    must equal the value it puts there.
    """
    table = drive_table(regress)
    how = table.get('reset', {})
    settings = ('program_counter', 'program', 'radix')
    want = {k: v for k, v in how.items() if k not in settings}
    vectored = {k: v for k, v in want.items()
                if re.fullmatch(r'\[[0-9A-Fa-f]+( [0-9A-Fa-f]+)*\]', v)}
    if vectored:
        hexes = [p for p in samples(board.who)
                 if not skip_reason(table.get(os.path.basename(p).rsplit('.', 1)[0], {}),
                                    board.who)]
        if how.get('program'):
            hexes = [p for p in hexes
                     if os.path.basename(p).rsplit('.', 1)[0] == how['program']]
        if not hexes:
            return False, 'no sample to load for the reset vector'
        load(board, hexes[0])
        mem = image_bytes(hexes[0])
        for k, v in vectored.items():
            addrs = [int(a, 16) for a in v[1:-1].split()]
            unset = [a for a in addrs if a not in mem]
            if unset:
                return False, '%s: %s leaves %s unset' % (
                    k, os.path.basename(hexes[0]), ' '.join('%04X' % a for a in unset))
            want[k] = ''.join('%02X' % mem[a] for a in addrs)
    seen, last = [], b''
    for _ in range(n):
        last = board.send(b'R')
        seen.append(pc_from(last))
    ok = len(set(seen)) == 1 and seen[0] is not None
    txt = last.decode('ascii', 'replace').replace('\r', '')
    missing = [k for k, v in want.items() if '%s=%s' % (k, v) not in txt]
    ok = ok and not missing
    note = 'reset PC %s' % (seen[0] if seen and len(set(seen)) == 1 else seen)
    if missing:
        note += ', missing %s' % ' '.join('%s=%s' % (k, want[k]) for k in missing)
    return ok, note


def load(board, path):
    """Upload `path` and reset, so the PC starts from the loaded program's
    own reset vector -- the reset before the upload (which stops the CPU)
    takes whatever vector the previous program left in memory."""
    board.send(b'R')
    board.upload_file(path)
    return board.send(b'R')


def case_step(board, n=4, regress=None):
    """S must advance the PC, and never repeat or go nowhere.

    Steps the first sample not marked `skip = true` in `regress`: a
    skipped one (e.g. a ROM patch) is not a program to step through.
    """
    table = drive_table(regress)
    hexes = [p for p in samples(board.who)
             if not skip_reason(table.get(os.path.basename(p).rsplit('.', 1)[0], {}),
                                board.who)]
    if not hexes:
        return None, 'no samples to step through'
    start = pc_from(load(board, hexes[0]))
    seen = []
    for _ in range(n):
        seen.append(pc_from(board.send(b'S')))
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
    silent = true\ncap = 180, or skip = true for a sample this case
    cannot drive (e.g. one needing a device the default setup lacks).

    `lines = N` instead halts at the first completed frame, or once N
    drawn rows have arrived and MIN_RUN seconds have passed -- so a
    frame-drawing sample costs about the same time on any target. Either
    can pair with `golden`, a path to a reference output file shared
    across targets (e.g. samples/arith/mandelbrot.golden), checked for a
    match instead of a per-target `expect` string: with `lines`, all the
    output drawn so far must match it exactly, however far it got.

    `io = "SCI"` routes the console through that device (the I command)
    for this sample only, then back to whichever one was enabled before.

    `chips = [...]` runs a sample only on those chips, e.g. one using an
    instruction the others of the file lack; on the rest it is skipped.

    The top-level `chips` list, which chips_of() reads, is not a sample.
    """
    table = {}
    if regress:
        with open(regress, 'rb') as f:
            for name, how in tomllib.load(f).items():
                if name == 'chips':
                    continue
                how = dict(how)
                if 'feed' in how:
                    how['feed'] = how['feed'].encode('latin-1')
                table[name] = how
    return table


def chips_of(regress):
    """The chips `regress` is written for: its top-level `chips`, the
    names the board's banner reports, e.g. `chips = ["MC6809",
    "HD6309"]`. Mandatory: a run on any other chip is refused."""
    with open(regress, 'rb') as f:
        chips = tomllib.load(f).get('chips')
    if not chips:
        sys.exit('%s: no chips = [...] naming the chips it is for' % regress)
    return chips


def files_for(chip):
    """The regress files whose `chips` include |chip|."""
    out = []
    for path in sorted(glob.glob(os.path.join(PROJ, 'samples', '*', 'bench',
                                              'regress*.toml'))):
        with open(path, 'rb') as f:
            if chip.upper() in (c.upper() for c in tomllib.load(f).get('chips', [])):
                out.append(os.path.relpath(path, PROJ))
    return out


def select_io(board, name):
    """Enable console device `name` with the I command; returns the name
    of the device enabled before, or None if `name` did not take."""
    txt = board.send(b'I' + name.encode() + b'\r', wait=4.0, idle=0.5,
                     delay=0.3).decode('ascii', 'replace').replace('\r', '')
    # I lists the devices before and after: `NAME  DESC  at ADDR`, with
    # a trailing DISABLED on all but the enabled one.
    lists = txt.split('which?')
    def enabled(part):
        found = re.findall(r'^(\S+) .* at [0-9A-Fa-f]+$', part, re.M)
        return found[0] if found else None
    before = enabled(lists[0])
    after = enabled(lists[1]) if len(lists) > 1 else None
    if after is None or after.upper() != name.upper():
        return None
    return before


def drive_sample(board, path, regress=None):
    """Run one sample the way it expects; returns (ok, note)."""
    name = os.path.basename(path).rsplit('.', 1)[0]
    how = drive_table(regress).get(name, {})
    load(board, path)
    if 'io' not in how:
        return _drive_sample(board, how)
    before = select_io(board, how['io'])
    if before is None:
        return False, 'could not select %s' % how['io']
    try:
        return _drive_sample(board, how)
    finally:
        ensure_prompt(board, name)
        select_io(board, before)


def _drive_sample(board, how):

    if 'feed' in how:
        board.send(b'G\r', wait=2.0, idle=0.5, delay=1.0)
        got = ''
        for ch in how['feed']:
            got += board.send(bytes([ch]), wait=2.0, idle=0.5,
                              delay=0.3).decode('ascii', 'replace')
        tail = board.send(b'\x00', wait=10.0, idle=1.0).decode(  # the samples exit on NUL
                'ascii', 'replace')
        ok = how['expect'] in got.replace('\r', '')
        return ok, '%s echoed %r' % ('' if ok else 'expected %r,' % how['expect'],
                                     got.replace('\r', '')[:32])

    if how.get('lines'):
        # Halt at the first completed frame, or once `lines` drawn rows
        # have arrived and MIN_RUN seconds have passed, whichever comes
        # first: a fast target is checked on a whole frame, a slow one on
        # `lines` rows, and neither runs much longer than MIN_RUN.
        t0 = time.time()
        log = []
        state, raw, _ = board.go(lines=how['lines'] + 1, frames=1, out=None,  # + 'G's reply
                                 log=log)
        left = MIN_RUN - (time.time() - t0)
        if state == 'lines' and left > 0:
            more_state, more, _ = board.wait_run(cap=left, stall=left + 5, frames=1, out=None,
                                                 log=log)
            raw += more
            if more_state in ('prompt', 'frames'):
                state = more_state
        took = time.time() - t0
        board.abort()
        board.send(wait=8.0, idle=1.0, delay=0.8)
        txt = raw.decode('ascii', 'replace').replace('\r', '')
        drawn = txt.split('\n', 1)[1] if '\n' in txt else ''   # after 'G's reply
        rows = drawn.count('\n')
        ok = state in ('lines', 'frames', 'prompt') and (state == 'frames' or rows >= how['lines'])
        if ok and 'golden' in how:
            frame = open(os.path.join(PROJ, how['golden'])).read().strip('\n') + '\n\n'
            if state == 'prompt':
                # Ended on its own: the whole frame must be there.
                ok = frame.strip('\n') in drawn
            else:
                # Everything drawn so far, however many frames and
                # however far into the last row, matches the golden run.
                ok = (frame * (len(drawn) // len(frame) + 2)).startswith(drawn)
        program = os.path.basename(how.get('golden', 'mandelbrot')).split('.')[0]
        frame, avg, dev = mt.summary(*mt.timing(log).get(program, ([], [])), program)
        note = '%s after %.1fs, %d rows' % (
                {'prompt': 'ended', 'frames': 'frame done'}.get(state, 'halted'), took, rows)
        if avg:
            note += ', %ss/frame, %ss/row (sd %s)' % (frame, avg, dev)
        if 'golden' in how:
            note += ', ' + ('content matched' if ok else 'content mismatch')
        elif not ok:
            note += ' (%s)' % state
        return ok, note

    if how.get('frames'):
        log = []
        state, raw, marks = board.go(cap=how.get('cap', 300.0), frames=how['frames'], out=None,
                                     log=log)
        board.abort()
        board.send(wait=8.0, idle=1.0, delay=0.8)
        ok = bool(marks) or state == 'running'
        if ok and 'golden' in how:
            golden = open(os.path.join(PROJ, how['golden'])).read()
            ok = golden.strip('\n') in raw.decode('ascii', 'replace').replace('\r', '')
        if marks:
            note = '%d iteration(s), %.2fs each' % (len(marks), marks[-1] / len(marks))
        elif state == 'running':
            note = 'halted mid-run after %.1fs, no stall' % how.get('cap', 300.0)
        else:
            note = 'no iteration completed (%s)' % state
        if 'golden' in how:
            program = os.path.basename(how['golden']).split('.')[0]
            frame, avg, dev = mt.summary(*mt.timing(log).get(program, ([], [])), program)
            if avg:
                note += ', %ss/frame, %ss/row (sd %s)' % (frame, avg, dev)
            note += ', ' + ('content matched' if ok else 'content mismatch')
        return ok, note

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


def case_samples(board, n=0, regress=None, only=None):
    """Every sample in samples/<target>/ must run and behave -- or just
    those named in `only`, from `sample:name[,name...]`."""
    hexes = samples(board.who)
    if not hexes:
        return None, 'no samples'
    if only:
        names = {os.path.basename(p).rsplit('.', 1)[0]: p for p in hexes}
        unknown = sorted(set(only) - set(names))
        if unknown:
            # A misspelt name would otherwise run nothing and read as passed.
            return False, 'no sample %s; samples: %s' % (
                    ' '.join(unknown), ' '.join(sorted(names)))
        hexes = [names[name] for name in sorted(only)]
    table = drive_table(regress)
    bad, skipped = [], 0
    for path in hexes:
        name = os.path.basename(path)
        why = skip_reason(table.get(name.rsplit('.', 1)[0], {}), board.who)
        if why:
            print('    %-16s SKIP  %s' % (name, why))
            skipped += 1
            continue
        if not ensure_prompt(board, name):
            bad.append('%s(stuck before)' % name)
            print('    %-16s SKIP  board stuck' % name)
            continue
        ok, note = drive_sample(board, path, regress=regress)
        if not ok:
            bad.append(name)
        print('    %-16s %-4s %s' % (name, 'ok' if ok else 'BAD', note))
        sys.stdout.flush()
    ran = '%d samples' % (len(hexes) - skipped)
    if skipped:
        ran += ', %d skipped' % skipped
    return not bad, (ran if not bad else 'failed: %s' % ', '.join(bad))


def case_haltgo(board, n=None, regress=None):
    """Halt a run with the halt port, continue with G, and check both.

    The PC has to be checked, not just that output resumed: a corrupted
    resume still emits bytes for a while, and only a later continue fails.

    Defaults to the first samples/<target>/*.hex with 'mandel' in its
    name (needs a free-running loop to halt and resume repeatedly), or
    the first sample of any kind. `[haltgo]` in `regress` overrides:
    `program = "name"` a specific one (basename, no extension),
    `halt_count = N` how many times (an explicit `haltgo=N` on the
    command line still wins), `address_unit = N` bytes per PC unit for
    a word/longword-addressed target (default 1, byte-addressed), and
    `halt_interval = [min, max]` randomizes each halt's run time in
    seconds (default [0, 0], immediate) so a corrupted resume that only
    shows up mid-instruction gets the chance to.
    """
    how = drive_table(regress).get('haltgo', {})
    unit = how.get('address_unit', 1)
    interval_lo, interval_hi = how.get('halt_interval', (0.0, 0.0))
    if n is None:
        n = how.get('halt_count', 10)
    hexes = samples(board.who)
    if how.get('program'):
        hexes = [p for p in hexes
                 if os.path.basename(p).rsplit('.', 1)[0] == how['program']]
    else:
        hexes = [p for p in hexes if 'mandel' in os.path.basename(p)] or hexes
    if not hexes:
        return None, 'no samples'
    path = hexes[0]
    load(board, path)
    # a sane PC stays inside one of these, in the target's own units
    ranges = [(lo // unit, hi // unit) for lo, hi in load_ranges(path)]
    # 'G 0': no backtrace to disassemble and print at the next halt --
    # only the PC and a nonzero byte count are checked, so it would
    # only cost time here.
    board.send(b'G 0\r', wait=3.0, idle=1.0, delay=1.0)
    bad, drift, output = 0, [], 0
    for i in range(n):
        if interval_hi:
            # run for exactly this long: the idle timeout never ends it early
            run = random.uniform(interval_lo, interval_hi)
            output += len(board.send(wait=run, idle=run))
        board.abort()
        # The dump can lag the halt; a reply ends at the prompt anyway.
        halted = board.send(wait=3.0, idle=2.0, delay=0.2)
        pc = pc_from(halted)
        if not bc.at_prompt(halted):
            if not board.recover():
                bad += 1
                drift.append('%d:unrecovered' % i)
                break
        inside = pc is not None and any(lo <= address(pc) < hi for lo, hi in ranges)
        # the resume echoes at once; what the program prints counts below
        got = len(board.send(b'G 0\r', wait=0.3, idle=0.1))
        if not inside or not got:
            bad += 1
            drift.append('%d:PC=%s,%dB' % (i, pc, got))
    if interval_hi and not output:
        bad += 1
        drift.append('no output after a resume')
    board.abort()
    board.send(wait=6.0, idle=1.0, delay=0.8)
    return bad == 0, ('%d/%d halt+continue ok' % (n - bad, n)
                      + ('; %s' % ' '.join(drift) if drift else ''))


def run_until_stop(board, cap=30.0):
    """G 0 (no backtrace to disassemble and print), then wait for the
    CLI to come back; returns (pc, text)."""
    how, raw, _ = board.go(cmd=b'G 0\r', cap=cap, out=None)
    txt = raw.decode('ascii', 'replace').replace('\r', '')
    if how != 'prompt':
        board.abort()
        txt += board.send(wait=8.0, idle=1.0, delay=0.8).decode('ascii', 'replace')
        return None, txt
    return pc_from(txt), txt


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
    load(board, hexes[0])
    return hexes[0]


def _stopped_at(pc, addr, how):
    """Whether the reported PC is the stop at `addr`. `pc_offset` in the
    case's table covers a CPU whose PC register holds the address before
    the next instruction (SC/MP: pc_offset = -1)."""
    return pc is not None and address(pc) == address(addr) + how.get('pc_offset', 0)


def case_breakpoint(board, n=3, regress=None):
    """Stop repeatedly at a persistent, named breakpoint.

    Continuing has to hit the *same* breakpoint n times running: a
    breakpoint that is consumed on the first hit, or whose patched opcode is
    not put back, passes a single-hit test and fails this one.

    `[breakpoint]` in `regress`: `program = "name"` the sample to load,
    `break_at = "label"` looked up in its .lst so a rebuild that moves
    the label can't leave a stale address silently wrong, and optionally
    `pc_offset` (see _stopped_at()).
    """
    how = drive_table(regress).get('breakpoint', {})
    if 'program' not in how or 'break_at' not in how:
        return None, 'no [breakpoint] program/break_at configured'
    hexes = [p for p in samples(board.who)
             if os.path.basename(p).rsplit('.', 1)[0] == how['program']]
    if not hexes:
        return None, 'no samples'
    path = hexes[0]
    addr = label_address(path.rsplit('.', 1)[0] + '.lst', how['break_at'])
    if addr is None:
        return False, '%s not found in %s.lst' % (how['break_at'], how['program'])
    load(board, path)
    board.clear_breaks()
    if 'set' not in board.set_break(addr):
        return False, 'could not set a breakpoint at %s (%s)' % (how['break_at'], addr)
    hits, notes = 0, []
    for i in range(n):
        pc, _txt = run_until_stop(board)
        if pc is None:
            notes.append('%d:no stop' % i)
            break
        if not _stopped_at(pc, addr, how):
            notes.append('%d:stopped at %s' % (i, pc))
            break
        hits += 1
        if addr.lstrip('0') not in board.list_breaks().replace(' ', '').upper():
            notes.append('%d:breakpoint gone from the list' % i)
            break
    board.clear_breaks()
    ok = hits == n and not notes
    return ok, 'break at %s (%s) hit %d/%d%s' % (how['break_at'], addr, hits, n,
                                                  '; ' + ' '.join(notes) if notes else '')


def case_gountil(board, n=3, regress=None):
    """`g` runs repeatedly to a named breakpoint, and checks it lands there.

    `[gountil]` in `regress`: `program = "name"` the sample to load,
    `break_at = "label"` looked up in its .lst so a rebuild that moves
    the label can't leave a stale address silently wrong, and optionally
    `pc_offset` (see _stopped_at()).
    """
    how = drive_table(regress).get('gountil', {})
    if 'program' not in how or 'break_at' not in how:
        return None, 'no [gountil] program/break_at configured'
    hexes = [p for p in samples(board.who)
             if os.path.basename(p).rsplit('.', 1)[0] == how['program']]
    if not hexes:
        return None, 'no samples'
    path = hexes[0]
    addr = label_address(path.rsplit('.', 1)[0] + '.lst', how['break_at'])
    if addr is None:
        return False, '%s not found in %s.lst' % (how['break_at'], how['program'])
    load(board, path)
    board.clear_breaks()
    # A *temp* breakpoint, so afterwards the list must be empty --
    # otherwise a go-until silently leaves a trap in the program.
    notes = []
    for i in range(n):
        state, raw, _ = board.go(cmd=b'g' + addr.encode() + b' 0\r', cap=30.0, out=None)
        txt = raw.decode('ascii', 'replace').replace('\r', '')
        if state != 'prompt':
            board.abort()
            board.send(wait=8.0, idle=1.0, delay=0.8)
            notes.append('%d:no stop' % i)
            break
        pc = pc_from(txt)
        if not _stopped_at(pc, addr, how):
            notes.append('%d:stopped at %s' % (i, pc))
            break
        left = board.list_breaks()
        if addr.lstrip('0') in left.replace(' ', '').upper():
            notes.append('%d:temp breakpoint left behind' % i)
            break
    board.clear_breaks()
    return not notes, 'go until %s (%s), %d times%s' % (
        how['break_at'], addr, n, '; ' + ' '.join(notes) if notes else '')


DISASM = re.compile(r'^[0-9A-F]{3,8}: ([0-9A-F]{2,4} )+ +\S+')


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
    board.send(b'G\r', wait=0.5, idle=0.2, delay=1.5)
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
         ('breakpoint', case_breakpoint), ('gountil', case_gountil),
         ('disasm', case_disasm))


def main():
    args = sys.argv[1:]
    # Mandatory and first: which arch/variant this run is exercising is
    # not something to infer from the target's own banner -- it comes
    # entirely from this file (see drive_table()).
    if not args or not args[0].endswith('.toml'):
        sys.exit('usage: bionic-regress.py REGRESS.toml [case[=n] ...]\n'
                  '       sample:name[,name...] runs just those samples\n'
                  'cases: %s' % ' '.join(name for name, _ in CASES))
    regress = args.pop(0)
    if not args:
        # A file with nothing named to run is more likely a "what's in
        # here" check than a "run everything" request -- show what the
        # file itself drives instead of guessing which one it means.
        with open(regress, 'rb') as f:
            samples_in_file = ' '.join(k for k in tomllib.load(f) if k != 'chips')
        sys.exit('usage: bionic-regress.py %s case[=n] ...\n'
                  '       sample:name[,name...] runs just those samples\n'
                  'cases: %s\n'
                  'samples in %s: %s' % (regress, ' '.join(name for name, _ in CASES),
                                          regress, samples_in_file))
    want, only = {}, {}
    for a in args:
        spec, _, cnt = a.partition('=')
        name, _, sel = spec.partition(':')
        if name == 'sample' and sel:
            name = 'samples'  # sample:name reads as one
        want[name] = int(cnt) if cnt else None
        if sel:
            only[name] = [s for s in sel.split(',') if s]
    unknown = [name for name in want if name not in dict(CASES)]
    if set(only) - {'samples'}:
        sys.exit('only sample takes :name: %s' % ' '.join(sorted(set(only) - {'samples'})))
    if unknown:
        # A misspelt case would otherwise run nothing and read as passed.
        sys.exit('unknown case: %s\ncases: %s' % (
                ' '.join(unknown), ' '.join(name for name, _ in CASES)))
    global PROGRAM_COUNTER, RADIX, SAMPLES_DIR
    PROGRAM_COUNTER = drive_table(regress).get('reset', {}).get('program_counter')
    RADIX = drive_table(regress).get('reset', {}).get('radix', 16)
    SAMPLES_DIR = samples_dir_of(regress)
    chips = chips_of(regress)
    with bc.Board.open() as board:
        print('target %s' % board.who)
        if (board.who or '').upper() not in (c.upper() for c in chips):
            sys.exit('%s is for %s, not %s; files for %s: %s' % (
                    regress, ', '.join(chips), board.who, board.who,
                    ' '.join(files_for(board.who or '')) or 'none'))
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
            if name in ('reset', 'step', 'samples', 'haltgo', 'breakpoint', 'gountil'):
                kw['regress'] = regress
            if name in only:
                kw['only'] = only[name]
            try:
                ok, note = fn(board, **kw)
            except Exception as e:
                # One case's crash must not end the suite: it fails.
                ok, note = False, '%s: %s' % (type(e).__name__, e)
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
    if skipped and not failed and not samples(target):
        summary += '  (no samples found in %s)' % (
                SAMPLES_DIR or os.path.join(PROJ, 'samples', target.lower()))
    print('\n%s' % summary)
    sys.exit(1 if failed else 0)


if __name__ == '__main__':
    main()
