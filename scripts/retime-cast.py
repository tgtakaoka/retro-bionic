#!/usr/bin/env python3
"""Trim the blank start of asciinema casts and hold their last screen.

    scripts/retime-cast.py CAST...

Rewrites each cast (v2) in place: the first output appears LEAD seconds
in, instead of after the seconds spent opening the board, and an empty
output event TAIL seconds after the last one keeps the final screen up
before the player stops. Running it again changes nothing.
"""
import json
import sys

LEAD = 0.5
TAIL = 3.0


def retime(path):
    with open(path) as f:
        header, *lines = f.read().splitlines()
    events = [json.loads(line) for line in lines if line]
    # Drop a hold left by an earlier run, so a rerun lands on the same times.
    if events and events[-1][1:] == ['o', '']:
        events.pop()
    if not events:
        return
    shift = events[0][0] - LEAD
    for e in events:
        e[0] = round(e[0] - shift, 6)
    events.append([round(events[-1][0] + TAIL, 6), 'o', ''])
    with open(path, 'w') as f:
        f.write(header + '\n')
        for e in events:
            f.write(json.dumps(e, ensure_ascii=False) + '\n')


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__.strip())
    for path in sys.argv[1:]:
        retime(path)


if __name__ == '__main__':
    main()
