#!/usr/bin/env python3
"""Fetch 2.11BSD's distribution tape.

    debugger/pdp11/tools/bsd211-fetch.py DIR

Downloads the 2.11BSD PL500 tape from TUHS into DIR/dl/ (kept, so later
runs work offline), checks its sha256 and unpacks it to DIR/PL500.tap,
which bsd211-install.py installs from under simh-fetch.py's simulator.
Writes DIR/.fetched when done.

2.11BSD is distributed under the BSD license and Caldera's license for
ancient UNIX (https://www.tuhs.org/Archive/Caldera-license.pdf).
"""
import bz2
import hashlib
import os
import sys
import urllib.error
import urllib.request

URL = 'https://www.tuhs.org/Archive/Distributions/UCB/2BSD/2.11BSD/' \
      'Patches/PL500.tap.bz2'
SHA256 = '198e8ca3d1e76ee17ea28b63040488ce9bec3c9157233ffc6272eae7fd19eaf9'
TAPE = 'PL500.tap'
TAPE_SHA256 = 'abdb342bbf94e15b4fc8a44d0e0292493585fbab216aa29ee8a4aa284c0fd36f'


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


def main(argv):
    if len(argv) != 1:
        sys.exit(__doc__)
    top = argv[0]
    cache = os.path.join(top, 'dl')
    os.makedirs(cache, exist_ok=True)
    tape(top, cache)
    with open(os.path.join(top, '.fetched'), 'w') as f:
        f.write(f'{TAPE}\n')


if __name__ == '__main__':
    main(sys.argv[1:])
