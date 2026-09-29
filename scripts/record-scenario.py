#!/usr/bin/env python3
"""Drive a scripted console demo, live or as an asciinema recording.

Target agnostic, like bionic-control.py: everything specific to one demo
-- which commands, which addresses, which sample -- lives in a .scenario
file next to the sample it drives, e.g. samples/z280/mandelbrot.scenario,
not here.

Everything this script writes to stdout is exactly what a person driving
or watching sees: it opens the board quietly first (recovering it to its
prompt the way every other bench script does, so that traffic never
reaches the output), then works through the scenario -- typing each
console command itself, one character at a time, with pauses long enough
to read -- while a reader thread echoes the board's replies live,
including a sample's own output as it prints.

    scripts/record-scenario.py SCENARIO [--nopace] [--save PATH | --upload]

With neither --save nor --upload, it just talks to the board directly
and streams its output to stdout live -- run it manually to watch or dry
-run a scenario, or wrap it yourself (`asciinema rec -c "scripts/record-
scenario.py SCENARIO"`) for more control over asciinema's own flags.

--save PATH re-invokes itself under `asciinema rec`, saved locally to
PATH; not uploaded. --upload does the same but uploads it to
asciinema.org instead, printing the resulting URL. Either replays the
scenario in full at whatever pace applies -- neither is instant.

--nopace shortens every pause to a tenth, for a dry run of the command
sequence without sitting through the real timing.

Scenario file: one directive per line, blank lines and '#' comments
ignored.

    key LETTER       a single-letter command that needs no Enter (the
                      CLI dispatches it on receipt), e.g. 'R', 'S' -- not
                      'G'/'g', which read an optional line limit and so
                      need `command`; wait for the '> ' prompt, pace
    command TEXT     type TEXT, Enter, wait for the '> ' prompt, pace --
                      for a value a sub-prompt is waiting for (e.g. an
                      address after 'D' opens "Disassemble? "), or for
                      'G [n]'/'g addr [n]' -- Enter alone, with no
                      digits, means unlimited
    type TEXT        type TEXT with no Enter and no wait, to open a
                      sub-prompt (e.g. 'D') without answering it yet
    wait TEXT        block until TEXT appears in what the board sent
    pace SECONDS     a narration pause
    step N           'S' repeated N times, each waiting for the prompt
    run N [TIMEOUT]  'G' capped to RUN_BACKTRACE_LIMIT lines, then wait
                      for N more blank-line frames from a sample that
                      redraws in a loop, then stop it with the halt port
                      and wait for the prompt. TIMEOUT (default 120s)
                      bounds the wait -- raise it for a slow target
    upload PATH      type 'U', wait for "Upload waiting...", paste the
                      Intel HEX/S-record file at PATH (relative to the
                      repo root), end with ^C, wait for the prompt
    rows N           terminal size for --save/--upload's asciinema
    cols N            recording (default 60x90); no effect live, since
                      the recorded pty always takes it from these, but a
                      live run just inherits the real terminal instead
"""
import argparse
import os
import select
import shlex
import sys
import threading
import time
from importlib.machinery import SourceFileLoader

HERE = os.path.dirname(os.path.abspath(__file__))
bc = SourceFileLoader('bc', os.path.join(HERE, 'bionic-control.py')).load_module()
PROJ = bc.PROJ

SCALE = 1.0   # set from --nopace in main(); pace() reads this each call

# A drawing sample's frame can be thousands of instructions; 'run N' abstracts
# it into "watch N frames render", so the backtrace after the halt is for
# orientation, not review -- keeping it small keeps the last-drawn frame on
# screen instead of scrolling it away.
RUN_BACKTRACE_LIMIT = 5

DEFAULT_ROWS = 60
DEFAULT_COLS = 90


def pace(seconds):
    time.sleep(seconds * SCALE)


def read_size(path):
    """Scan a scenario for `rows`/`cols` directives, ahead of running it.

    record() needs these before it execs asciinema -- by the time
    run_scenario() would see them, the pty is already sized -- so this
    is a separate pre-pass, not a directive run_scenario dispatches.
    """
    rows, cols = DEFAULT_ROWS, DEFAULT_COLS
    for raw in open(path):
        line = raw.strip()
        word, _, rest = line.partition(' ')
        if word == 'rows':
            rows = int(rest.strip())
        elif word == 'cols':
            cols = int(rest.strip())
    return rows, cols


