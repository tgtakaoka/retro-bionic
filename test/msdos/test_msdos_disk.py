#!/usr/bin/env python3
"""Pins the MS-DOS disk layout that samples/i8086/rombios.asm,
samples/i8086/msdosboot.asm, scripts/msdos-disk.py and
debugger/i8086/devs_i8086.cpp share, and scripts/omf-link.py's reading
of Microsoft's objects.

    python3 -m unittest discover -s test/msdos
"""
import importlib.util
import os
import re
import shutil
import struct
import subprocess
import tempfile
import unittest

PROJ = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
SAMPLES = os.path.join(PROJ, 'samples', 'i8086')
DEBUGGER = os.path.join(PROJ, 'debugger')


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


msdos_disk = load('msdos_disk', os.path.join(PROJ, 'scripts', 'msdos-disk.py'))
omf_link = load('omf_link', os.path.join(PROJ, 'scripts', 'omf-link.py'))

GEOMETRY = re.compile(
    r'\{"([\w-]+)",\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(0x[0-9A-Fa-f]+)\}')


def formats(path):
    """The DiskGeometry entries of a devs_*.cpp, in order."""
    with open(path) as f:
        return [(m.group(1),) + tuple(int(m.group(i), 0) for i in range(2, 8))
                for m in GEOMETRY.finditer(f.read())]


def equates(path, names):
    """The values of the named EQUs in an .asm."""
    found = {}
    with open(path) as f:
        for line in f:
            m = re.match(r'(\w+):\s+equ\s+([0-9A-Fa-f]+)H?\b', line)
            if m and m.group(1) in names:
                found[m.group(1)] = int(m.group(2), 16 if 'H' in line.split(
                    'equ')[1][:12].upper() else 10)
    return found


class Formats(unittest.TestCase):
    """The debugger's 8086 formats against the tool's and the BIOS's."""

    def setUp(self):
        self.formats = formats(os.path.join(DEBUGGER, 'i8086', 'pc_disk.cpp'))

    def test_cpm_formats_first(self):
        cpm = formats(os.path.join(DEBUGGER, 'i8080', 'cpm80_disk.cpp'))
        self.assertEqual(self.formats[:2], cpm)

    def test_dos_formats_match_tool(self):
        for name, cylinders, heads, sectors, bytes_, first, fill in self.formats[2:]:
            fmt = msdos_disk.FORMATS[name]
            self.assertEqual(cylinders * heads * sectors * bytes_, fmt.size, name)
            self.assertEqual(bytes_, 512, name)
            if name.startswith('fd'):
                self.assertEqual((cylinders, heads, sectors),
                                 (fmt.cylinders, fmt.heads, fmt.spt), name)
                self.assertEqual(first, 1, name)
            else:
                self.assertEqual((cylinders, heads, sectors), (256, 1, 256), name)
                self.assertEqual(first, 0, name)

    def test_sizes_distinct(self):
        sizes = [c * h * s * b for _, c, h, s, b, _, _ in self.formats]
        self.assertEqual(len(sizes), len(set(sizes)))

    def test_bios_indexes(self):
        names = [f[0] for f in self.formats]
        eq = equates(os.path.join(SAMPLES, 'rombios.asm'),
                     ('FMT_FD360', 'FMT_FD1440', 'FMT_HD32', 'HD_CYLS',
                      'HD_HEADS', 'HD_SPT'))
        self.assertEqual(eq['FMT_FD360'], names.index('fd360'))
        self.assertEqual(eq['FMT_FD1440'], names.index('fd1440'))
        self.assertEqual(eq['FMT_HD32'], names.index('hd32'))
        hd32 = msdos_disk.FORMATS['hd32']
        self.assertEqual((eq['HD_CYLS'], eq['HD_HEADS'], eq['HD_SPT']),
                         (hd32.cylinders, hd32.heads, hd32.spt))
        self.assertEqual(names.index('fd1440') - names.index('fd360'), 3)
        self.assertEqual(names[names.index('fd360'):names.index('fd1440') + 1],
                         ['fd360', 'fd720', 'fd1200', 'fd1440'])

    def test_bios_floppy_table(self):
        """fd_table's heads, sectors a track and cylinders - 1 by index."""
        with open(os.path.join(SAMPLES, 'rombios.asm')) as f:
            src = f.read()
        table = re.search(r'fd_table:\n((?:\s+db\s+[\d, ]+;.*\n)+)', src).group(1)
        rows = [[int(v) for v in re.findall(r'\d+', line.split(';')[0])]
                for line in table.strip().split('\n')]
        for row, name in zip(rows, ('fd360', 'fd720', 'fd1200', 'fd1440')):
            fmt = msdos_disk.FORMATS[name]
            self.assertEqual(row[:3], [fmt.heads, fmt.spt, fmt.cylinders - 1], name)


