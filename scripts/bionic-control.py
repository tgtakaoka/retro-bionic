#!/usr/bin/env python3
"""Bench tool for the Bionic debugger board -- any target.

Nothing here is architecture specific: the target names itself in the `?`
banner (`* BionicZ280 * 1.0`), and that name selects the only table that
differs between targets, the bus status decode.

Subcommands
  flash            build and upload, retrying (boards differ: some take
                   the first attempt, some reliably fail it)
  reset [cmd...]   send V (verbose on) then the commands, default R;
                   print a compact summary of the cycle dump
  send CMDS        send raw command characters, print the reply
  full             like reset but print every cycle line
  upload F [cmd..] inject an Intel HEX / S-record file with the U command,
                   then optionally send more commands in the same session
  run [cap [n]]    send G and wait for the run to finish; waits as long as
                   it keeps producing output, so a target that needs tens of
                   minutes is fine.  An optional cap in seconds bounds it;
                   without one, only a stall (BIONIC_STALL, default 45s of
                   silence) ends the wait early.  `n` stops after n completed
                   iterations -- the blank line a redrawing sample prints
                   between them -- and reports the time each one took, which
                   is the honest way to compare a slow target with a fast
                   one
  abort            abort a running CPU (any byte on the halt port)
  probe            report whether the board is at its prompt, and as what
  recover          bring a stuck board back to its prompt
  logic            export the newest Logic 2 capture and summarise it

Every board operation has a hard deadline and every one of them ends by
classifying the board (see `state`), so a run that never comes back is
reported as a failure instead of hanging the caller.

Full text always lands in $BIONIC_OUT for a follow-up look without
re-running the board.
"""
import collections
import fcntl
import json
import os
import re
import select
import subprocess
import sys
import time
import urllib.request

