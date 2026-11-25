#!/usr/bin/env python3
"""Fetch the third-party files the CP/M-80 disks are built from.

    debugger/i8080/tools/cpm80-fetch.py DIR

Downloads each archive below into DIR/dl/ (kept, so later runs work
offline), checks its sha256, and extracts the listed files: CPM22.ASM
into DIR/, the A: files into DIR/a/. Writes DIR/.fetched when done.

It fetches DRI's CP/M 2.2 (see http://www.cpm.z80.de/license.html), BDS
C (public domain), HI-TECH C 3.09 (free), and Microsoft's MBASIC 5.21
from the textfiles.com mirror of an old CP/M archive.
"""
import fnmatch
import hashlib
import io
import os
import sys
import urllib.error
import urllib.request
import zipfile

import lbr

CPM = 'http://www.cpm.z80.de/download/'
HTC = 'https://raw.githubusercontent.com/agn453/HI-TECH-Z80-C/' \
      'd5ebbb2e1e4353b8f7644aef38698ae71eb87c12/'
BARNYARD = 'https://discmaster.textfiles.com/file/19793/' \
           '2015.02.ftp.barnyard.co.uk.tar/ftp.barnyard.co.uk/cpm/'

# (URL, sha256, {member pattern: subdirectory of DIR})
SOURCES = [
    (CPM + 'cpm2-asm.zip',
     'fb09f8bf32ae359d94c417fbf075e94431d734b9de0c857fc7a7901953469525',
     {'CPM22.ASM': ''}),
    # cpm22-b.zip's DUMP.ASM is damaged; this one is whole.
    (CPM + 'cpm2-plm.zip',
     '0c9baa32ce95860caeca2c87e629d19d690190f0551339ed71ccd3685dbed84c',
     {'DUMP.ASM': 'a'}),
    (CPM + 'cpm22-b.zip',
     '88735cca77e786505f53a63746ddbc5ca221eda79fd822fde5dc9f00a5bfba6f',
     {n + '.COM': 'a' for n in ('ASM', 'DDT', 'DSKMAINT', 'DUMP', 'ED',
                                'LOAD', 'PIP', 'STAT', 'SUBMIT', 'XSUB')}),
    ('https://www.bdsoft.com/dist/bdsc-all.zip',
     'bf5ab207081acb50729471fd29bf40ee63b3800e7617144e358e7533da558c0c',
     {'bdsc160/*': 'a', 'extra/BCD.LBR': 'a'}),
    (HTC + 'htc-bin.lbr',
     '4c1f140725700fe337408c64a13f96b78f3c4ded34cce2d2bcdb5815313c067a',
     {'*': 'a'}),
    # A plain file, not an archive.
    (BARNYARD + 'commercial-software/programming/Mbasic.com',
     '29d957fc6899c24f6296a1662a27eca545d85ee3f7d70d2794c9d045d92ff157',
     {'MBASIC.COM': 'a'}),
]


def download(url, sha, cache):
    path = os.path.join(cache, url.rsplit('/', 1)[1])
    if os.path.exists(path):
        with open(path, 'rb') as f:
            data = f.read()
        if hashlib.sha256(data).hexdigest() == sha:
            return data
    print('fetch', url)
    req = urllib.request.Request(url, headers={'User-Agent': 'cpm80-fetch'})
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


def members(url, data):
    if url.endswith('.lbr'):
        yield from lbr.members(data)
        return
    if not url.endswith('.zip'):
        yield url.rsplit('/', 1)[1].upper(), data
        return
    z = zipfile.ZipFile(io.BytesIO(data))
    for name in z.namelist():
        if not name.endswith('/'):
            yield name, z.read(name)


def match(name, patterns):
    # A pattern matches within one directory level only.
    for pat, sub in patterns.items():
        if name.count('/') == pat.count('/') and \
           fnmatch.fnmatchcase(name.upper(), pat.upper()):
            return sub
    return None


def main(argv):
    if len(argv) != 1:
        sys.exit(__doc__)
    top = argv[0]
    cache = os.path.join(top, 'dl')
    os.makedirs(cache, exist_ok=True)
    for url, sha, patterns in SOURCES:
        data = download(url, sha, cache)
        found = set()
        for name, body in members(url, data):
            sub = match(name, patterns)
            if sub is None:
                continue
            found.add(sub)
            out = os.path.join(top, sub)
            os.makedirs(out, exist_ok=True)
            with open(os.path.join(out, name.rsplit('/', 1)[-1]), 'wb') as f:
                f.write(body)
        if not found:
            sys.exit(f'{url}: nothing extracted')
    open(os.path.join(top, '.fetched'), 'w').close()


if __name__ == '__main__':
    main(sys.argv[1:])
