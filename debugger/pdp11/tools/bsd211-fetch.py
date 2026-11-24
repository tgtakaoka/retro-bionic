#!/usr/bin/env python3
"""Fetch 2.11BSD's distribution tape and build the simulator that installs it.

    debugger/pdp11/tools/bsd211-fetch.py DIR

Downloads the 2.11BSD PL500 tape from TUHS into DIR/dl/ (kept, so later
runs work offline), checks its sha256 and unpacks it to DIR/PL500.tap.
Clones open-simh at a pinned commit into DIR/open-simh/ and builds its
PDP-11 simulator, DIR/open-simh/BIN/pdp11, which bsd211-install.py runs
the installation on; cmake and a C compiler do that. Writes DIR/.fetched
when done.

2.11BSD is distributed under the BSD license and Caldera's license for
ancient UNIX (https://www.tuhs.org/Archive/Caldera-license.pdf).
"""
import bz2
import hashlib
import os
import subprocess
import sys
import urllib.error
import urllib.request

URL = 'https://www.tuhs.org/Archive/Distributions/UCB/2BSD/2.11BSD/' \
      'Patches/PL500.tap.bz2'
SHA256 = '198e8ca3d1e76ee17ea28b63040488ce9bec3c9157233ffc6272eae7fd19eaf9'
TAPE = 'PL500.tap'
TAPE_SHA256 = 'abdb342bbf94e15b4fc8a44d0e0292493585fbab216aa29ee8a4aa284c0fd36f'
SIMH_URL = 'https://github.com/open-simh/simh'
SIMH_COMMIT = '87eb7d5e96f9ce0ee6ac183e20160e5c486b0712'  # 2026-09-22
SIMH = 'open-simh'
PDP11 = os.path.join(SIMH, 'BIN', 'pdp11')


def download(cache):
    path = os.path.join(cache, URL.rsplit('/', 1)[1])
    if os.path.exists(path):
        with open(path, 'rb') as f:
            data = f.read()
        if hashlib.sha256(data).hexdigest() == SHA256:
            return data
    print('fetch', URL)
    req = urllib.request.Request(URL, headers={'User-Agent': 'bsd211-fetch'})
    try:
        with urllib.request.urlopen(req) as r:
            data = r.read()
    except urllib.error.URLError as e:
        sys.exit(f'{URL}: {e.reason}')
    if hashlib.sha256(data).hexdigest() != SHA256:
        sys.exit(f'{URL}: sha256 mismatch')
    with open(path, 'wb') as f:
        f.write(data)
    return data


def tape(top, cache):
    path = os.path.join(top, TAPE)
    if os.path.exists(path):
        with open(path, 'rb') as f:
            if hashlib.sha256(f.read()).hexdigest() == TAPE_SHA256:
                return
    data = bz2.decompress(download(cache))
    if hashlib.sha256(data).hexdigest() != TAPE_SHA256:
        sys.exit(f'{TAPE}: sha256 mismatch')
    with open(path, 'wb') as f:
        f.write(data)


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
    cache = os.path.join(top, 'dl')
    os.makedirs(cache, exist_ok=True)
    tape(top, cache)
    simh(top)
    with open(os.path.join(top, '.fetched'), 'w') as f:
        f.write(f'{TAPE}\n{PDP11}\n')


if __name__ == '__main__':
    main(sys.argv[1:])
