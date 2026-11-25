#!/usr/bin/env python3
"""Build and read FAT12 disk images for the debugger's sector disk.

    scripts/msdos-disk.py make OUT.dsk --format NAME [--boot BOOT.hex]
        [--mbr MBR.hex] [--system IO.SYS MSDOS.SYS] [FILE...]
    scripts/msdos-disk.py list IMAGE.dsk
    scripts/msdos-disk.py get IMAGE.dsk NAME.EXT [OUT]
    scripts/msdos-disk.py put IMAGE.dsk FILE...

The formats, each an image of exactly its size, as the debugger mounts
them (debugger/i8086/pc_disk.cpp):

    fd360   5.25" double-sided 9-sector floppy: 40 x 2 x 9 sectors of
            512 bytes, 2 sectors a cluster, 112 root entries, media FD.
    fd720   3.5" double density: 80 x 2 x 9, 2 a cluster, 112, F9.
    fd1200  5.25" high density: 80 x 2 x 15, 1 a cluster, 224, F9.
    fd1440  3.5" high density: 80 x 2 x 18, 1 a cluster, 224, F0.
    hd32    a 32 MB hard disk: 65536 sectors of 512 bytes, which the
            ROM BIOS addresses as 1024 cylinders x 4 heads x 16 sectors.
            Its MBR holds one active FAT12 partition from sector 16 (head
            1 of cylinder 0) through cylinder 1021: 65392 sectors, 16 a
            cluster, 512 root entries, media F8, 4083 clusters, within
            FAT12's 4084.

`make` writes an empty disk, its boot sector the code of BOOT.hex (a
samples/i8086/msdosboot.hex, at 7C00H) with the format's BPB filled in,
and, for hd32, MBR.hex (samples/i8086/mbr.hex, at 0600H, where it moves
itself) with the partition table filled in. --system puts IO.SYS and MSDOS.SYS first in
the root directory, each in one run of clusters, hidden, system and
read-only, as the boot sector expects; FILE... follow. A file's name on
the disk is its base name in upper case. Every file is stamped
1983-03-08 12:00, so a build is the same bytes each time.

`list` shows the root directory, `get` copies a file out, and `put`
copies files in, replacing files of the same name.
"""
import os
import struct
import sys

SECTOR = 512
# DOS date and time of every file: 1983-03-08 12:00.
STAMP_DATE = ((1983 - 1980) << 9) | (3 << 5) | 8
STAMP_TIME = 12 << 11
ATTR_SYSTEM = 0x07  # read-only, hidden, system
FAT12_MAX_CLUSTERS = 4084


class Format:
    """A disk's geometry, and its one FAT12 volume's BPB."""

    def __init__(self, name, cylinders, heads, spt, cluster, root, media,
            partition=0, volume_sectors=None):
        self.name = name
        self.cylinders, self.heads, self.spt = cylinders, heads, spt
        self.sectors = cylinders * heads * spt
        self.size = self.sectors * SECTOR
        self.cluster = cluster  # sectors
        self.root = root  # entries
        self.media = media
        self.partition = partition  # the volume's first sector, hidden
        self.volume_sectors = volume_sectors or self.sectors
        self.reserved = 1
        self.fats = 2
        self.root_sectors = root * 32 // SECTOR
        # The FAT holds two entries before the clusters'.
        self.fat_sectors = 1
        while True:
            data = (self.volume_sectors - self.reserved -
                    self.fats * self.fat_sectors - self.root_sectors)
            self.clusters = data // cluster
            if (self.clusters + 2) * 3 // 2 <= self.fat_sectors * SECTOR:
                break
            self.fat_sectors += 1
        self.data_sector = (self.reserved + self.fats * self.fat_sectors +
                            self.root_sectors)

    def bpb(self):
        """The 21 bytes at boot sector offset 0BH: the DOS 2.0 BPB and
        the sectors a track, heads and hidden sectors after it, with
        hidden sectors 4 bytes wide as DOS 3.31 and later read them."""
        return struct.pack('<HBHBHHBHHHI', SECTOR, self.cluster, self.reserved,
                self.fats, self.root, self.volume_sectors, self.media,
                self.fat_sectors, self.spt, self.heads, self.partition)

    def chs(self, lba):
        """(cylinder, head, sector) of |lba|, sectors from 1."""
        cylinder, rest = divmod(lba, self.heads * self.spt)
        head, sector = divmod(rest, self.spt)
        return cylinder, head, sector + 1


