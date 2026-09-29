#!/usr/bin/env python3
"""Time the Mandelbrot samples' rows in a recorded cast or a live run.

    scripts/mandelbrot-timing.py CAST...

CAST is an asciinema cast file (v2), or the numeric id of one on
asciinema.org. Prints one Markdown table row per program a cast runs,
mandelbrot (integer) or fmandel (floating point), told apart by their
golden frames: a full frame's seconds -- measured between two frame
boundaries when the cast has one, else estimated -- the average seconds
per row and their standard deviation, and the CPU the banner names.

Rows differ in work: each pixel prints its iteration count (0-9, A-F,
or a space for 16), and costs that many iterations plus a fixed part.
The estimate fits seconds = a * iterations + b over the timed rows,
then sums that over the golden frame's rows.

A row is timed from the end of the line before it: another row, or the
blank line between frames. The first row of a run is not timed, since
a run may resume in the middle of one. A cast recorded through a
terminal multiplexer moves the cursor instead of printing newlines, so
there escape sequences end lines, and frames are only estimated.
"""
import json
import os
import re
import statistics
import sys
import urllib.request

ROW = re.compile(r'[0-9A-Z ]{79}')
FRAME_ROWS = 25
ARITH = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'samples', 'arith')
PROGRAMS = ('mandelbrot', 'fmandel')


def golden(program):
    """The rows of `program`'s golden frame."""
    with open(os.path.join(ARITH, program + '.golden')) as f:
        return f.read().strip('\n').split('\n')


GOLDEN = {p: golden(p) for p in PROGRAMS}
# A run whose rows differ from its golden frame by more pixels than
# this, on average, is not counted: the chip or the bus went wrong.
MAX_MISS = 20


def miss(row, program):
    """The fewest pixels `row` differs by from a row of `program`."""
    return min(sum(a != b for a, b in zip(row, g)) for g in GOLDEN[program])


def program_of(rows):
    """The program a run of rows came from, the one its rows are nearest
    to (an older sample can differ a little), or None."""
    best = min(PROGRAMS, key=lambda p: sum(miss(r, p) for r in rows))
    return best if sum(miss(r, best) for r in rows) <= MAX_MISS * len(rows) else None
ESC = re.compile(r'\x1b(\[[0-9;?!]*[A-Za-z@`]|[()][0-9A-Za-z]|[=>]|\][^\x07]*\x07)')
BANNER = re.compile(r'\* Bionic(\S*)(?: \(CPU: (\S+)\))? \*')


def lines(events, screen=False):
    """(time, line) for each complete line of `events`, (time, text)
    output chunks; the time is that of the chunk ending the line. With
    `screen`, escape sequences end lines too, and empty lines are
    dropped."""
    part = ''
    for t, text in events:
        if screen:
            text = ESC.sub('\n', text)
        part += text.replace('\r', '')
        *done, part = part.split('\n')
        for line in done:
            if line or not screen:
                yield t, line


def iterations(row):
    """The iterations a row's pixels took."""
    return sum(16 if c == ' ' else int(c, 16) + 1 for c in row)


def runs(events, screen=False):
    """The runs of `events`: lists of (time, line) of rows and the blank
    lines between frames, split at any other line."""
    run = []
    for t, line in lines(events, screen):
        if ROW.fullmatch(line) or (line == '' and run):
            run.append((t, line))
        else:
            if run:
                yield run
            run = []
    if run:
        yield run


def timing(events, screen=False):
    """{program: (rows, frame seconds)} out of `events`; rows are
    (seconds, iterations)."""
    out = {}
    for run in runs(events, screen):
        program = program_of([line for _, line in run if line])
        if program is None:
            continue
        rows, frames = out.setdefault(program, ([], []))
        prev = None         # end of the line a row may follow
        frame_start = None  # end of the blank line starting a frame
        count = 0
        for t, line in run:
            if line:
                if prev is not None:
                    rows.append((t - prev, iterations(line)))
                prev, count = t, count + 1
            else:
                if frame_start is not None and count == FRAME_ROWS:
                    frames.append(t - frame_start)
                prev = frame_start = t
                count = 0
    return out


def estimate(rows, program):
    """`program`'s golden frame's seconds by the least-squares fit of
    `rows`."""
    secs = [s for s, _ in rows]
    its = [i for _, i in rows]
    mi, ms = statistics.mean(its), statistics.mean(secs)
    sxx = sum((i - mi) ** 2 for i in its)
    a = sum((i - mi) * (s - ms) for i, s in zip(its, secs)) / sxx if sxx else 0
    if a <= 0:
        a, b = ms / mi, 0.0     # too few distinct rows to fit
    else:
        b = ms - a * mi
    return sum(a * iterations(row) + b for row in GOLDEN[program])


def summary(rows, frames, program='mandelbrot'):
    """(frame, average, deviation) as table cells."""
    if len(rows) < 2:
        return '', '', ''
    secs = [s for s, _ in rows]
    frame = statistics.mean(frames) if frames else estimate(rows, program)
    return '%.1f' % frame, '%.3f' % statistics.mean(secs), '%.3f' % statistics.stdev(secs)


def read_cast(src):
    """(cpu, events) of a cast file or asciinema.org id."""
    if re.fullmatch(r'\d+', src):
        with urllib.request.urlopen('https://asciinema.org/a/%s.cast' % src) as f:
            text = f.read().decode()
    else:
        with open(src) as f:
            text = f.read()
    events = []
    for line in text.splitlines()[1:]:
        e = json.loads(line)
        if e[1] == 'o':
            events.append((e[0], e[2]))
    m = BANNER.search(''.join(e[1] for e in events[:200]))
    cpu = (m.group(2) or m.group(1)) if m else '?'
    return cpu, events


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__.strip().split('\n\n')[1])
    for src in sys.argv[1:]:
        cpu, events = read_cast(src)
        screen = any('\x1b[' in text for _, text in events)
        for program, (rows, frames) in timing(events, screen).items():
            frame, avg, dev = summary(rows, frames, program)
            print('| %s | %s | %s | %s | %s | %s |' % (
                    frame, avg, dev, cpu, os.path.basename(src), program))


if __name__ == '__main__':
    main()
