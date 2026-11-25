#!/usr/bin/env python3
"""Build open-simh's PDP-11 simulator, which installs the DCJ11's Unixes.

    debugger/pdp11/tools/simh-fetch.py DIR

Clones open-simh at a pinned commit into DIR/open-simh/ and builds its
PDP-11 simulator, DIR/open-simh/BIN/pdp11, which v6-install.py and
bsd211-install.py run the installations on; cmake and a C compiler do
that. Writes DIR/.fetched when done.
"""
import os
import subprocess
import sys

SIMH_URL = 'https://github.com/open-simh/simh'
SIMH_COMMIT = '87eb7d5e96f9ce0ee6ac183e20160e5c486b0712'  # 2026-09-22
SIMH = 'open-simh'
PDP11 = os.path.join(SIMH, 'BIN', 'pdp11')


def run(args, cwd):
    print(' '.join(args))
    subprocess.run(args, cwd=cwd, check=True)


# The repository holds more than the commit; a shallow fetch takes just it.
def simh(top):
    pdp11 = os.path.join(top, PDP11)
    if os.path.exists(pdp11):
        return
    src = os.path.join(top, SIMH)
    if not os.path.exists(os.path.join(src, 'CMakeLists.txt')):
        os.makedirs(src, exist_ok=True)
        run(['git', 'init', '-q'], src)
        run(['git', 'remote', 'add', 'origin', SIMH_URL], src)
        run(['git', 'fetch', '-q', '--depth', '1', 'origin', SIMH_COMMIT], src)
        run(['git', '-c', 'advice.detachedHead=false', 'checkout', '-q',
             'FETCH_HEAD'], src)
    run(['cmake', '-S', '.', '-B', 'build', '-DWITH_NETWORK=OFF',
         '-DWITH_VIDEO=OFF', '-DCMAKE_BUILD_TYPE=Release'], src)
    run(['cmake', '--build', 'build', '--target', 'pdp11', '-j',
         str(os.cpu_count() or 1)], src)
    if not os.path.exists(pdp11):
        sys.exit(f'{pdp11}: not built')


def main(argv):
    if len(argv) != 1:
        sys.exit(__doc__)
    top = argv[0]
    os.makedirs(top, exist_ok=True)
    simh(top)
    with open(os.path.join(top, '.fetched'), 'w') as f:
        f.write(f'{PDP11}\n')


if __name__ == '__main__':
    main(sys.argv[1:])