class Layout(unittest.TestCase):
    """The hard disk's partition and every volume's FAT12 within limits."""

    def test_hd32_partition(self):
        hd32 = msdos_disk.FORMATS['hd32']
        self.assertEqual(hd32.size, 32 * 1024 * 1024)
        self.assertEqual(hd32.partition, 16)
        self.assertEqual(hd32.volume_sectors, 65392)
        self.assertLessEqual(hd32.volume_sectors, 65535)
        self.assertEqual((hd32.cluster, hd32.root, hd32.fat_sectors, hd32.media),
                         (16, 512, 12, 0xF8))
        self.assertEqual(hd32.clusters, 4083)
        table = msdos_disk.partition_table(hd32)
        self.assertEqual(table[0], 0x80)
        self.assertEqual(table[4], 0x01)
        start, count = struct.unpack_from('<II', table, 8)
        self.assertEqual((start, count), (16, 65392))
        self.assertEqual(hd32.chs(start), (0, 1, 1))
        self.assertEqual(hd32.chs(start + count - 1), (1021, 3, 16))

    def test_cluster_counts(self):
        for name, fmt in msdos_disk.FORMATS.items():
            self.assertLessEqual(fmt.clusters, 4084, name)
            # The FAT holds every cluster's entry.
            self.assertGreaterEqual(fmt.fat_sectors * 512 * 2 // 3,
                                    fmt.clusters + 2, name)

    def test_fd360(self):
        fd = msdos_disk.FORMATS['fd360']
        self.assertEqual(fd.bpb(), bytes.fromhex(
            '0002' '02' '0100' '02' '7000' 'd002' 'fd' '0200' '0900' '0200'
            '00000000'))


class Images(unittest.TestCase):
    """Images the tool makes, checked by fsck.fat where it is installed."""

    def setUp(self):
        self.dir = tempfile.mkdtemp()
        self.files = []
        for name, size in (('IO.SYS', 3000), ('MSDOS.SYS', 16690),
                           ('COMMAND.COM', 15480), ('HELLO.TXT', 13)):
            path = os.path.join(self.dir, name)
            with open(path, 'wb') as f:
                f.write(bytes((i * 7 + len(name)) & 0xFF for i in range(size)))
            self.files.append(path)

    def tearDown(self):
        shutil.rmtree(self.dir)

    def make(self, fmt, **kw):
        out = os.path.join(self.dir, fmt + '.dsk')
        msdos_disk.make(out, msdos_disk.FORMATS[fmt],
                        kw.get('boot'), kw.get('mbr'), self.files[:2],
                        self.files[2:])
        return out

    def fsck(self, path, offset=0, sectors=None):
        if not shutil.which('fsck.fat'):
            self.skipTest('no fsck.fat')
        slice_ = path
        if offset:
            slice_ = path + '.part'
            with open(path, 'rb') as f, open(slice_, 'wb') as out:
                f.seek(offset)
                out.write(f.read(sectors * 512))
        r = subprocess.run(['fsck.fat', '-n', slice_], capture_output=True,
                           text=True)
        # A DOS 2.0 boot sector has no volume label, which fsck.fat counts
        # as a fault to fix; anything else it finds is one.
        benign = ('fsck.fat ', 'Label ', '  Auto-removing label',
                  'Checking for unused clusters.',
                  'Leaving filesystem unchanged.', slice_ + ': ')
        for line in (r.stdout + r.stderr).splitlines():
            if line and not line.startswith(benign):
                self.fail(line)
        self.assertIn('Leaving filesystem unchanged.', r.stdout)

    def check_files(self, image):
        with open(image, 'rb') as f:
            volume = msdos_disk.open_volume(bytearray(f.read()))
        names = [msdos_disk.show_name(n) for _, n in volume.entries()]
        self.assertEqual(names, ['IO.SYS', 'MSDOS.SYS', 'COMMAND.COM', 'HELLO.TXT'])
        for path in self.files:
            with open(path, 'rb') as f:
                self.assertEqual(volume.get(msdos_disk.dos_name(path)), f.read())
        # The system files each in one run of clusters, from cluster 2.
        entries = list(volume.entries())
        first, = struct.unpack_from('<H', volume.image, entries[0][0] + 26)
        self.assertEqual(first, 2)
        for e, _ in entries[:2]:
            start, = struct.unpack_from('<H', volume.image, e + 26)
            chain = volume.chain(start)
            self.assertEqual(chain, list(range(start, start + len(chain))))
            self.assertEqual(volume.image[e + 11], 0x07)

    def test_fd360(self):
        image = self.make('fd360', boot=os.path.join(SAMPLES, 'msdosboot.hex'))
        self.assertEqual(os.path.getsize(image), 368640)
        with open(image, 'rb') as f:
            boot = f.read(512)
        self.assertEqual(boot[0], 0xEB)
        self.assertEqual(boot[0x0B:0x20], msdos_disk.FORMATS['fd360'].bpb())
        self.assertEqual(boot[0x1FE:], b'\x55\xAA')
        self.check_files(image)
        self.fsck(image)

    def test_hd32(self):
        image = self.make('hd32', boot=os.path.join(SAMPLES, 'msdosboot.hex'),
                          mbr=os.path.join(SAMPLES, 'mbr.hex'))
        self.assertEqual(os.path.getsize(image), 32 * 1024 * 1024)
        hd32 = msdos_disk.FORMATS['hd32']
        with open(image, 'rb') as f:
            mbr = f.read(512)
            f.seek(hd32.partition * 512)
            boot = f.read(512)
        self.assertEqual(mbr[0x1FE:], b'\x55\xAA')
        self.assertEqual(mbr[0x1BE:0x1CE], msdos_disk.partition_table(hd32))
        self.assertEqual(boot[0x0B:0x20], hd32.bpb())
        self.check_files(image)
        self.fsck(image, hd32.partition * 512, hd32.volume_sectors)


