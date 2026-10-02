#!/usr/bin/env python3
"""Print the files an assembler source includes, nested ones too.

    scripts/asm-includes.py SOURCE

Prints them on one line, relative to the source's directory, as a
Makefile's prerequisites (see samples/deps.mk). An include is looked up
next to the file naming it, then in the source's directory; a missing
one is skipped, and the assembler reports it.
"""
import os
import re
import sys

INCLUDE = re.compile(r'^\s*include\s+"([^"]+)"', re.IGNORECASE)


def includes(path, top, seen):
    """The files `path` includes, nested ones too, relative to `top`."""
    with open(path, errors='replace') as f:
        names = [m.group(1) for m in map(INCLUDE.match, f) if m]
    for name in names:
        for base in (os.path.dirname(path), top):
            found = os.path.normpath(os.path.join(base, name))
            if os.path.isfile(found):
                break
        else:
            continue
        rel = os.path.relpath(found, top)
        if rel not in seen:
            seen.append(rel)
            includes(found, top, seen)
    return seen


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__.strip().split('\n\n')[1])
    src = sys.argv[1]
    print(' '.join(includes(src, os.path.dirname(src) or '.', [])))


if __name__ == '__main__':
    main()
