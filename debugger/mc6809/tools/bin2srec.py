#!/usr/bin/env python3
"""Write binary files as Motorola S-records at the addresses given.

    debugger/mc6809/tools/bin2srec.py OUT.s19 ADDR FILE [ADDR FILE...]

Each FILE's bytes go at ADDR (hexadecimal, 0x prefix or not), as S1
records of 32 bytes, with an S9 record at the end, as the debugger's L
and U commands take them.
"""
import sys


def record(kind, addr, data):
    body = bytes([len(data) + 3, addr >> 8, addr & 0xFF]) + data
    return kind + (body + bytes([~sum(body) & 0xFF])).hex().upper() + '\n'


def main(argv):
    if len(argv) < 3 or len(argv) % 2 == 0:
        sys.exit(__doc__.strip())
    out, pairs = argv[0], list(zip(argv[1::2], argv[2::2]))
    with open(out, 'w') as f:
        for addr, path in pairs:
            at = int(addr, 16)
            with open(path, 'rb') as b:
                data = b.read()
            if at + len(data) > 0x10000:
                sys.exit(f'{path}: {len(data)} bytes at {at:04X} pass FFFF')
            for i in range(0, len(data), 32):
                f.write(record('S1', at + i, data[i:i + 32]))
        f.write(record('S9', 0, b''))


if __name__ == '__main__':
    main(sys.argv[1:])