def hd32():
    """The hard disk: its partition ends on the last cylinder that keeps
    the cluster count within FAT12."""
    cylinders, heads, spt = 1024, 4, 16
    start = spt  # head 1 of cylinder 0
    for end in range(cylinders, 0, -1):
        f = Format('hd32', cylinders, heads, spt, 16, 512, 0xF8, start,
                   end * heads * spt - start)
        if f.clusters <= FAT12_MAX_CLUSTERS:
            return f


FORMATS = {
    'fd360': Format('fd360', 40, 2, 9, 2, 112, 0xFD),
    'fd720': Format('fd720', 80, 2, 9, 2, 112, 0xF9),
    'fd1200': Format('fd1200', 80, 2, 15, 1, 224, 0xF9),
    'fd1440': Format('fd1440', 80, 2, 18, 1, 224, 0xF0),
    'hd32': hd32(),
}
assert all(f.clusters <= FAT12_MAX_CLUSTERS for f in FORMATS.values())


def read_hex(path, base):
    """The Intel HEX file's bytes from |base|, as far as they reach."""
    mem = {}
    ext = 0
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line.startswith(':'):
                continue
            rec = bytes.fromhex(line[1:])
            count, addr, kind = rec[0], (rec[1] << 8) | rec[2], rec[3]
            if kind == 0:
                for i in range(count):
                    mem[ext + addr + i] = rec[4 + i]
            elif kind == 4:
                ext = ((rec[4] << 8) | rec[5]) << 16
            elif kind == 2:
                ext = ((rec[4] << 8) | rec[5]) << 4
    if not mem or min(mem) != base:
        sys.exit(f'{path}: not at {base:X}')
    return bytes(mem.get(a, 0) for a in range(base, max(mem) + 1))


def dos_name(path):
    """|path|'s base name as the 11-byte directory field."""
    base = os.path.basename(path).upper()
    name, _, ext = base.partition('.')
    if not name or len(name) > 8 or len(ext) > 3 or '.' in ext:
        sys.exit(f'{path}: not an 8.3 name')
    return name.ljust(8).encode('ascii') + ext.ljust(3).encode('ascii')


def show_name(field):
    name = field[:8].decode('ascii').rstrip()
    ext = field[8:].decode('ascii').rstrip()
    return f'{name}.{ext}' if ext else name


