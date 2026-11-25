#!/usr/bin/env python3
"""Fetch the Microsoft files the MS-DOS 2.0 disks are built from.

    debugger/i8086/tools/msdos-fetch.py DIR

Downloads each file below from github.com/microsoft/MS-DOS at a pinned
commit (kept, so later runs work offline), checks its sha256, and puts
the system files in DIR/ and the utilities in DIR/bin/. Writes
DIR/.fetched when done.

The system files are MSDOS.SYS and COMMAND.COM, and SYSINIT.OBJ and
SYSIMES.OBJ, Microsoft's halves of IO.SYS, which samples/i8086/Makefile
links with msdosbio.asm. The binaries are the 2.0 release's: MSDOS.SYS
says "version 2.00", and the OEM kit's SYSINIT.OBJ matches it.

All are Microsoft's, MIT licensed (LICENSE in the repository).
"""
import hashlib
import os
import sys
import urllib.error
import urllib.request

COMMIT = '2d04cacc5322951f187bb17e017c12920ac8ebe2'
RAW = f'https://raw.githubusercontent.com/microsoft/MS-DOS/{COMMIT}/v2.0/bin/'

# (name, sha256, subdirectory of DIR)
SOURCES = [
    ('MSDOS.SYS',
     '1edf5190671ed4edcc9e6cc095e5094749c58f1833a2e26e40ba7d3389e276d1', ''),
    ('COMMAND.COM',
     '4cc71b3692b894eef9a7c8b3ac0fc63a71bdfa08175089562fdc4c7c5b038e8a', ''),
    ('SYSINIT.OBJ',
     '56d9d59d8cf6dbec648c3b391d3532bf4dffa8b615a096aca76312acbeecc7aa', ''),
    ('SYSIMES.OBJ',
     '336f44f68eac8061970dcb3b3c4a97558e792cdad4ebfd32720e40174b031a26', ''),
    ('CHKDSK.COM',
     '75be6db096873aed6525e71dcacfe81a56ba038699600d5f203bb40d6a308201', 'bin'),
    ('DEBUG.COM',
     'cbb171acca2a5581d926b6782aebeaad49111ecd359e64cf6f2e5dd0a000b4ca', 'bin'),
    ('EDLIN.COM',
     '799767e5bc6727fd3fb1b0c54a2099c5c53d46dcab5b7b5012b688f3c718b444', 'bin'),
    ('EXE2BIN.EXE',
     '80ac337843421764bb019845f9ef802d35ef642045026b1788373cea756201e2', 'bin'),
    ('FC.EXE',
     '6bf625fcfcd05466f07f3cb7585c22031523d3981cee0f252cf39b7fb4a173e0', 'bin'),
    ('FIND.EXE',
     'd57bab73ba6a77be1aa8d2d3f1fd7d1d6ce6d459b2ab067d26cf63228e940198', 'bin'),
    ('MORE.COM',
     '56830889fef22d56ef5938df0394bf30a8416cc06225b8dc35adc74bb13416b7', 'bin'),
    ('PRINT.COM',
     'b17575ff302d64ba92ec5c26d80355775652eef073c5a3fe099d951553886492', 'bin'),
    ('RECOVER.COM',
     '115a2a262579d1c8831bd6ffaed8ea31e2d156ad5ba399fc365a0ab7d843e8d2', 'bin'),
    ('SORT.EXE',
     'f2135e16cd1ebff6233b4d01fcb7f73b311a1f5524ddca21056248b29fa015b8', 'bin'),
    ('LINK.EXE',
     'fe52892da7bd6832cb88a959cccd230f307e30c7c83a225b2e638f4ea25d76be', 'bin'),
    ('MASM.EXE',
     'bf3399d0ec33fd97c3789887b336208d9f4900524609c04985e16a124cbf4449', 'bin'),
    ('CREF.EXE',
     '9d38031ea566ed01e8c2157f37120e21fd3aa6af2482f0dc329b5ac5bb0fc95e', 'bin'),
]


def fetch(name, sha, path):
    if os.path.exists(path):
        with open(path, 'rb') as f:
            data = f.read()
        if hashlib.sha256(data).hexdigest() == sha:
            return
    url = RAW + name
    print('fetch', url)
    req = urllib.request.Request(url, headers={'User-Agent': 'msdos-fetch'})
    try:
        with urllib.request.urlopen(req) as r:
            data = r.read()
    except urllib.error.URLError as e:
        sys.exit(f'{url}: {e.reason}')
    if hashlib.sha256(data).hexdigest() != sha:
        sys.exit(f'{url}: sha256 mismatch')
    with open(path, 'wb') as f:
        f.write(data)


def main(argv):
    if len(argv) != 1:
        sys.exit(__doc__)
    top = argv[0]
    for name, sha, sub in SOURCES:
        out = os.path.join(top, sub)
        os.makedirs(out, exist_ok=True)
        fetch(name, sha, os.path.join(out, name))
    open(os.path.join(top, '.fetched'), 'w').close()


if __name__ == '__main__':
    main(sys.argv[1:])
