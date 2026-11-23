#!/usr/bin/env python3
"""Fetch the Unix V6 distribution tape and cut its three RK05 disks.

    debugger/pdp11/tools/v6-fetch.py DIR

Downloads Ken Wellsch's V6 tape from TUHS into DIR/dl/ (kept, so later
runs work offline), checks its sha256, and writes the disks it carries,
4000 blocks of 512 bytes each, as DIR/rk0.dsk (the root, tape block 100
on), DIR/rk1.dsk (/usr/source, from 4100) and DIR/rk2.dsk (/usr/doc,
from 8100): the blocks V6's setup copies with tmrk and dd. Writes
DIR/.fetched when done.

The tape is distributed under Caldera's license for ancient UNIX
(https://www.tuhs.org/Archive/Caldera-license.pdf).
"""
import gzip
import hashlib
import os
import sys
import urllib.error
import urllib.request

URL = 'https://www.tuhs.org/Archive/Distributions/Research/' \
      'Ken_Wellsch_v6/v6.tape.gz'
SHA256 = 'f29a328e03c91ceac378061d8571b81f1a6ca8e72866a327012f8542021b3ba4'
BLOCK = 512
DISK_BLOCKS = 4000
# (file, first tape block)
DISKS = [('rk0.dsk', 100), ('rk1.dsk', 4100), ('rk2.dsk', 8100)]


def download(cache):
    path = os.path.join(cache, URL.rsplit('/', 1)[1])
    if os.path.exists(path):
        with open(path, 'rb') as f:
            data = f.read()
        if hashlib.sha256(data).hexdigest() == SHA256:
            return data
    print('fetch', URL)
    req = urllib.request.Request(URL, headers={'User-Agent': 'v6-fetch'})
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


def main(argv):
    if len(argv) != 1:
        sys.exit(__doc__)
    top = argv[0]
    cache = os.path.join(top, 'dl')
    os.makedirs(cache, exist_ok=True)
    tape = gzip.decompress(download(cache))
    for name, start in DISKS:
        disk = tape[start * BLOCK:(start + DISK_BLOCKS) * BLOCK]
        if len(disk) != DISK_BLOCKS * BLOCK:
            sys.exit(f'{URL}: tape too short for {name}')
        with open(os.path.join(top, name), 'wb') as f:
            f.write(disk)
    open(os.path.join(top, '.fetched'), 'w').close()


if __name__ == '__main__':
    main(sys.argv[1:])