class Console:
    """Live pass-through of the board's console, plus scripted typing.

    The reader thread writes every byte the board sends straight to
    stdout as it arrives -- that raw stream is the recording. `type_line`
    types into the same fd with human-scale per-key pauses; the echo the
    board sends back is what makes the typing visible on screen.
    """

    def __init__(self, fd):
        self.fd = fd
        self._buf = bytearray()
        self._frames = 0   # FRAME markers seen since start, not just in _buf
        self._lock = threading.Lock()
        self._stop = False
        self._thread = threading.Thread(target=self._read_loop, daemon=True)
        self._thread.start()

    def _read_loop(self):
        # The port is O_NONBLOCK (bc._open()), so a bare os.read() raises
        # BlockingIOError -- an OSError -- the moment nothing is waiting,
        # which is most of the time; select() is what waits.
        while not self._stop:
            r, _, _ = select.select([self.fd], [], [], 0.2)
            if not r:
                continue
            try:
                d = os.read(self.fd, 4096)
            except BlockingIOError:
                # select() can report ready and the read still find
                # nothing; ending here would stop draining the board,
                # whose output then backs up until its program blocks.
                continue
            except OSError:
                break
            if not d:
                continue
            sys.stdout.buffer.write(d)
            sys.stdout.buffer.flush()
            with self._lock:
                # Count across the chunk boundary, in case a marker splits.
                seam = bytes(self._buf[-(len(bc.FRAME) - 1):])
                self._frames += (seam + d).count(bc.FRAME) - seam.count(bc.FRAME)
                self._buf += d
                del self._buf[:-4096]   # bounded tail, cheap to scan

    def tail(self, n=4096):
        with self._lock:
            return bytes(self._buf[-n:])

    def type_line(self, text, cr=True):
        """Type `text` one key at a time, then Enter unless `cr` is False."""
        pace(0.5)   # "about to type" beat
        for i, ch in enumerate(text):
            os.write(self.fd, ch.encode())
            pace(0.05 + 0.03 * (i % 3))   # human-ish jitter
        if cr:
            os.write(self.fd, b'\r')

    def wait_for(self, needle, timeout=5.0, strip_cr=False, anywhere=False):
        """Block until `needle` is in (or, by default, ends) what was sent.

        A scenario's `wait` targets a sub-prompt like "Disassemble? " --
        distinctive enough that a substring match is safe, and simpler than
        fighting trailing-whitespace loss through the scenario file's own
        line stripping. `wait_prompt` below needs the stricter ends-with:
        a sample's own output can itself contain "> ".
        """
        end = time.time() + timeout
        while time.time() < end:
            seen = self.tail()
            if strip_cr:
                seen = seen.replace(b'\r', b'')
            if (needle in seen) if anywhere else seen.endswith(needle):
                return True
            time.sleep(0.02)
        return False

    def wait_prompt(self, timeout=15.0):
        return self.wait_for(bc.PROMPT, timeout, strip_cr=True)

    def wait_frames(self, n, timeout=120.0):
        """Block until `n` more FRAME markers have streamed by.

        Polled at 20ms: a cached run can redraw a frame in well under a
        second, so a coarser poll would let a visible slice of the next
        one print before abort() lands.
        """
        target = self._frames + n
        end = time.time() + timeout
        while time.time() < end:
            if self._frames >= target:
                return True
            time.sleep(0.02)
        return False

    def close(self):
        self._stop = True


def do_command(con, text):
    con.type_line(text)
    con.wait_prompt()
    pace(0.7)   # let the audience read the reply


def do_key(con, letter):
    """A single-letter command: the CLI dispatches it without an Enter."""
    con.type_line(letter, cr=False)
    con.wait_prompt()
    pace(0.7)