PORT = os.environ.get('BIONIC_PORT', '/dev/ttyACM0')
# The second USB serial is the halt switch: main.cpp's serialEventUSB1()
# fires Pins::isrHaltSwitch() on any byte, which loop() checks each step.
# It is the only way out of a run that does not come back on its own.
HALT_PORT = os.environ.get('BIONIC_HALT_PORT', '/dev/ttyACM1')
# The repository this script lives in, so a worktree needs no edit here.
PROJ = os.environ.get(
    'BIONIC_PROJ', os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ENV = os.environ.get('BIONIC_ENV', 'teensy41')
OUT = os.environ.get('BIONIC_OUT', '/tmp/bionic-last.txt')
MCP = os.environ.get('BIONIC_MCP', 'http://127.0.0.1:10530')
LOCK = os.environ.get('BIONIC_LOCK', '/tmp/bionic-board.lock')
# A run is judged by progress, not by elapsed time: STALL is how long the
# console may stay silent before a run counts as stuck.  It has to exceed the
# gap between two lines of output on the slowest target, not the fastest.
STALL = float(os.environ.get('BIONIC_STALL', '45'))

# The CLI prompt, with the newline that precedes it.  All three parts matter:
# a sample's own output can end in '>' -- arith prints comparisons like
# '30 > -48' -- and a read can split immediately after either the '>' or the
# space that follows it, so only the preceding newline tells the prompt apart
# from a line still being written.
PROMPT = b'\n> '
BANNER = re.compile(r'\* Bionic(\S*) \* (\S+)')


# ------------------------------------------------------------------ lock
_lockfd = None


def acquire(timeout=0.0):
    """Take the board lock, so only one test drives the hardware at a time.

    This is a physical device, not a service: two sessions talking to it
    interleave their commands and produce results that look like flaky
    hardware.  The lock is an flock, so it is released even if the holder
    is killed, and it is re-entrant within one process.
    """
    global _lockfd
    if _lockfd is not None:
        return True
    fd = os.open(LOCK, os.O_CREAT | os.O_RDWR, 0o666)
    end = time.time() + timeout
    while True:
        try:
            fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            if time.time() >= end:
                try:
                    held = os.pread(fd, 32, 0).decode().strip() or '?'
                except OSError:
                    held = '?'
                os.close(fd)
                sys.exit('board is in use by pid %s (%s)' % (held, LOCK))
            time.sleep(0.2)
            continue
        os.ftruncate(fd, 0)
        os.pwrite(fd, b'%d\n' % os.getpid(), 0)
        _lockfd = fd
        return True


def release():
    global _lockfd
    if _lockfd is not None:
        fcntl.flock(_lockfd, fcntl.LOCK_UN)
        os.close(_lockfd)
        _lockfd = None


# ---------------------------------------------------------------- serial
def _open(timeout=20):
    acquire(float(os.environ.get('BIONIC_LOCK_WAIT', '0')))
    end = time.time() + timeout
    while time.time() < end:
        try:
            fd = os.open(PORT, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
            os.system('stty -F %s raw -echo' % PORT)
            return fd
        except OSError:
            time.sleep(0.5)
    sys.exit('%s never appeared' % PORT)


def _drain(fd, secs, idle=2.0):
    """Read until `idle` seconds of quiet, or `secs` in total -- never longer.

    The total is a hard deadline on purpose: a failed run emits nothing at
    all while the CPU cycles refresh forever, so an idle-only wait would
    block for the whole budget on every later command.
    """
    buf, last, end = b'', time.time(), time.time() + secs
    while time.time() < end:
        r, _, _ = select.select([fd], [], [], 0.1)
        if r:
            try:
                d = os.read(fd, 65536)
                if d:
                    buf += d
                    last = time.time()
            except BlockingIOError:
                pass
        elif buf and time.time() - last > idle:
            break
    return buf


def at_prompt(buf):
    """True if `buf` ends at the CLI prompt rather than merely containing it."""
    return buf.replace(b'\r', b'').endswith(PROMPT)


# A sample that redraws in a loop separates its iterations with a blank line.
# Counting those is how a run that never ends on its own is both timed and
# brought to a stop after a known amount of work.
FRAME = b'\n\r\n'


def wait_run(fd, cap=0.0, stall=None, frames=0, progress=30.0, out=sys.stderr):
    """Wait out a run that may take a second or half an hour.

    `mandelbrot` finishes in a moment on a fast target and takes minutes, or
    tens of minutes, on a slow one -- while a *stuck* run produces nothing at
    all.  So the two are separated by progress rather than by a deadline: keep
    waiting as long as bytes keep arriving, and only call it stalled after
    `stall` seconds of silence with no prompt.

    Returns (how, text, marks) where marks holds the elapsed time of each
    completed iteration, and how is one of:
      'prompt'   the run ended and the CLI came back
      'frames'   the requested number of iterations completed
      'stalled'  silent for `stall` seconds and no prompt -- stuck
      'running'  still producing output when `cap` expired; for a sample that
                 loops until stopped this is the healthy outcome, not a failure

    `cap` of 0 means no overall limit: wait as long as it keeps working.
    """
    if stall is None:
        stall = STALL
    buf = b''
    marks = []
    t0 = last = time.time()
    said = t0
    seen = 0
    while True:
        r, _, _ = select.select([fd], [], [], 0.2)
        now = time.time()
        if r:
            try:
                d = os.read(fd, 65536)
            except BlockingIOError:
                d = b''
            if d:
                buf += d
                last = now
                # Count iteration boundaries as they arrive, so a slow target
                # is timed per iteration rather than only in total.
                n = buf.count(FRAME)
                while seen < n:
                    seen += 1
                    marks.append(now - t0)
                    if out is not None:
                        print('  iteration %d at %.1fs' % (seen, marks[-1]),
                              file=out)
                        out.flush()
                        said = now
                if at_prompt(buf):
                    return 'prompt', buf, marks
                if frames and seen >= frames:
                    return 'frames', buf, marks
        if now - said >= progress and out is not None:
            print('  ... %.0fs, %d bytes' % (now - t0, len(buf)), file=out)
            out.flush()
            said = now
        if now - last >= stall:
            return 'stalled', buf, marks
        if cap and now - t0 >= cap:
            return 'running', buf, marks


def abort():
    """Stop a running CPU. Harmless when it is not running."""
    acquire(float(os.environ.get('BIONIC_LOCK_WAIT', '0')))
    try:
        fd = os.open(HALT_PORT, os.O_WRONLY | os.O_NOCTTY | os.O_NONBLOCK)
    except OSError as e:
        print('halt port %s: %s' % (HALT_PORT, e))
        return False
    os.write(fd, b'x')
    os.close(fd)
    return True


# ------------------------------------------------------------ board state
def identify(fd, budget=6.0):
    """Return the target name if the CLI is at its prompt, else None.

    `?` is the right probe: it prints the banner straight from the CLI and
    never touches the CPU, so it answers even when a run has gone wrong.
    Both halves matter. The console must fall silent first, because a
    running sample keeps emitting and its output can contain anything; and
    the reply must *end* at the prompt, because output still draining from
    an earlier run satisfies a substring match on its own while only a CLI
    actually waiting for input prints `> ` last.
    """
    _drain(fd, 2.0, 0.4)                       # discard what is still coming
    if _drain(fd, 1.2, 1.2) != b'':            # still emitting: not a prompt
        return None
    os.write(fd, b'?')
    reply = _drain(fd, budget, 0.8)
    if not at_prompt(reply):
        return None
    m = BANNER.search(reply.decode('ascii', 'replace'))
    return m.group(1) or 'unknown' if m else None


def state(fd):
    """Classify the board: 'prompt', 'running' or 'wedged'.

    The three need different handling and look alike from a distance, so
    name them rather than reporting a bare failure:

      prompt   the CLI answered `?` -- ready for the next command
      running  the console is emitting, so the firmware is alive and it is
               the CPU that has not come back; the halt port will stop it
      wedged   silent and no prompt: the firmware itself is stuck in an
               unbounded wait, and only a reflash clears that
    """
    if identify(fd) is not None:
        return 'prompt'
    return 'running' if _drain(fd, 1.5, 1.5) != b'' else 'wedged'


def recover(fd, tries=3):
    """Work the ladder back to the prompt. Returns the target name or None.

    Ctrl-C cancels a half-finished CLI prompt, NUL is what the samples
    exit on, and the halt port stops a run that ignores both. Reflashing
    is deliberately *not* in here: it is the last resort, and the caller
    should decide to spend it.
    """
    for _ in range(tries):
        os.write(fd, b'\x03')                  # cancel a CLI prompt
        _drain(fd, 2.0, 0.5)
        os.write(fd, b'\x00')                  # the samples exit on NUL
        _drain(fd, 2.0, 0.6)
        who = identify(fd)
        if who is not None:
            return who
        abort()
        time.sleep(0.8)
        _drain(fd, 4.0, 0.8)
        who = identify(fd)
        if who is not None:
            return who
    return None


def open_board(budget=8.0):
    """Open the console and return (fd, target), recovering if need be."""
    fd = _open()
    time.sleep(1.0)
    who = identify(fd, budget)
    if who is None:
        who = recover(fd)
    if who is None:
        os.close(fd)
        sys.exit('board will not return to its prompt -- reflash needed')
    return fd, who


# ---------------------------------------------------------------- session
def upload_file(fd, path):
    """Inject one HEX/S-record file through the debugger's U command.

    U loops reading records until the line editor is cancelled, and the
    cancel key is Ctrl-C -- not ESC, which it simply ignores, so a session
    that sends the wrong one is left stuck in the loop and every later
    command is eaten as a malformed record.
    """
    os.write(fd, b'\x03')                      # leave any half-finished prompt
    _drain(fd, 1.5, 0.3)
    os.write(fd, b'U')
    _drain(fd, 3.0, 0.3)
    sent = 0
    for line in open(path):
        line = line.strip()
        if not line:
            continue
        os.write(fd, line.encode() + b'\r')
        _drain(fd, 3.0, 0.15)
        sent += 1
    os.write(fd, b'\x03')
    return sent, _drain(fd, 5.0, 0.5).decode('ascii', 'replace').replace('\r', '')


def converse(cmds, wait=40.0, verbose=True):
    fd, who = open_board()
    if verbose:                                # V toggles; make sure it is ON
        os.write(fd, b'V')
        time.sleep(0.4)
        if b'Verbose OFF' in _drain(fd, 1.5, 0.4):
            os.write(fd, b'V')
            time.sleep(0.4)
            _drain(fd, 1.5, 0.4)
    out = b''
    for c in cmds:
        os.write(fd, c.encode())
        chunk = _drain(fd, wait)
        if not at_prompt(chunk):
            # No prompt back. Say which of the two it is rather than
            # leaving the caller to guess from an empty reply.
            how = state(fd)
            print('after %r: %s' % (c, how), file=sys.stderr)
            if how != 'wedged' and recover(fd) is not None:
                chunk += _drain(fd, 5.0, 0.8)
            else:
                out += chunk
                break
        out += chunk
    os.close(fd)
    txt = out.decode('ascii', 'replace').replace('\r', '')
    open(OUT, 'w').write(txt)
    return txt, who


# ---------------------------------------------------------------- report
# Flags, index, direction, address and data are common to every target;
# the status nibble and the byte/word and read/write bits are printed only
# by those that have them.
CYC = re.compile(r'^([ic ]?[ic ]?)\s*(\d+)\s+([RW]) A=([0-9A-F]+) D=\s*([0-9A-F]+)'
                 r'(?: S=([0-9A-F]))?(?: b=(\d))?(?: r=(\d))?')

# Bus status decode, per target. Absent target: print the raw nibble.
STATUS = {
    'Z280': {0: 'Resv', 1: 'Refresh', 2: 'I/O', 3: 'HALT', 4: 'INTA-A',
             5: 'NMIA', 6: 'INTA-B', 7: 'INTA-C', 8: 'MEM', 9: 'MEMnc',
             10: 'EPUmem', 12: 'EPUopr', 13: 'EPUopc', 14: 'EPUcpu',
             15: 'LOCK'},
}


def status_names(target):
    return STATUS.get(target, {})


def parse(txt):
    out = []
    for line in txt.split('\n'):
        m = CYC.match(line)
        if m:
            f, n, rw, a, d, st, b, r = m.groups()
            out.append(dict(flag=f.strip(), n=int(n), rw=rw, addr=int(a, 16),
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


def report(txt, target='', head=14, full=False):
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
    mem = (8, 9) if c else None
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
    print('(full text: %s)' % OUT)


# ---------------------------------------------------------------- logic 2
def mcp(name, args):
    req = urllib.request.Request(
        MCP, method='POST',
        headers={'Content-Type': 'application/json',
                 'Accept': 'application/json, text/event-stream'},
        data=json.dumps({'jsonrpc': '2.0', 'id': 1, 'method': 'tools/call',
                         'params': {'name': name, 'arguments': args}}).encode())
    with urllib.request.urlopen(req, timeout=120) as r:
        body = r.read().decode()
    for line in body.splitlines():
        if line.startswith('data: '):
            body = line[6:]
    return json.loads(body)


def logic(directory='/tmp/bionic-logic'):
    for cid in range(60, -1, -1):              # newest id wins
        res = mcp('export_raw_data_csv',
                  {'captureId': cid, 'directory': directory,
                   'analogDownsampleRatio': 1})
        if 'does not exist' not in json.dumps(res):
            print('capture id %d -> %s/digital.csv' % (cid, directory))
            return os.path.join(directory, 'digital.csv')
    sys.exit('no Logic 2 capture found')


def logic_report(path, target=''):
    import csv
    name = status_names(target)
    r = csv.DictReader(open(path))
    f = r.fieldnames
    ST = [c for c in f if c.startswith('ST')] or ['Channel %d' % i
                                                  for i in range(4, 8)]
    AS = '#AS' if '#AS' in f else 'Channel 10'
    RS = '#RESET' if '#RESET' in f else 'Channel 13'
    WT = '#WAIT' if '#WAIT' in f else 'Channel 12'
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
    mem = [t for t, s in ev if s in (8, 9)]
    ref = [t for t, s in ev if s == 1]
    if mem:
        print('MEM     %.5f..%.5f s (n=%d)' % (mem[0], mem[-1], len(mem)))
    if ref:
        print('Refresh %.5f..%.5f s (n=%d)' % (ref[0], ref[-1], len(ref)))
        if mem:
            print('refresh after last MEM: %d'
                  % len([t for t in ref if t > mem[-1]]))
    print('#WAIT: ' + ' '.join('%.1f%s' % (t * 1e6, 'H' if v else 'L')
                               for t, v in wait[:14]))


# ---------------------------------------------------------------- main
def flash(tries=3):
    """Build and upload, retrying.

    Some boards program on the first attempt, others reliably fail it and
    take the second -- so retry rather than assuming either, and treat
    'Booting' as success even when pio reports failure (the teensy_size
    step can error out after the image is already in).
    """
    b = subprocess.run(['pio', 'run', '-e', ENV], cwd=PROJ,
                       capture_output=True, text=True)
    if 'SUCCESS' not in b.stdout:
        print(b.stdout[-2000:])
        sys.exit('build failed')
    out = ''
    for i in range(tries):
        p = subprocess.run(['pio', 'run', '-e', ENV, '-t', 'upload'],
                           cwd=PROJ, capture_output=True, text=True)
        out = p.stdout + p.stderr
        if 'SUCCESS' in out or 'Booting' in out:
            print('upload ok (attempt %d)' % (i + 1))
            return
        print('upload attempt %d failed' % (i + 1))
        time.sleep(1.0)
    print(out[-1500:])
    sys.exit('upload failed after %d attempts' % tries)


def main():
    a = sys.argv[1:] or ['reset']
    cmd = a[0]
    if cmd == 'flash':
        flash()
    elif cmd == 'probe':
        fd = _open()
        time.sleep(1.0)
        how = state(fd)
        who = identify(fd) if how == 'prompt' else None
        os.close(fd)
        print('%s%s' % (how, ' (%s)' % who if who else ''))
        sys.exit(0 if how == 'prompt' else 1)
    elif cmd == 'recover':
        fd = _open()
        time.sleep(1.0)
        who = identify(fd) or recover(fd)
        os.close(fd)
        print('at prompt (%s)' % who if who else 'still stuck -- reflash')
        sys.exit(0 if who else 1)
    elif cmd == 'send':
        txt, _ = converse(list(a[1]), wait=10.0, verbose=False)
        print(txt[-2000:])
    elif cmd in ('reset', 'full'):
        txt, who = converse(a[1:] or ['R'])
        report(txt, who, full=(cmd == 'full'))
    elif cmd == 'upload':
        if len(a) < 2:
            sys.exit('usage: %s upload FILE [cmd...]' % os.path.basename(sys.argv[0]))
        fd, who = open_board()
        sent, txt = upload_file(fd, a[1])
        for c in a[2:]:
            os.write(fd, c.encode())
            txt += _drain(fd, 30.0).decode('ascii', 'replace').replace('\r', '')
        os.close(fd)
        open(OUT, 'w').write(txt)
        loaded = [l for l in txt.split('\n') if 'uploaded' in l]
        print('%d records sent; %s' % (sent, loaded[-1].strip() if loaded
                                       else 'NO CONFIRMATION'))
        print(txt[-600:])
        print('(full text: %s)' % OUT)
    elif cmd == 'abort':
        print('aborted' if abort() else 'abort failed')
    elif cmd == 'run':
        # An optional cap, in seconds; 0 (the default) waits as long as the
        # run keeps making progress, however slow the target is.
        cap = float(a[1]) if len(a) > 1 else 0.0
        frames = int(a[2]) if len(a) > 2 else 0
        fd, who = open_board()
        t0 = time.time()
        os.write(fd, b'G')
        how, raw, marks = wait_run(fd, cap=cap, frames=frames)
        took = time.time() - t0
        if how != 'prompt':                    # still going, or stuck
            abort()
            raw += _drain(fd, 15.0)
            how = ('ended by abort (%s)' % how
                   if state(fd) == 'prompt' else state(fd))
        txt = raw.decode('ascii', 'replace').replace('\r', '')
        os.close(fd)
        open(OUT, 'w').write(txt)
        print('%.1fs, %d bytes, %d lines; %s'
              % (took, len(txt), txt.count('\n'), how))
        if marks:
            gaps = [marks[0]] + [marks[i] - marks[i - 1]
                                 for i in range(1, len(marks))]
            print('%d iterations, %.2fs each (%s)'
                  % (len(marks), sum(gaps) / len(gaps),
                     ' '.join('%.2f' % g for g in gaps)))
        report(txt, who)
        sys.exit(0 if 'prompt' in how or how.startswith('ended') else 1)
    elif cmd == 'logic':
        logic_report(logic())
    else:
        sys.exit(__doc__)


if __name__ == '__main__':
    main()
