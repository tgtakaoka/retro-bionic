#!/usr/bin/env python3
"""Fetch the third-party files the CP/M-68K disks are built from.

    debugger/mc68000/tools/cpm68k-fetch.py DIR

Downloads each archive below into DIR/dl/ (kept, so later runs work
offline), checks its sha256, and extracts the listed files: CPM15000.SR
into DIR/, the A: files and NOBIOSHI.S into DIR/a/. The
command files are renamed from .REL to .68K, as the CCP runs them.
Writes DIR/.fetched when done. The 1.2 and 1.3 archives are shrunk and
imploded, PKZIP 1 methods Python's zipfile cannot read, so those members
come out through Info-ZIP's unzip.

Only DRI's files are fetched, freely distributable under
http://www.cpm.z80.de/license.html: CP/M-68K 1.3, the Alcyon C of 1.2,
whose C068 converts floating constants rightly where 1.3's does not, and
CBASIC.
"""
import fnmatch
import hashlib
import io
import os
import subprocess
import sys
import urllib.error
import urllib.request
import zipfile

CPM = 'http://www.cpm.z80.de/download/'


def rel(member, sub='a'):
    """A command file, renamed .68K in |sub|."""
    name = member.rsplit('/', 1)[-1].rsplit('.', 1)[0]
    return {member: f'{sub}/{name}.68K'}


# (URL, sha256, {member pattern: DIR's subdirectory, ending in '/', or
# the file's path in DIR})
SOURCES = [
    (CPM + '68kv1_3.zip',
     '63afddd9bd39935cdf8e831344cc5861f1bdfb22d2729784624de6bc442e7f4a',
     {'DISK8/CPM15000.SR': '',
      **rel('DISK1/PIP.REL'), **rel('DISK1/STAT.REL'), **rel('DISK3/ED.REL'),
      **rel('DISK2/DUMP.REL'), **rel('DISK6/LINK68.REL'),
      **rel('DISK1/RELOC.REL'),
      'DISK8/CPMLIB': 'a/', 'DISK7/NOBIOSHI.S': 'a/'}),
    (CPM + '68kv1_2.zip',
     'a55e21d67a3b8f9a1191945c21c15f69cc021d2cee178540aa7475287b84aa13',
     {**rel('DISK3/CP68.REL'), **rel('DISK4/C068.REL'),
      **rel('DISK4/C168.REL'), **rel('DISK3/AS68.REL'),
      **rel('DISK5/LO68.REL'),
      'DISK3/AS68INIT': 'a/', 'DISK8/CLIB': 'a/', 'DISK8/S.O': 'a/',
      'DISK4/LIBF.A': 'a/', 'DISK5/LIBE.A': 'a/', 'DISK3/*.H': 'a/',
      'DISK1/OSIF.H': 'a/', 'DISK1/OSATTR.H': 'a/'}),
    (CPM + 'cb68.zip',
     'e829abee906627db48a3e005d35ddedc593813c0f669f6b469c6ba45c58a1cfa',
     {**rel('CB68.REL'), 'CB68.L68': 'a/'}),
]


def download(url, sha, cache):
    path = os.path.join(cache, url.rsplit('/', 1)[1])
    if os.path.exists(path):
        with open(path, 'rb') as f:
            data = f.read()
        if hashlib.sha256(data).hexdigest() == sha:
            return data
    print('fetch', url)
    req = urllib.request.Request(url, headers={'User-Agent': 'cpm68k-fetch'})
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


def read(z, path, name):
    if z.getinfo(name).compress_type in (zipfile.ZIP_STORED,
                                         zipfile.ZIP_DEFLATED):
        return z.read(name)
    try:
        return subprocess.run(['unzip', '-p', path, name], check=True,
                              capture_output=True).stdout
    except (OSError, subprocess.CalledProcessError) as e:
        sys.exit(f'{path}: {name}: unzip failed: {e}')


def target(name, patterns):
    # A pattern matches within one directory level only.
    for pat, dest in patterns.items():
        if name.count('/') == pat.count('/') and \
           fnmatch.fnmatchcase(name.upper(), pat.upper()):
            if dest == '' or dest.endswith('/'):
                return dest + name.rsplit('/', 1)[-1]
            return dest
    return None


def main(argv):
    if len(argv) != 1:
        sys.exit(__doc__)
    top = argv[0]
    cache = os.path.join(top, 'dl')
    os.makedirs(cache, exist_ok=True)
    for url, sha, patterns in SOURCES:
        data = download(url, sha, cache)
        archive = os.path.join(cache, url.rsplit('/', 1)[1])
        z = zipfile.ZipFile(io.BytesIO(data))
        found = set()
        for name in z.namelist():
            dest = target(name, patterns)
            if dest is None or name.endswith('/'):
                continue
            found.update(p for p in patterns
                         if fnmatch.fnmatchcase(name.upper(), p.upper()))
            path = os.path.join(top, dest)
            os.makedirs(os.path.dirname(path) or top, exist_ok=True)
            with open(path, 'wb') as f:
                f.write(read(z, archive, name))
        missing = set(patterns) - found
        if missing:
            sys.exit(f'{url}: not found: ' + ', '.join(sorted(missing)))
    open(os.path.join(top, '.fetched'), 'w').close()


if __name__ == '__main__':
    main(sys.argv[1:])