def do_upload(con, path):
    con.type_line('U', cr=False)
    con.wait_for(b'Upload waiting...', anywhere=True)
    pace(0.3)
    for line in open(os.path.join(PROJ, path)):
        line = line.strip()
        if not line:
            continue
        bc.write_all(con.fd, line.encode() + b'\r')
        pace(0.001)
    os.write(con.fd, b'\x03')   # ^C ends the U loop
    con.wait_prompt()
    pace(0.75)


def do_run(con, n, timeout=120.0):
    con.type_line('G %d' % RUN_BACKTRACE_LIMIT)
    con.wait_frames(n, timeout)
    bc.abort()
    con.wait_prompt(10.0)
    pace(2.0)


def run_scenario(con, path):
    for lineno, raw in enumerate(open(path), 1):
        line = raw.strip()
        if not line or line.startswith('#'):
            continue
        word, _, rest = line.partition(' ')
        rest = rest.strip()
        if word == 'command':
            do_command(con, rest)
        elif word == 'key':
            do_key(con, rest)
        elif word == 'type':
            con.type_line(rest, cr=False)
        elif word == 'wait':
            if not con.wait_for(rest.encode(), anywhere=True):
                sys.exit('%s:%d: timed out waiting for %r' % (path, lineno, rest))
        elif word == 'pace':
            pace(float(rest))
        elif word == 'step':
            for _ in range(int(rest)):
                do_key(con, 'S')
        elif word == 'run':
            parts = rest.split()
            n = int(parts[0])
            timeout = float(parts[1]) if len(parts) > 1 else 120.0
            do_run(con, n, timeout)
        elif word == 'upload':
            do_upload(con, rest)
        elif word in ('rows', 'cols'):
            pass   # consumed by read_size() before the recording started
        else:
            sys.exit('%s:%d: unknown directive %r' % (path, lineno, word))


def _ensure_verbose_off(board):
    """Force V off before the recording starts, whatever it was left at.

    'V' toggles; only 'R' looks at it -- S/g/G always show the raw
    bus-cycle trace regardless -- but a stale ON would still make every
    'R' print its own cycle dump on top of the usual register dump. Done
    with board.send, not the Console, so it never reaches stdout.
    """
    if b'Verbose ON' in board.send(b'V', wait=2.0, idle=0.4):
        board.send(b'V', wait=2.0, idle=0.4)


def parse_args():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    p = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('scenario')
    p.add_argument('--nopace', action='store_true')
    g = p.add_mutually_exclusive_group()
    g.add_argument('--save', metavar='PATH')
    g.add_argument('--upload', action='store_true')
    return p.parse_args()


def record(scenario, nopace, save):
    """Re-invoke this same script under `asciinema rec`.

    That is what actually turns the live board session into a .cast file
    -- this process just becomes asciinema (os.execvp), which spawns the
    inner command in a pty and records everything it prints, exactly the
    live stdout stream `run_direct` produces on its own.
    """
    inner = [sys.executable, os.path.abspath(__file__), scenario]
    if nopace:
        inner.append('--nopace')
    rows, cols = read_size(scenario)
    cmd = ['asciinema', 'rec', '-c', shlex.join(inner),
            '-t', os.path.basename(scenario),
            '--rows', str(rows), '--cols', str(cols)]
    # --overwrite: re-recording an existing .cast is the normal case;
    # without it asciinema refuses and exits 1 before running anything.
    cmd += ['-q', '--overwrite', save] if save else ['-y']   # -y: auto-confirm the upload
    try:
        os.execvp('asciinema', cmd)
    except FileNotFoundError:
        sys.exit('asciinema not found on PATH -- install it first '
                  '(apt install asciinema, or pip install --user asciinema)')


def run_direct(scenario, nopace):
    global SCALE
    SCALE = 0.1 if nopace else 1.0
    board = bc.Board.open()   # silent: recovers the board, nothing printed
    _ensure_verbose_off(board)
    con = Console(board.fd)
    try:
        run_scenario(con, scenario)
    finally:
        con.close()
        board.close()
        bc.release()


def main():
    args = parse_args()
    if args.save or args.upload:
        record(args.scenario, args.nopace, args.save)
        return   # os.execvp does not return on success
    run_direct(args.scenario, args.nopace)


if __name__ == '__main__':
    main()
