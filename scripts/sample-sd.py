#!/usr/bin/env python3
"""Put a sample directory's operating system files on its bench's SD card.

    scripts/sample-sd.py --cpu CPU[,CPU...] [--dir DIR] FILE... [--keep FILE...]

The bench is the one the bench tools pick: BIONIC_BENCH, the one the
session holds (scripts/bionic-bench.py lock NAME) or the only one on
USB, and `bionic-control.py probe` must find one of the CPUs at its
prompt there, as the banner names them: which chip sits on which bench
is never assumed. With every mounted disk unmounted, each FILE goes
into DIR/ on the card, the current directory's name unless --dir says
otherwise (samples/i8080 puts them in i8080/), only the chunks that
differ. A FILE after --keep, a user disk, goes only onto a card without
one, so files saved there stay.

The drives to unmount come from O's listing, which Board.mounted() in
bionic-control.py parses; a change to that listing must keep it working.
"""
import os
import subprocess
import sys
import time
from importlib.machinery import SourceFileLoader

SCRIPTS = os.path.dirname(os.path.abspath(__file__))
CONTROL = os.path.join(SCRIPTS, 'bionic-control.py')
bc = SourceFileLoader('bc', CONTROL).load_module()


def usage():
    sys.exit(__doc__.strip())


def main(argv):
    cpus, folder, files, keep = [], os.path.basename(os.getcwd()), [], []
    into = files
    while argv:
        a = argv.pop(0)
        if a == '--cpu' and argv:
            cpus += argv.pop(0).split(',')
        elif a == '--dir' and argv:
            folder = argv.pop(0)
        elif a == '--keep':
            into = keep
        elif a.startswith('-'):
            usage()
        else:
            into.append(a)
    if not cpus or not (files or keep):
        usage()
    p = subprocess.run([CONTROL, 'probe'], capture_output=True, text=True)
    said = (p.stdout + p.stderr).strip()
    who = said[said.find('(') + 1:said.rfind(')')] if said.startswith('prompt (') else None
    if p.returncode or who is None or who.upper() not in (c.upper() for c in cpus):
        sys.exit('no %s at a bench prompt: %s' % (' or '.join(cpus), said))
    with bc.Board.open() as board:
        for drive in board.mounted():
            board.unmount(drive)
            print('%s unmounted' % drive)
        for local in files + keep:
            remote = '%s/%s' % (folder, os.path.basename(local))
            if local in keep and board.sd_crcs(remote) is not None:
                print('%s: kept, the card has one' % remote)
                continue
            size, t0 = os.path.getsize(local), time.time()
            sent = board.sd_put(local, remote, sync=True)
            print('%s: %d of %d bytes sent in %.1fs' % (remote, sent, size,
                                                        time.time() - t0))


if __name__ == '__main__':
    try:
        main(sys.argv[1:])
    except RuntimeError as e:
        sys.exit(str(e))