class Link(unittest.TestCase):
    """omf-link.py on Microsoft's objects, when fetched."""

    FETCH = os.path.join(SAMPLES, 'msdos.fetch')

    def setUp(self):
        if not os.path.exists(os.path.join(self.FETCH, '.fetched')):
            self.skipTest('no msdos.fetch')

    def test_sysinit_publics(self):
        m = omf_link.Module(os.path.join(self.FETCH, 'SYSINIT.OBJ'))
        publics = {n: off for n, _, off in m.publics}
        self.assertEqual(publics, {
            'SYSINIT': 0x0000, 'CURRENT_DOS_LOCATION': 0x0005,
            'FINAL_DOS_LOCATION': 0x0009, 'DEVICE_LIST': 0x000B,
            'MEMORY_SIZE': 0x000F, 'DEFAULT_DRIVE': 0x0011, 'BUFFERS': 0x0012,
            'FILES': 0x0013})
        self.assertEqual(m.segments[1][:2], ('SYSINITSEG', 0x0746))
        self.assertEqual(sorted(m.externs[1:]), [
            'BADCOM', 'BADLD', 'BADOPM', 'BADSIZ', 'CRLFM', 'RE_INIT', 'SYSSIZE'])

    def test_link(self):
        modules = [omf_link.Module(os.path.join(self.FETCH, n))
                   for n in ('SYSINIT.OBJ', 'SYSIMES.OBJ')]
        # A 7-byte stand-in BIOS: JMP, RETF, HLT.
        bios = bytes.fromhex('e9020000cbf4')
        link = omf_link.Link(0x70, bios, modules, {'RE_INIT': 3})
        self.assertEqual(link.segment, 0x71)
        self.assertEqual(link.length, 0x746 + 0x6E)
        image = link.image
        sysinit = image[0x10:]
        # CALL RE_INIT, far, to 0070:0003; MOV AX,OFFSET SYSSIZE+15.
        self.assertIn(bytes.fromhex('9a03007000'), sysinit)
        self.assertIn(b'\xb8' + struct.pack('<H', 0x746 + 0x6E + 15), sysinit)
        # The EXEC0 block's far pointers to COMMAND_LINE, DEFAULT_DRIVE
        # and ZERO in SYSINITSEG.
        self.assertEqual(sysinit[0x35:0x43], struct.pack('<HHHHHHH', 0,
                0x14, 0x71, 0x11, 0x71, 0x34, 0x71))
        self.assertIn(b'Bad or missing $', sysinit)


if __name__ == '__main__':
    unittest.main()
