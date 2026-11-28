#!/usr/bin/env python3
"""Build Microware's BASIC09 or RunB module from its six source files.

    debugger/mc6809/tools/basic09-build.py SRC DEFS basic09|runb OUT [-DSYM=V...]

SRC is nitros9-languages' basic09/microware/, DEFS NitrOS-9's defs/.
Each of comand, compil, binder, stmts, exprsn and cnvio is assembled
with lwasm as an OS-9 module, then they are joined as Microware joined
them: the first module's header gets the whole size and, in its table
from 0FH, the offsets of the other five; then the header parity and
the module CRC are made again. RunB is the same sources with only the
run-time and math packages (INCLUDED=6). -DH6309=1 builds the HD6309's.
"""
import os
import subprocess
import sys
import tempfile

MODULES = ['comand', 'compil', 'binder', 'stmts', 'exprsn', 'cnvio']


def crc24(data):
    """The OS-9 module CRC of |data|, before its final inversion."""
    crc = 0xFFFFFF
    for byte in data:
        crc ^= byte << 16
        for _ in range(8):
            crc <<= 1
            if crc & 0x1000000:
                crc ^= 0x800063
        crc &= 0xFFFFFF
    return crc


def main(argv):
    if len(argv) < 4 or argv[2] not in ('basic09', 'runb'):
        sys.exit(__doc__.strip())
    src, defs, mode, out, extra = argv[0], argv[1], argv[2], argv[3], argv[4:]
    flags = ['-DNOS9VER=', '-DNOS9MAJ=', '-DNOS9MIN=', '-DCOMMUNITY=1']
    if mode == 'runb':
        flags.append('-DINCLUDED=6')
    image, starts = bytearray(), []
    with tempfile.TemporaryDirectory() as tmp:
        for m in MODULES:
            obj = os.path.join(tmp, m)
            r = subprocess.run(
                ['lwasm', '--no-warn=ifp1', '--6309', '--format=os9',
                 '--pragma=pcaspcr,nosymbolcase,condundefzero,undefextern,'
                 'dollarnotlocal,noforwardrefmax',
                 f'--includedir={src}', f'--includedir={defs}',
                 *flags, *extra, f'--output={obj}',
                 os.path.join(src, m + '.asm')])
            if r.returncode:
                sys.exit(f'{m}.asm: lwasm failed')
            starts.append(len(image))
            with open(obj, 'rb') as f:
                image += f.read()
    size = len(image)
    if size > 0xFFFF:
        sys.exit(f'{out}: {size} bytes, too large')
    image[2:4] = size.to_bytes(2, 'big')
    for i, at in enumerate(starts[1:]):
        image[0x0F + 2 * i:0x11 + 2 * i] = at.to_bytes(2, 'big')
    parity = 0xFF
    for b in image[:8]:
        parity ^= b
    image[8] = parity
    crc = crc24(image[:size - 3]) ^ 0xFFFFFF
    image[size - 3:] = crc.to_bytes(3, 'big')
    if crc24(image) != 0x800FE3:
        sys.exit(f'{out}: CRC check failed')
    with open(out, 'wb') as f:
        f.write(image)


if __name__ == '__main__':
    main(sys.argv[1:])
