#!/usr/bin/env python3
"""Build and read CP/M 2.2 disk images for the debugger's sector disk.

    scripts/cpm-disk.py make OUT.dsk [--system HEX...] [FILE...]
    scripts/cpm-disk.py list IMAGE.dsk
    scripts/cpm-disk.py get IMAGE.dsk NAME.TYP [OUT]

The images are 8" single-sided single-density disks, the IBM 3740
format that cpmtools calls ibm-3740: 77 tracks of 26 sectors of 128
bytes, 1K blocks, 64 directory entries, two system tracks, and the data
tracks interleaved with skew 6.

`make` writes an empty disk, with FILE... copied in as user 0. With
--system, the Intel HEX files' bytes from CCP (E400) up to the end of
track 1 go on the system tracks from track 0 sector 2, where the BIOS's
warm boot and samples/i8080/cpm80.asm read them, unskewed: CCP and BDOS
assembled for 64K, then samples/i8080/cpm80bios.hex. Bytes the HEX files
leave out are E5.
"""
import os
import sys

SPT = 26
TRACKS = 77
SECTOR = 128
OFF = 2
BLOCK = 1024
DSM = 242
DRM = 63
DIR_BLOCKS = 2
CCP = 0xE400
SKEW = [1, 7, 13, 19, 25, 5, 11, 17, 23, 3, 9, 15, 21,
        2, 8, 14, 20, 26, 6, 12, 18, 24, 4, 10, 16, 22]
SIZE = TRACKS * SPT * SECTOR
RECORDS_PER_BLOCK = BLOCK // SECTOR
EXTENT_RECORDS = 128


def offset(track, sector):
    """The image offset of physical |sector| (1-26) on |track|."""
    return (track * SPT + sector - 1) * SECTOR


def record_offset(record):
    """The offset of the data area's logical |record|, through the skew."""
    track, sector = divmod(record, SPT)
    return offset(OFF + track, SKEW[sector])


def read_hex(path, memory):
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line.startswith(':'):
                continue
            data = bytes.fromhex(line[1:])
            count, addr, kind = data[0], (data[1] << 8) | data[2], data[3]
            if kind == 0:
                memory[addr:addr + count] = data[4:4 + count]
            elif kind == 1:
                break


def put_system(image, hexes):
    memory = bytearray(b'\xe5' * 0x10000)
    for path in hexes:
        read_hex(path, memory)
    sectors = OFF * SPT - 1
    end = CCP + sectors * SECTOR
    if end > 0x10000:
        sys.exit('system tracks reach past 64K')
    for i in range(sectors):
        track, sector = divmod(i + 1, SPT)
        at = offset(track, sector + 1)
        image[at:at + SECTOR] = memory[CCP + i * SECTOR:CCP + (i + 1) * SECTOR]


def cpm_name(path):
    base = os.path.basename(path).upper()
    name, _, typ = base.partition('.')
    if not name or len(name) > 8 or len(typ) > 3:
        sys.exit(f'{path}: not an 8.3 name')
    return name.ljust(8).encode(), typ.ljust(3).encode()


def directory(image):
    for i in range(DRM + 1):
        at = record_offset(i * 32 // SECTOR) + i * 32 % SECTOR
        yield i, at, image[at:at + 32]


def add_file(image, path, used, entries):
    name, typ = cpm_name(path)
    with open(path, 'rb') as f:
        data = f.read()
    records = (len(data) + SECTOR - 1) // SECTOR
    data += b'\x1a' * (records * SECTOR - len(data))
    for extent in range(max(1, (records + EXTENT_RECORDS - 1) // EXTENT_RECORDS)):
        part = data[extent * EXTENT_RECORDS * SECTOR:
                    (extent + 1) * EXTENT_RECORDS * SECTOR]
        rc = len(part) // SECTOR
        blocks = []
        for b in range(0, len(part), BLOCK):
            block = next((n for n in range(DIR_BLOCKS, DSM + 1) if n not in used), None)
            if block is None:
                sys.exit(f'{path}: disk full')
            used.add(block)
            blocks.append(block)
            chunk = part[b:b + BLOCK]
            for r in range(0, len(chunk), SECTOR):
                at = record_offset(block * RECORDS_PER_BLOCK + r // SECTOR)
                image[at:at + SECTOR] = chunk[r:r + SECTOR]
        if not entries:
            sys.exit(f'{path}: directory full')
        at = entries.pop(0)
        image[at:at + 32] = (bytes([0]) + name + typ + bytes([extent, 0, 0, rc])
                             + bytes(blocks).ljust(16, b'\0'))


def make(out, hexes, files):
    image = bytearray(b'\xe5' * SIZE)
    if hexes:
        put_system(image, hexes)
    entries = [at for _, at, _ in directory(image)]
    used = set(range(DIR_BLOCKS))
    for path in files:
        add_file(image, path, used, entries)
    with open(out, 'wb') as f:
        f.write(image)


def files(image):
    """{(user, NAME.TYP): [entry...]} sorted by extent."""
    found = {}
    for _, _, e in directory(image):
        if e[0] > 15:
            continue
        name = e[1:9].decode('ascii', 'replace').strip()
        typ = bytes(c & 0x7F for c in e[9:12]).decode('ascii', 'replace').strip()
        found.setdefault((e[0], f'{name}.{typ}'), []).append(e)
    for es in found.values():
        es.sort(key=lambda e: (e[14], e[12]))
    return found


def contents(image, entries):
    data = bytearray()
    for e in entries:
        left = e[15]
        for block in e[16:32]:
            if block == 0 or left == 0:
                break
            for r in range(min(left, RECORDS_PER_BLOCK)):
                at = record_offset(block * RECORDS_PER_BLOCK + r)
                data += image[at:at + SECTOR]
            left -= min(left, RECORDS_PER_BLOCK)
    return bytes(data)


def load(path):
    with open(path, 'rb') as f:
        image = bytearray(f.read())
    image += b'\xe5' * (SIZE - len(image))
    return image


def main(argv):
    if len(argv) >= 2 and argv[0] == 'make':
        out, rest = argv[1], argv[2:]
        hexes = []
        if rest[:1] == ['--system']:
            rest = rest[1:]
            while rest and rest[0].lower().endswith('.hex'):
                hexes.append(rest.pop(0))
            if not hexes:
                sys.exit('--system needs HEX files')
        make(out, hexes, rest)
    elif len(argv) == 2 and argv[0] == 'list':
        image = load(argv[1])
        for (user, name), es in sorted(files(image).items()):
            records = len(contents(image, es)) // SECTOR
            print(f'{user:2d}: {name:12s} {(records * SECTOR + 1023) // 1024:4d}K')
    elif len(argv) in (3, 4) and argv[0] == 'get':
        image = load(argv[1])
        want = argv[2].upper()
        es = next((es for (u, n), es in files(image).items() if n == want), None)
        if es is None:
            sys.exit(f'{want}: not found')
        with open(argv[3] if len(argv) == 4 else want.lower(), 'wb') as f:
            f.write(contents(image, es))
    else:
        sys.exit(__doc__)


if __name__ == '__main__':
    main(sys.argv[1:])
