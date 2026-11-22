#!/usr/bin/env python3
"""Fetch the third-party files the CP/M-86 disks are built from.

    debugger/i8086/tools/cpm86-fetch.py DIR

Downloads each archive below into DIR/dl/ (kept, so later runs work
offline), checks its sha256, and extracts the listed files: CPM.SYS
into DIR/, the A: files into DIR/a/, the B: files into DIR/b/. Writes
DIR/.fetched when done. pbasic86.zip is imploded, which Python's
zipfile cannot read, so it takes Info-ZIP's unzip.

All are DRI's, from http://www.cpm.z80.de/download/ (see
http://www.cpm.z80.de/license.html).
"""
import hashlib
import os
import subprocess
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
      # ASM86 reads an INCLUDE from its source's drive.
      'V1-1R/D2/8087.LIB': 'b',
      **{'V1-1R/D2/' + n: 'a' for n in (
          'ASM86.CMD', 'DDT86.CMD', 'ED.CMD', 'GENCMD.CMD', 'HELP.CMD',
          'HELP.HLP', 'PIP.CMD', 'STAT.CMD', 'SUBMIT.CMD')}}),
    (CPM + 'cbasic86.zip',
     '8c3d2d75311aea4ec02b080aad28e94192c016653acfd81a399ee29e8013e23d',
     {n: 'a' for n in ('CB86.CMD', 'CB86.OR1', 'CB86.OR2', 'CB86.OR3',
                       'CB86.L86')}),
    (CPM + 'pbasic86.zip',
     'e14abc10536dd2eb08f5f694dc6d926588f59056a12aa178fc13694395d822a7',
     {'BASIC.CMD': 'b'}),
    # DRC 1.11; DISK3's LINK86 1.4 links its objects, CBASIC's 1.02 does not.
    (CPM + 'drc_86.zip',
     '136916513514e7aca68698688d5a0c2f7459e17d237d068270605db07e307f33',
     {**{'V1-11/DISK1/' + n: 'a' for n in (
         'DRC.CMD', 'DRC860.CMD', 'DRC861.CMD', 'DRC862.CMD', 'DRCRPP.CMD',
         'R.CMD', 'DRC.ERR', 'CTYPE.H', 'ERRNO.H', 'PORTAB.H', 'SETJMP.H',
         'STDIO.H')},
      'V1-11/DISK2/CLEARS.L86': 'a',
      'V1-11/DISK3/LINK86.CMD': 'a',
      'V1-11/DISK3/RASM86.CMD': 'a'}),
]


def download(url, sha, cache):
    path = os.path.join(cache, url.rsplit('/', 1)[1])
    if os.path.exists(path):
        with open(path, 'rb') as f:
            data = f.read()
        if hashlib.sha256(data).hexdigest() == sha:
            return path
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
    return path


def extract(path, name):
    with zipfile.ZipFile(path) as z:
        try:
            return z.read(name)
        except KeyError:
            sys.exit(f'{path}: no {name}')
        except NotImplementedError:
            return subprocess.run(['unzip', '-p', path, name], check=True,
                                  stdout=subprocess.PIPE).stdout


def main(argv):
    if len(argv) != 1:
        sys.exit(__doc__)
    top = argv[0]
    cache = os.path.join(top, 'dl')
    os.makedirs(cache, exist_ok=True)
    for url, sha, files in SOURCES:
        path = download(url, sha, cache)
        for name, sub in files.items():
            body = extract(path, name)
            out = os.path.join(top, sub)
            os.makedirs(out, exist_ok=True)
            with open(os.path.join(out, name.rsplit('/', 1)[-1]), 'wb') as f:
                f.write(body)
    open(os.path.join(top, '.fetched'), 'w').close()


if __name__ == '__main__':
    main(sys.argv[1:])
