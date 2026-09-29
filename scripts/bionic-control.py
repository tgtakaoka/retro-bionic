#!/usr/bin/env python3
"""Bench tool for the Bionic debugger board -- any target.

Purely a board controller: sends bytes, reads bytes, manages recovery and
timing. Nothing here decodes what comes back -- that's bionic-report.py's
job -- and nothing here knows about any other piece of bench equipment --
that's logic-analyzer.py's job.

Subcommands
  flash            build and upload, retrying (boards differ: some take
                   the first attempt, some reliably fail it)
  reset [cmd...]   send V (verbose on) then the commands, default R;
                   print the captured text
  send CMDS        send raw command characters, print the reply
  full             same as reset
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

Every board operation has a hard deadline and every one of them ends by
classifying the board (see `state`), so a run that never comes back is
reported as a failure instead of hanging the caller.

Full text always lands in $BIONIC_OUT for a follow-up look without
re-running the board.
"""
import fcntl
import os
import re
import select
import subprocess
import sys
import time

PORT = os.environ.get('BIONIC_PORT', '/dev/ttyACM0')
# The second USB serial is the halt switch: main.cpp's serialEventUSB1()
# fires Pins::isrHaltSwitch() on any byte, which loop() checks each step.
# It is the only way out of a run that does not come back on its own.
HALT_PORT = os.environ.get('BIONIC_HALT_PORT', '/dev/ttyACM1')
# The repository this script lives in, so a worktree needs no edit here.
PROJ = os.environ.get(
    'BIONIC_PROJ', os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ENV = os.environ.get('BIONIC_ENV', 'teensy41')
# Its own subdirectory, not loose files directly under /tmp: isolated from
# unrelated cleanup there, and `rm -rf` on this one directory is the whole
# cleanup.
RUN_DIR = os.environ.get('BIONIC_RUN_DIR', '/tmp/bionic-bench')
os.makedirs(RUN_DIR, exist_ok=True)
OUT = os.environ.get('BIONIC_OUT', os.path.join(RUN_DIR, 'last.txt'))
LOCK = os.environ.get('BIONIC_LOCK', os.path.join(RUN_DIR, 'board.lock'))
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


def pc_from(buf):
    """The PC/IP value out of a register dump, decoded if `buf` is bytes.

    The *last* PC=/IP= line wins when there is more than one -- the most
    recent register dump in the text, not the first. None if there is no
    such line at all.
    """
    text = buf.decode('ascii', 'replace').replace('\r', '') if isinstance(buf, bytes) else buf
    line = None
    for ln in text.split('\n'):
        if ln.startswith(('PC=', 'IP=')):
            line = ln
    return line.split()[0].split('=')[1] if line else None


def at_prompt(buf):
    """True if `buf` ends at the CLI prompt rather than merely containing it."""
    return buf.replace(b'\r', b'').endswith(PROMPT)


# A sample that redraws in a loop separates its iterations with a blank line.
# Counting those is how a run that never ends on its own is both timed and
# brought to a stop after a known amount of work.
FRAME = b'\n\r\n'


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


class Board:
    """One open board session: the serial fd, and the target name once
    identify()/recover() has found it (None until then)."""

    def __init__(self, fd, who=None):
        self.fd = fd
        self.who = who

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()
        return False

    def close(self):
        os.close(self.fd)

    def _drain(self, secs, idle=2.0):
        """Read until `idle` seconds of quiet, or `secs` in total -- never longer.

        The total is a hard deadline on purpose: a failed run emits nothing at
        all while the CPU cycles refresh forever, so an idle-only wait would
        block for the whole budget on every later command.
        """
        buf, last, end = b'', time.time(), time.time() + secs
        while time.time() < end:
            r, _, _ = select.select([self.fd], [], [], 0.1)
            if r:
                try:
                    d = os.read(self.fd, 65536)
                    if d:
                        buf += d
                        last = time.time()
                except BlockingIOError:
                    pass
            elif buf and time.time() - last > idle:
                break
        return buf

    def send(self, data=b'', wait=8.0, idle=0.8, delay=0.0):
        """Write `data` (bytes or str) if any, pause `delay` seconds, then
        drain the reply.

        `delay` covers two shapes: with no `data`, "give an abort a moment,
        then read whatever settles out"; with `data`, pacing a single
        keystroke before checking for a reply.
        """
        if data:
            os.write(self.fd, data if isinstance(data, bytes) else data.encode())
        if delay:
            time.sleep(delay)
        return self._drain(wait, idle)

    def wait_run(self, cap=0.0, stall=None, frames=0, progress=30.0, out=sys.stderr):
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
            r, _, _ = select.select([self.fd], [], [], 0.2)
            now = time.time()
            if r:
                try:
                    d = os.read(self.fd, 65536)
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

    def go(self, cmd=b'G', **kw):
        """`cmd` (default G), then wait_run(**kw) -- the pairing every
        run-and-wait case makes; `cmd` overrides for a one-shot command like
        `g<addr>\r` that also starts a run."""
        os.write(self.fd, cmd)
        return self.wait_run(**kw)

    def abort(self):
        """Stop a running CPU via the halt port. Harmless when not running."""
        return abort()

    # ------------------------------------------------------------ board state
    def identify(self, budget=6.0):
        """Return the target name if the CLI is at its prompt, else None.

        `?` is the right probe: it prints the banner straight from the CLI and
        never touches the CPU, so it answers even when a run has gone wrong.
        Both halves matter. The console must fall silent first, because a
        running sample keeps emitting and its output can contain anything; and
        the reply must *end* at the prompt, because output still draining from
        an earlier run satisfies a substring match on its own while only a CLI
        actually waiting for input prints `> ` last.
        """
        self._drain(2.0, 0.4)                       # discard what is still coming
        if self._drain(1.2, 1.2) != b'':            # still emitting: not a prompt
            return None
        os.write(self.fd, b'?')
        reply = self._drain(budget, 0.8)
        if not at_prompt(reply):
            return None
        m = BANNER.search(reply.decode('ascii', 'replace'))
        return m.group(1) or 'unknown' if m else None

    def state(self):
        """Classify the board: 'prompt', 'running' or 'wedged'.

        The three need different handling and look alike from a distance, so
        name them rather than reporting a bare failure:

          prompt   the CLI answered `?` -- ready for the next command
          running  the console is emitting, so the firmware is alive and it is
                   the CPU that has not come back; the halt port will stop it
          wedged   silent and no prompt: the firmware itself is stuck in an
                   unbounded wait, and only a reflash clears that
        """
        if self.identify() is not None:
            return 'prompt'
        return 'running' if self._drain(1.5, 1.5) != b'' else 'wedged'

    def recover(self, tries=3):
        """Work the ladder back to the prompt. Returns the target name or None.

        Ctrl-C cancels a half-finished CLI prompt, NUL is what the samples
        exit on, and the halt port stops a run that ignores both. Reflashing
        is deliberately *not* in here: it is the last resort, and the caller
        should decide to spend it.
        """
        for _ in range(tries):
            os.write(self.fd, b'\x03')                  # cancel a CLI prompt
            self._drain(2.0, 0.5)
            os.write(self.fd, b'\x00')                  # the samples exit on NUL
            self._drain(2.0, 0.6)
            who = self.identify()
            if who is not None:
                return who
            abort()
            time.sleep(0.8)
            self._drain(4.0, 0.8)
            who = self.identify()
            if who is not None:
                return who
        return None

    @classmethod
    def open(cls, budget=8.0):
        """Open the console and return a Board, recovering if need be."""
        fd = _open()
        time.sleep(1.0)
        board = cls(fd)
        who = board.identify(budget)
        if who is None:
            who = board.recover()
        if who is None:
            board.close()
            sys.exit('board will not return to its prompt -- reflash needed')
        board.who = who
        return board

    def list_breaks(self):
        """The breakpoint list, with the 'clear?' prompt cancelled."""
        txt = self.send(b'b', wait=6.0, idle=0.8).decode('ascii', 'replace').replace('\r', '')
        if 'clear?' in txt:
            txt += self.send(b'\x03', wait=4.0, idle=0.6).decode(  # cancel: an empty line clears index 0
                    'ascii', 'replace')
        return txt

    def clear_breaks(self, tries=6):
        for _ in range(tries):
            txt = self.send(b'b', wait=6.0, idle=0.8).decode('ascii', 'replace')
            if 'clear?' not in txt:
                return True
            self.send(b'0\r', wait=4.0, idle=0.6)
        return False

    def set_break(self, addr):
        return self.send(b'B' + addr.encode() + b'\r', wait=6.0, idle=0.8).decode(
                'ascii', 'replace').replace('\r', '')

    # ---------------------------------------------------------------- session
    def upload_file(self, path):
        """Inject one HEX/S-record file through the debugger's U command.

        U loops reading records until the line editor is cancelled, and the
        cancel key is Ctrl-C -- not ESC, which it simply ignores, so a session
        that sends the wrong one is left stuck in the loop and every later
        command is eaten as a malformed record.
        """
        os.write(self.fd, b'\x03')                      # leave any half-finished prompt
        self._drain(1.5, 0.3)
        os.write(self.fd, b'U')
        self._drain(3.0, 0.3)
        sent = 0
        for line in open(path):
            line = line.strip()
            if not line:
                continue
            os.write(self.fd, line.encode() + b'\r')
            self._drain(3.0, 0.15)
            sent += 1
        os.write(self.fd, b'\x03')
        return sent, self._drain(5.0, 0.5).decode('ascii', 'replace').replace('\r', '')

    @classmethod
    def converse(cls, cmds, wait=40.0, verbose=True):
        board = cls.open()
        if verbose:                                # V toggles; make sure it is ON
            os.write(board.fd, b'V')
            time.sleep(0.4)
            if b'Verbose OFF' in board._drain(1.5, 0.4):
                os.write(board.fd, b'V')
                time.sleep(0.4)
                board._drain(1.5, 0.4)
        out = b''
        for c in cmds:
            chunk = board.send(c, wait=wait, idle=2.0)
            if not at_prompt(chunk):
                # No prompt back. Say which of the two it is rather than
                # leaving the caller to guess from an empty reply.
                how = board.state()
                print('after %r: %s' % (c, how), file=sys.stderr)
                if how != 'wedged' and board.recover() is not None:
                    chunk += board.send(wait=5.0, idle=0.8)
                else:
                    out += chunk
                    break
            out += chunk
        who = board.who
        board.close()
        txt = out.decode('ascii', 'replace').replace('\r', '')
        open(OUT, 'w').write(txt)
        return txt, who


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
        board = Board(_open())
        time.sleep(1.0)
        how = board.state()
        who = board.identify() if how == 'prompt' else None
        board.close()
        print('%s%s' % (how, ' (%s)' % who if who else ''))
        sys.exit(0 if how == 'prompt' else 1)
    elif cmd == 'recover':
        board = Board(_open())
        time.sleep(1.0)
        who = board.identify() or board.recover()
        board.close()
        print('at prompt (%s)' % who if who else 'still stuck -- reflash')
        sys.exit(0 if who else 1)
    elif cmd == 'send':
        txt, _ = Board.converse(list(a[1]), wait=10.0, verbose=False)
        print(txt[-2000:])
    elif cmd in ('reset', 'full'):
        txt, who = Board.converse(a[1:] or ['R'])
        print(txt)
    elif cmd == 'upload':
        if len(a) < 2:
            sys.exit('usage: %s upload FILE [cmd...]' % os.path.basename(sys.argv[0]))
        board = Board.open()
        board.send(b'R')          # a clean start -- upload alone leaves
                                   # whatever the CPU was already doing
        sent, txt = board.upload_file(a[1])
        for c in a[2:]:
            txt += board.send(c, wait=30.0, idle=2.0).decode('ascii', 'replace').replace('\r', '')
        board.close()
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
        board = Board.open()
        t0 = time.time()
        how, raw, marks = board.go(cap=cap, frames=frames)
        took = time.time() - t0
        if how != 'prompt':                    # still going, or stuck
            board.abort()
            raw += board.send(wait=15.0, idle=2.0)
            how = ('ended by abort (%s)' % how
                   if board.state() == 'prompt' else board.state())
        txt = raw.decode('ascii', 'replace').replace('\r', '')
        board.close()
        open(OUT, 'w').write(txt)
        print('%.1fs, %d bytes, %d lines; %s'
              % (took, len(txt), txt.count('\n'), how))
        if marks:
            gaps = [marks[0]] + [marks[i] - marks[i - 1]
                                 for i in range(1, len(marks))]
            print('%d iterations, %.2fs each (%s)'
                  % (len(marks), sum(gaps) / len(gaps),
                     ' '.join('%.2f' % g for g in gaps)))
        print(txt)
        sys.exit(0 if 'prompt' in how or how.startswith('ended') else 1)
    else:
        sys.exit(__doc__)


if __name__ == '__main__':
    main()
