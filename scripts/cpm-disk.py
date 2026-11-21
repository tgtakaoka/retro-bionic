#!/usr/bin/env python3
"""Build and read CP/M 2.2 disk images for the debugger's sector disk.

    scripts/cpm-disk.py make OUT.dsk [--format NAME] [--system HEX...] [FILE...]
    scripts/cpm-disk.py put IMAGE.dsk FILE...
    scripts/cpm-disk.py era IMAGE.dsk NAME.TYP...
    scripts/cpm-disk.py sys IMAGE.dsk HEX...
    scripts/cpm-disk.py list IMAGE.dsk
    scripts/cpm-disk.py get IMAGE.dsk NAME.TYP [OUT]

The formats, with the DPB every CP/M shares for each:

    ibm-3740  the 8" single-sided single-density disk that cpmtools calls
              ibm-3740: 77 tracks of 26 sectors of 128 bytes, sectors from
              1, 1K blocks, 64 directory entries, the data tracks
              interleaved with skew 6. Any image up to 256256 bytes.
    hd8       8 MB: 256 tracks of 256 sectors of 128 bytes, sectors from 0,
              8K blocks, 1024 directory entries, no skew. An image of
              exactly 8388608 bytes.

Both keep two system tracks. An image of any other size is refused, as
the debugger does, and `make` always writes a full-size image (ibm-3740
unless --format says otherwise).

`make` writes an empty disk, with FILE... copied in as user 0. With
--system, the Intel HEX files' bytes from CCP (E400) up to the end of
the system go on the system tracks from track 0 sector 2, unskewed, at
the track and sector numbers samples/i8080/cpm80.asm and the BIOS's warm
boot read: CCP and BDOS assembled for 64K, then samples/i8080/cpm80bios.hex.
Bytes the HEX files leave out are E5.

`put` copies FILE... into an image as user 0, replacing files of the
same name; `era` erases files of user 0 from it, as CP/M's ERA does.
`sys` writes the HEX files' bytes over the system tracks as --system
does, keeping the bytes they leave out: a new BIOS over an image's
CCP and BDOS.
"""
import os
import sys

SECTOR = 128
CCP = 0xE400
# The system's sectors: the boot sector, then CCP, BDOS and BIOS, at
# ibm-3740's track and sector numbers whatever the format.
SYSTEM_SPT = 26
SYSTEM_SECTORS = 2 * SYSTEM_SPT - 1


class Format:
    def __init__(self, name, tracks, spt, first, skew, block, dsm, drm,
                 dir_blocks, off):
        self.name, self.tracks, self.spt, self.first = name, tracks, spt, first
        self.skew, self.block, self.dsm, self.drm = skew, block, dsm, drm
        self.dir_blocks, self.off = dir_blocks, off
        self.size = tracks * spt * SECTOR
        self.records_per_block = block // SECTOR
        # A directory entry's block numbers are bytes up to 255 blocks.
        self.wide = dsm > 255
        self.exm = block // (2048 if self.wide else 1024) - 1
        self.entry_records = (self.exm + 1) * 128

    def offset(self, track, sector):
        """The image offset of physical |sector| on |track|."""
        return (track * self.spt + sector - self.first) * SECTOR

    def record_offset(self, record):
        """The offset of the data area's logical |record|."""
        track, sector = divmod(record, self.spt)
        physical = self.skew[sector] if self.skew else sector + self.first
        return self.offset(self.off + track, physical)


SKEW = [1, 7, 13, 19, 25, 5, 11, 17, 23, 3, 9, 15, 21,
        2, 8, 14, 20, 26, 6, 12, 18, 24, 4, 10, 16, 22]
IBM3740 = Format('ibm-3740', 77, 26, 1, SKEW, 1024, 242, 63, 2, 2)
HD8 = Format('hd8', 256, 256, 0, None, 8192, 1015, 1023, 4, 2)
FORMATS = {f.name: f for f in (IBM3740, HD8)}

# ibm-3740's, as before there were formats.
SPT, TRACKS, OFF, BLOCK, DSM, DRM = (IBM3740.spt, IBM3740.tracks, IBM3740.off,
                                     IBM3740.block, IBM3740.dsm, IBM3740.drm)
DIR_BLOCKS = IBM3740.dir_blocks
SIZE = IBM3740.size
offset = IBM3740.offset
record_offset = IBM3740.record_offset


def format_of(size, path='image'):
    """The format an image of |size| bytes takes, as the debugger picks it."""
    if size <= IBM3740.size:
        return IBM3740
    for f in FORMATS.values():
        if size == f.size:
            return f
    sys.exit(f'{path}: no disk format of {size} bytes')


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


def system_offsets(fmt):
    for i in range(SYSTEM_SECTORS):
        track, sector = divmod(i + 1, SYSTEM_SPT)
        yield i, fmt.offset(track, sector + 1)


def get_system(image, fmt=None):
    """The system tracks from track 0 sector 2 as 64K memory from CCP."""
    fmt = fmt or format_of(len(image))
    memory = bytearray(b'\xe5' * 0x10000)
    for i, at in system_offsets(fmt):
        memory[CCP + i * SECTOR:CCP + (i + 1) * SECTOR] = image[at:at + SECTOR]
    return memory


def put_system(image, hexes, memory=None, fmt=None):
    fmt = fmt or format_of(len(image))
    if memory is None:
        memory = bytearray(b'\xe5' * 0x10000)
    for path in hexes:
        read_hex(path, memory)
    if CCP + SYSTEM_SECTORS * SECTOR > 0x10000:
        sys.exit('system tracks reach past 64K')
    for i, at in system_offsets(fmt):
        image[at:at + SECTOR] = memory[CCP + i * SECTOR:CCP + (i + 1) * SECTOR]