class Volume:
    """A FAT12 volume in an image: its bytes, from the boot sector."""

    def __init__(self, image, start):
        self.image = image  # bytearray of the whole disk
        self.start = start  # byte offset of the boot sector
        bpb = image[start + 0x0B:start + 0x0B + 21]
        (sector, self.cluster, self.reserved, self.fats, self.root,
         self.sectors, self.media, self.fat_sectors, self.spt, self.heads,
         hidden) = struct.unpack('<HBHBHHBHHHI', bpb)
        if sector != SECTOR or self.cluster == 0 or self.fats == 0:
            sys.exit('not a FAT12 volume')
        self.root_sectors = self.root * 32 // SECTOR
        self.data_sector = (self.reserved + self.fats * self.fat_sectors +
                            self.root_sectors)
        self.clusters = (self.sectors - self.data_sector) // self.cluster

    def at(self, sector):
        return self.start + sector * SECTOR

    def fat(self, cluster):
        at = self.at(self.reserved) + cluster * 3 // 2
        v = self.image[at] | (self.image[at + 1] << 8)
        return v >> 4 if cluster & 1 else v & 0xFFF

    def set_fat(self, cluster, value):
        for f in range(self.fats):
            at = (self.at(self.reserved + f * self.fat_sectors) +
                  cluster * 3 // 2)
            v = self.image[at] | (self.image[at + 1] << 8)
            if cluster & 1:
                v = (v & 0x000F) | (value << 4)
            else:
                v = (v & 0xF000) | value
            self.image[at] = v & 0xFF
            self.image[at + 1] = v >> 8

    def format(self, fmt):
        """Writes empty FATs and root directory."""
        for f in range(self.fats):
            at = self.at(self.reserved + f * self.fat_sectors)
            self.image[at:at + self.fat_sectors * SECTOR] = bytes(
                    self.fat_sectors * SECTOR)
        self.set_fat(0, 0xF00 | self.media)
        self.set_fat(1, 0xFFF)
        at = self.at(self.reserved + self.fats * self.fat_sectors)
        self.image[at:at + self.root_sectors * SECTOR] = bytes(
                self.root_sectors * SECTOR)

    def entries(self):
        """(offset in image, name field) of each root entry in use."""
        at = self.at(self.reserved + self.fats * self.fat_sectors)
        for i in range(self.root):
            e = at + i * 32
            first = self.image[e]
            if first == 0:
                return
            if first != 0xE5:
                yield e, bytes(self.image[e:e + 11])

    def free_entry(self):
        at = self.at(self.reserved + self.fats * self.fat_sectors)
        for i in range(self.root):
            e = at + i * 32
            if self.image[e] in (0, 0xE5):
                return e
        sys.exit('root directory full')

    def chain(self, first):
        """The clusters of a file from |first|."""
        clusters = []
        c = first
        while 2 <= c < 0xFF0 and len(clusters) <= self.clusters:
            clusters.append(c)
            c = self.fat(c)
        return clusters

    def cluster_at(self, cluster):
        return self.at(self.data_sector + (cluster - 2) * self.cluster)

    def free_clusters(self):
        return [c for c in range(2, self.clusters + 2) if self.fat(c) == 0]

    def remove(self, name):
        for e, n in list(self.entries()):
            if n == name:
                first = self.image[e + 26] | (self.image[e + 27] << 8)
                for c in self.chain(first):
                    self.set_fat(c, 0)
                self.image[e] = 0xE5

    def add(self, path, attr=0x20, contiguous=False):
        """Copies |path| in, replacing a file of its name, in one run of
        clusters when |contiguous|."""
        name = dos_name(path)
        self.remove(name)
        with open(path, 'rb') as f:
            data = f.read()
        bytes_a_cluster = self.cluster * SECTOR
        need = (len(data) + bytes_a_cluster - 1) // bytes_a_cluster
        free = self.free_clusters()
        if contiguous:
            run = [c for i, c in enumerate(free)
                   if free[i:i + need] == list(range(c, c + need))]
            if not run:
                sys.exit(f'{path}: no {need} clusters in a run')
            free = list(range(run[0], run[0] + need))
        if len(free) < need:
            sys.exit(f'{path}: disk full')
        clusters = free[:need]
        for i, c in enumerate(clusters):
            self.set_fat(c, clusters[i + 1] if i + 1 < need else 0xFFF)
            at = self.cluster_at(c)
            chunk = data[i * bytes_a_cluster:(i + 1) * bytes_a_cluster]
            self.image[at:at + bytes_a_cluster] = chunk.ljust(
                    bytes_a_cluster, b'\0')
        e = self.free_entry()
        self.image[e:e + 32] = struct.pack('<11sB10xHHHI', name, attr,
                STAMP_TIME, STAMP_DATE, clusters[0] if clusters else 0,
                len(data))

    def get(self, name):
        for e, n in self.entries():
            if n == name:
                first, size = struct.unpack('<HI', self.image[e + 26:e + 32])
                data = bytearray()
                for c in self.chain(first):
                    at = self.cluster_at(c)
                    data += self.image[at:at + self.cluster * SECTOR]
                return bytes(data[:size])
        sys.exit(f'{show_name(name)}: not found')

    def list(self):
        used = 0
        for e, n in self.entries():
            attr, = struct.unpack_from('<B', self.image, e + 11)
            first, size = struct.unpack('<HI', self.image[e + 26:e + 32])
            flags = ''.join(f if attr & bit else '-' for f, bit in
                            (('r', 1), ('h', 2), ('s', 4), ('a', 0x20)))
            used += len(self.chain(first))
            print(f'{show_name(n):14} {size:8} {flags}')
        free = self.clusters - used
        print(f'{free} of {self.clusters} clusters free, '
              f'{free * self.cluster * SECTOR} bytes')


def open_volume(image):
    """The image's volume: behind its MBR's first partition, or at 0."""
    if image[0x1FE:0x200] == b'\x55\xAA' and image[0x0B:0x0D] != b'\x00\x02':
        start, = struct.unpack_from('<I', image, 0x1BE + 8)
        return Volume(image, start * SECTOR)
    return Volume(image, 0)


def partition_table(fmt):
    """The MBR's one entry: active, FAT12, the volume's extent."""
    first = fmt.chs(fmt.partition)
    last = fmt.chs(fmt.partition + fmt.volume_sectors - 1)

    def chs_bytes(c, h, s):
        return bytes([h, ((c >> 2) & 0xC0) | s, c & 0xFF])

    return (bytes([0x80]) + chs_bytes(*first) + bytes([0x01]) +
            chs_bytes(*last) + struct.pack('<II', fmt.partition,
                                           fmt.volume_sectors))


def make(out, fmt, boot, mbr, system, files):
    image = bytearray(fmt.size)
    if boot:
        sector = bytearray(read_hex(boot, 0x7C00).ljust(SECTOR, b'\0'))
        if len(sector) != SECTOR:
            sys.exit(f'{boot}: longer than a sector')
    else:
        sector = bytearray(SECTOR)
        sector[0x1FE:0x200] = b'\x55\xAA'
    sector[0x0B:0x0B + 21] = fmt.bpb()
    start = fmt.partition * SECTOR
    image[start:start + SECTOR] = sector
    if fmt.partition:
        if mbr:
            sector = bytearray(read_hex(mbr, 0x600).ljust(SECTOR, b'\0'))
            if len(sector) != SECTOR:
                sys.exit(f'{mbr}: longer than a sector')
        else:
            sector = bytearray(SECTOR)
        sector[0x1BE:0x1CE] = partition_table(fmt)
        sector[0x1CE:0x1FE] = bytes(0x30)
        sector[0x1FE:0x200] = b'\x55\xAA'
        image[0:SECTOR] = sector
    elif mbr:
        sys.exit(f'{fmt.name}: a floppy has no MBR')
    volume = Volume(image, start)
    volume.format(fmt)
    for path in system:
        volume.add(path, ATTR_SYSTEM, contiguous=True)
    for path in files:
        volume.add(path)
    with open(out, 'wb') as f:
        f.write(image)


def load(path):
    with open(path, 'rb') as f:
        return bytearray(f.read())


def main(argv):
    if len(argv) < 2:
        sys.exit(__doc__)
    command, path, argv = argv[0], argv[1], argv[2:]
    if command == 'make':
        fmt = boot = mbr = None
        system, files = [], []
        i = 0
        while i < len(argv):
            a = argv[i]
            if a == '--format':
                fmt = FORMATS.get(argv[i + 1])
                if fmt is None:
                    sys.exit(f'{argv[i + 1]}: no such format')
                i += 2
            elif a == '--boot':
                boot = argv[i + 1]
                i += 2
            elif a == '--mbr':
                mbr = argv[i + 1]
                i += 2
            elif a == '--system':
                system = argv[i + 1:i + 3]
                i += 3
            else:
                files.append(a)
                i += 1
        if fmt is None:
            sys.exit('--format is needed')
        make(path, fmt, boot, mbr, system, files)
    elif command == 'list':
        open_volume(load(path)).list()
    elif command == 'get':
        if not argv:
            sys.exit(__doc__)
        data = open_volume(load(path)).get(dos_name(argv[0]))
        with open(argv[1] if len(argv) > 1 else os.path.basename(argv[0]),
                  'wb') as f:
            f.write(data)
    elif command == 'put':
        image = load(path)
        volume = open_volume(image)
        for p in argv:
            volume.add(p)
        with open(path, 'wb') as f:
            f.write(image)
    else:
        sys.exit(__doc__)


if __name__ == '__main__':
    main(sys.argv[1:])
