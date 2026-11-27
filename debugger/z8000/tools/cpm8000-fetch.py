#!/usr/bin/env python3
"""Fetch the DRI files the CP/M-8000 disks are built from.

    debugger/z8000/tools/cpm8000-fetch.py [--source] DIR

Downloads CP/M-8000 1.1 into DIR/dl/ (kept, so later runs work
offline), checks its sha256, and extracts from the four 8" disk images
inside it: CPMSYS.REL, the CCP and BDOS, and LIBCPM.A into DIR/, the A:
files, LIBCPM.A too, into DIR/a/. Writes DIR/.fetched when done.

The images, not the DISK1-4.ZIP beside them: those archives' text files
lost their line feeds and kept stale bytes past their ends. STDIO.H
includes a PORTAB.H that 1.1's disks lack, so A: gets the one of the
1.3 sources, cpm8k13.zip.

With --source, it downloads P-CP/M-Z8K, DRI's 1983 sources, instead
and extracts the CCP, BDOS and Z8002 glue that
pcpm8000-build.py builds a 1.2 kernel from into DIR/src/, writing
DIR/.fetched-source.

CP/M-8000 1.1 is DRI's release of December 1984 for the Olivetti M20, the
only one made, from the Unofficial CP/M Web Site under the licence at
http://www.cpm.z80.de/license.html.
"""
import hashlib
import io
import os
import sys
import urllib.error
import importlib.machinery
import urllib.request
import zipfile

_disk = importlib.machinery.SourceFileLoader(
    'cpm_disk', os.path.join(os.path.dirname(os.path.abspath(__file__)),
                             '../../../scripts/cpm-disk.py')).load_module()

URL = 'http://www.cpm.z80.de/download/cpm8k11.zip'
SHA = '3df52fdc8393283b08c900f00bcb86cd8f03bd2e9c40be4f58145bc857d9b183'
URL_SRC = 'http://www.cpm.z80.de/download/8k0583.zip'
SHA_SRC = '0e9ad74b70b1fdf0b449830bb0ac234c4a695defc95283d19970d58967cfa2c4'
# The 1.2 BDOS and CCP and the Z8002 glue of Kontron's port (newos/,
# else a loader); newos/stdio.h is the whole of the file bdos/ and ccp/
# have cut short.
SOURCES = (
    ['bdos/' + n for n in (
        'bdosmain.c', 'bdosmisc.c', 'bdosrw.c', 'conbdos.c', 'dskutil.c',
        'fileio.c', 'iosys.c', 'pgmld.c', 'basepage.h', 'bdosdef.h',
        'biosdef.h', 'pktio.h', 'stdio.h')] +
    ['ccp/' + n for n in ('ccp.c', 'go.c', 'ccpdef.h', 'ccpif.z8k')] +
    ['newos/' + n for n in ('bdosif2.z8k', 'exceptn2.z8k', 'biosdefs2.z8k',
                            'xout.h', 'stdio.h')])
URL13 = 'http://www.cpm.z80.de/download/cpm8k13.zip'
SHA13 = '0cb55fe9699c4c2afb7734ba550ed75438ee0436fcef5a66156bc9050ebc92da'

# {disk image: {file: subdirectories of DIR}}. DDT, the compiler passes,
# the assembler and the linker need more than a 64K TPA or split
# instruction and data spaces, so they are on A: for a system that has
# them. Olivetti's disk utilities COPY, FORMAT and PUTBOOT stay out.
A = ('a',)
DISKS = {
    '8K_1OF4.IMG': {n: A for n in ('DDT.Z8K', 'DUMP.Z8K', 'ED.Z8K',
                                   'NMZ8K.Z8K', 'PIP.Z8K')},
    '8K_2OF4.IMG': {n: A for n in ('AR8K.Z8K', 'LD8K.Z8K', 'SIZEZ8K.Z8K',
                                   'STAT.Z8K', 'ZCC.Z8K', 'ZCC1.Z8K',
                                   'ZCC2.Z8K')},
    '8K_3OF4.IMG': {'LIBCPM.A': ('', 'a'), **{n: A for n in (
        'ASZ8K.Z8K', 'ASZ8K.PD', 'CTYPE.H', 'ERRNO.H', 'OPTION.H',
        'SETJMP.H', 'SIGNAL.H', 'STDIO.H', 'XOUT.H', 'STARTUP.O',
        'XCON.Z8K', 'XDUMP.Z8K', 'ZCC3.Z8K')}},
    '8K_4OF4.IMG': {'CPMSYS.REL': ('',)},
}


def download(url, sha, cache):
    path = os.path.join(cache, url.rsplit('/', 1)[1])
    if os.path.exists(path):
        with open(path, 'rb') as f:
            data = f.read()
        if hashlib.sha256(data).hexdigest() == sha:
            return data
    print('fetch', url)
    req = urllib.request.Request(url, headers={'User-Agent': 'cpm8000-fetch'})
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


def source(top, cache):
    src = zipfile.ZipFile(io.BytesIO(download(URL_SRC, SHA_SRC, cache)))
    for name in SOURCES:
        path = os.path.join(top, 'src', name)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, 'wb') as f:
            f.write(src.read(name))
    open(os.path.join(top, '.fetched-source'), 'w').close()


def main(argv):
    with_source = argv[:1] == ['--source']
    if with_source:
        argv = argv[1:]
    if len(argv) != 1:
        sys.exit(__doc__)
    top = argv[0]
    cache = os.path.join(top, 'dl')
    os.makedirs(cache, exist_ok=True)
    if with_source:
        source(top, cache)
        return
    outer = zipfile.ZipFile(io.BytesIO(download(URL, SHA, cache)))
    for disk, members in DISKS.items():
        image = bytearray(outer.read(disk))
        found = {name: es for (user, name), es in _disk.files(image).items()
                 if user == 0}
        missing = set(members) - set(found)
        if missing:
            sys.exit(f'{disk}: missing {" ".join(sorted(missing))}')
        for name, dirs in members.items():
            data = _disk.contents(image, found[name])
            for d in dirs:
                out = os.path.join(top, d)
                os.makedirs(out, exist_ok=True)
                with open(os.path.join(out, name), 'wb') as f:
                    f.write(data)
    src13 = zipfile.ZipFile(io.BytesIO(download(URL13, SHA13, cache)))
    with open(os.path.join(top, 'a', 'PORTAB.H'), 'wb') as f:
        f.write(src13.read('PORTAB.H'))
    open(os.path.join(top, '.fetched'), 'w').close()


if __name__ == '__main__':
    main(sys.argv[1:])