def cpm_name(path):
    base = os.path.basename(path).upper()
    name, _, typ = base.partition('.')
    if not name or len(name) > 8 or len(typ) > 3:
        sys.exit(f'{path}: not an 8.3 name')
    return name.ljust(8).encode(), typ.ljust(3).encode()


def directory(image, fmt=None):
    fmt = fmt or format_of(len(image))
    for i in range(fmt.drm + 1):
        at = fmt.record_offset(i * 32 // SECTOR) + i * 32 % SECTOR
        yield i, at, image[at:at + 32]


def entry_blocks(e, fmt):
    if fmt.wide:
        return [e[16 + 2 * i] | (e[17 + 2 * i] << 8) for i in range(8)]
    return list(e[16:32])


def add_file(image, path, used, entries, fmt=None):
    fmt = fmt or format_of(len(image))
    name, typ = cpm_name(path)
    with open(path, 'rb') as f:
        data = f.read()
    records = (len(data) + SECTOR - 1) // SECTOR
    data += b'\x1a' * (records * SECTOR - len(data))
    count = max(1, (records + fmt.entry_records - 1) // fmt.entry_records)
    for n in range(count):
        part = data[n * fmt.entry_records * SECTOR:
                    (n + 1) * fmt.entry_records * SECTOR]
        part_records = len(part) // SECTOR
        blocks = []
        for b in range(0, len(part), fmt.block):
            block = next((k for k in range(fmt.dir_blocks, fmt.dsm + 1)
                          if k not in used), None)
            if block is None:
                sys.exit(f'{path}: disk full')
            used.add(block)
            blocks.append(block)
            chunk = part[b:b + fmt.block]
            for r in range(0, len(chunk), SECTOR):
                at = fmt.record_offset(block * fmt.records_per_block + r // SECTOR)
                image[at:at + SECTOR] = chunk[r:r + SECTOR]
        # The entry's last logical extent, and its records.
        last = (part_records - 1) // 128 if part_records else 0
        extent = n * (fmt.exm + 1) + last
        rc = part_records - last * 128
        if fmt.wide:
            pointers = b''.join(k.to_bytes(2, 'little') for k in blocks)
        else:
            pointers = bytes(blocks)
        if not entries:
            sys.exit(f'{path}: directory full')
        at = entries.pop(0)
        image[at:at + 32] = (bytes([0]) + name + typ
                             + bytes([extent & 0x1F, 0, extent >> 5, rc])
                             + pointers.ljust(16, b'\0'))


def make(out, hexes, files, fmt=IBM3740):
    image = bytearray(b'\xe5' * fmt.size)
    if hexes:
        put_system(image, hexes, fmt=fmt)
    entries = [at for _, at, _ in directory(image, fmt)]
    used = set(range(fmt.dir_blocks))
    for path in files:
        add_file(image, path, used, entries, fmt)
    with open(out, 'wb') as f:
        f.write(image)


def erase(image, path):
    """Marks user 0's entries of |path|'s name erased; whether any were."""
    name, typ = cpm_name(path)
    found = False
    for _, at, e in directory(image):
        if e[0] == 0 and e[1:9] == name and bytes(c & 0x7F for c in e[9:12]) == typ:
            image[at] = 0xE5
            found = True
    return found


def save(path, image):
    with open(path, 'r+b' if os.path.getsize(path) else 'wb') as f:
        f.write(image)


def era(path, names):
    image = load(path)
    for n in names:
        if not erase(image, n):
            sys.exit(f'{n}: not found')
    save(path, image)


def put(path, paths):
    image = load(path)
    fmt = format_of(len(image))
    for p in paths:
        erase(image, p)
    used = set(range(fmt.dir_blocks))
    entries = []
    for _, at, e in directory(image, fmt):
        if e[0] == 0xE5:
            entries.append(at)
        else:
            used.update(b for b in entry_blocks(e, fmt) if b)
    for p in paths:
        add_file(image, p, used, entries, fmt)
    save(path, image)


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
    fmt = format_of(len(image))
    data = bytearray()
    for e in entries:
        left = (e[12] & fmt.exm) * 128 + e[15]
        for block in entry_blocks(e, fmt):
            if block == 0 or left == 0:
                break
            for r in range(min(left, fmt.records_per_block)):
                at = fmt.record_offset(block * fmt.records_per_block + r)
                data += image[at:at + SECTOR]
            left -= min(left, fmt.records_per_block)
    return bytes(data)


def load(path):
    with open(path, 'rb') as f:
        image = bytearray(f.read())
    fmt = format_of(len(image), path)
    image += b'\xe5' * (fmt.size - len(image))
    return image


def main(argv):
    if len(argv) >= 2 and argv[0] == 'make':
        out, rest = argv[1], argv[2:]
        fmt = IBM3740
        if rest[:1] == ['--format']:
            if len(rest) < 2 or rest[1] not in FORMATS:
                sys.exit('--format needs one of ' + ', '.join(FORMATS))
            fmt = FORMATS[rest[1]]
            rest = rest[2:]
        hexes = []
        if rest[:1] == ['--system']:
            rest = rest[1:]
            while rest and rest[0].lower().endswith('.hex'):
                hexes.append(rest.pop(0))
            if not hexes:
                sys.exit('--system needs HEX files')
        make(out, hexes, rest, fmt)
    elif len(argv) >= 3 and argv[0] == 'put':
        put(argv[1], argv[2:])
    elif len(argv) >= 3 and argv[0] == 'era':
        era(argv[1], argv[2:])
    elif len(argv) >= 3 and argv[0] == 'sys':
        image = load(argv[1])
        put_system(image, argv[2:], get_system(image))
        save(argv[1], image)
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
