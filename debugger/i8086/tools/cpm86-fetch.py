#!/usr/bin/env python3
"""Fetch the third-party files the CP/M-86 disks are built from.

    debugger/i8086/tools/cpm86-fetch.py DIR

Downloads each archive below into DIR/dl/ (kept, so later runs work
offline), checks its sha256, and extracts the listed files: CPM.SYS
into DIR/, the A: files into DIR/a/, the B: files into DIR/b/. Writes
DIR/.fetched when done.

All are DRI's, from http://www.cpm.z80.de/download/ (see
http://www.cpm.z80.de/license.html).
"""
import hashlib
import io
import os
import sys
import urllib.error
import urllib.request
import zipfile

CPM = 'http://www.cpm.z80.de/download/'

# (URL, sha256, {member: subdirectory of DIR})
SOURCES = [
    (CPM + 'CPM-86.zip',
     'be13110cc8706e15a6061a07b866383113c67874474937802bf90f01d5bf3017',
     {'V1-1R/D2/CPM.SYS': '',
      **{'V1-1R/D2/' + n: 'a' for n in (
          'ASM86.CMD', 'DDT86.CMD', 'ED.CMD', 'GENCMD.CMD', 'HELP.CMD',
          'HELP.HLP', 'PIP.CMD', 'STAT.CMD', 'SUBMIT.CMD')}}),
]


def download(url, sha, cache):
    path = os.path.join(cache, url.rsplit('/', 1)[1])
    if os.path.exists(path):
        with open(path, 'rb') as f:
            data = f.read()
        if hashlib.sha256(data).hexdigest() == sha:
            return data
    print('fetch', url)
    req = urllib.request.Request(url, headers={'User-Agent': 'cpm86-fetch'})
    try:
        with urllib.request.urlopen(req) as r:
            data = r.read()
    except urllib.error.URLError as e:
        sys.exit(f'{url}: {e.reason}')
    if hashlib.sha256(data).hexdigest() != sha:
        sys.exit(f'{url}: sha256 mismatch')
    with open(path, 'wb') as f:
        f.write(data)
    return data


def main(argv):
    if len(argv) != 1:
        sys.exit(__doc__)
    top = argv[0]
    cache = os.path.join(top, 'dl')
    os.makedirs(cache, exist_ok=True)
    for url, sha, files in SOURCES:
        z = zipfile.ZipFile(io.BytesIO(download(url, sha, cache)))
        for name, sub in files.items():
            try:
                body = z.read(name)
            except KeyError:
                sys.exit(f'{url}: no {name}')
            out = os.path.join(top, sub)
            os.makedirs(out, exist_ok=True)
            with open(os.path.join(out, name.rsplit('/', 1)[-1]), 'wb') as f:
                f.write(body)
    open(os.path.join(top, '.fetched'), 'w').close()


if __name__ == '__main__':
    main(sys.argv[1:])
