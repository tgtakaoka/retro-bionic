#!/usr/bin/env python3
"""List or extract a CP/M .LBR library's members, as LBREXT does on a Z80.

    debugger/i8080/tools/lbr.py list LIB
    debugger/i8080/tools/lbr.py extract LIB OUTDIR

Squeezed (?Q?) and crunched (?Z?) members come out as they are.
"""
import os
import sys


def members(data):
    n = int.from_bytes(data[14:16], 'little')  # directory length in sectors
    for i in range(1, n * 4):
        e = data[i * 32:(i + 1) * 32]
        if len(e) < 32 or e[0] != 0:
            continue
        name = e[1:9].decode('ascii', 'replace').strip()
        typ = e[9:12].decode('ascii', 'replace').strip()
        index = int.from_bytes(e[12:14], 'little')
        length = int.from_bytes(e[14:16], 'little')
        pad = e[26]
        body = data[index * 128:(index + length) * 128]
        if pad and pad < 128 and length:
            body = body[:len(body) - pad]
        yield f'{name}.{typ}' if typ else name, body


def main(argv):
    with open(argv[1], 'rb') as f:
        data = f.read()
    if argv[0] == 'list':
        for name, body in members(data):
            print(f'{name:12s} {len(body):7d}')
    elif argv[0] == 'extract':
        os.makedirs(argv[2], exist_ok=True)
        for name, body in members(data):
            with open(os.path.join(argv[2], name), 'wb') as f:
                f.write(body)
    else:
        sys.exit(__doc__)


if __name__ == '__main__':
    main(sys.argv[1:])
