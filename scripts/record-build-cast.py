#!/usr/bin/env python3
"""Record a build from the host shell as an asciinema cast.

    scripts/record-build-cast.py CAST CMD [CMD...]

Run from the directory the commands run in, usually samples/<arch>,
with the bench picked as for the other bench tools (BIONIC_BENCH or the
held lock). Each CMD, a shell command line, is shown as "$ CMD" and run
in turn, at 90x60; the first that fails ends the recording. CAST is
retimed as record-scenario.py --save retimes a cast, then searched for
absolute host paths, which a cast must not show: each hit is printed
and the exit status is 1.

    ../../scripts/record-build-cast.py msdos211_build_80c86.cast \\
        'make msdos211 msdos211-sd' \\
        '../../scripts/record-scenario.py msdos211_80c86.scenario --nopace'
"""
import os
import shlex
import subprocess
import sys
import tempfile


def run(status_file, cmds):
    """Runs |cmds|, leaving in |status_file| the exit status, which
    asciinema does not pass on."""
    status = 0
    for cmd in cmds:
        print(f'$ {cmd}', flush=True)
        status = subprocess.run(cmd, shell=True).returncode
        if status:
            break
    with open(status_file, 'w') as f:
        f.write(str(status))


def host_paths(cast):
    """Lines of |cast| naming the home directory or anything under /home/."""
    home = os.path.expanduser('~')
    with open(cast) as f:
        return [(n, line.rstrip()) for n, line in enumerate(f, 1)
                if '/home/' in line or home in line]


def main(argv):
    if len(argv) >= 3 and argv[0] == '--run':
        run(argv[1], argv[2:])
        return
    if len(argv) < 2 or argv[0].startswith('-'):
        sys.exit(__doc__.strip())
    cast, cmds = argv[0], argv[1:]
    # Relative, so that no absolute path reaches the cast.
    me = os.path.relpath(os.path.abspath(__file__))
    with tempfile.TemporaryDirectory(prefix='record-build-cast-') as tmp:
        status_file = os.path.join(tmp, 'status')
        inner = shlex.join(['python3', me, '--run', status_file] + cmds)
        subprocess.run(['asciinema', 'rec', '--cols', '90', '--rows', '60',
                        '-q', '--overwrite', '-c', inner, cast])
        try:
            with open(status_file) as f:
                status = int(f.read())
        except (OSError, ValueError):
            status = 1
    if os.path.exists(cast):
        # As record-scenario.py --save does: first output at 0.5 s, the
        # last screen held.
        subprocess.run([os.path.join(os.path.dirname(me), 'retime-cast.py'),
                        cast], check=True)
    hits = host_paths(cast) if os.path.exists(cast) else []
    for n, line in hits:
        print(f'{cast}:{n}: {line[:200]}', file=sys.stderr)
    if hits:
        sys.exit(f'{cast}: {len(hits)} lines show a host path')
    sys.exit(status)


if __name__ == '__main__':
    main(sys.argv[1:])
