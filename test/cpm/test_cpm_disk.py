#!/usr/bin/env python3
"""Pins the CP/M disk layout that samples/i8080/cpm22bios.asm,
scripts/cpm-disk.py and debugger/sector_disk.cpp share.

    python3 -m unittest discover -s test/cpm
"""
import importlib.util
import os
import re
import tempfile
import unittest

PROJ = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
SAMPLES = os.path.join(PROJ, 'samples', 'i8080')

spec = importlib.util.spec_from_file_location(
    'cpm_disk', os.path.join(PROJ, 'scripts', 'cpm-disk.py'))
cpm_disk = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cpm_disk)

# The 8" single-density tables of DRI's skeletal CBIOS.ASM (CP/M 2.2
# Alteration Guide): trans, and dpblk's SPT BSH BLM EXM DSM DRM AL0 AL1
# CKS OFF.
DRI_TRANS = [1, 7, 13, 19, 25, 5, 11, 17, 23, 3, 9, 15, 21,
             2, 8, 14, 20, 26, 6, 12, 18, 24, 4, 10, 16, 22]
DRI_DPB = bytes([26, 0, 3, 7, 0, 242, 0, 63, 0, 0xC0, 0x00, 16, 0, 2, 0])


def labels(lst):
    found = {}
    with open(lst) as f:
        lines = f.readlines()
    for line in lines:
        m = re.match(r'\s*([0-9A-F]{4}) :\s+(?:[0-9A-F]{2} )*\s*(\w+):', line)
        if m:
            found[m.group(2)] = int(m.group(1), 16)
    return found


def memory(*hexes):
    mem = bytearray(b'\xe5' * 0x10000)
    for path in hexes:
        cpm_disk.read_hex(path, mem)
    return mem


class BiosTables(unittest.TestCase):
    def setUp(self):
        self.mem = memory(os.path.join(SAMPLES, 'cpm22bios.hex'))
        self.at = labels(os.path.join(SAMPLES, 'cpm22bios.lst'))

    def test_skew(self):
        xlt = self.at['xlt']
        self.assertEqual(list(self.mem[xlt:xlt + 26]), DRI_TRANS)
        self.assertEqual(cpm_disk.SKEW, DRI_TRANS)

    def test_dpb(self):
        dpb = self.at['dpb']
        self.assertEqual(bytes(self.mem[dpb:dpb + 15]), DRI_DPB)
        self.assertEqual((cpm_disk.SPT, cpm_disk.DSM, cpm_disk.DRM,
                          cpm_disk.OFF, cpm_disk.BLOCK),
                         (26, 242, 63, 2, 1024))

    def test_bios_fits_its_sectors(self):
        # cpm22.inc: the BIOS ends track 1, after the boot sector and the
        # 44 sectors of CCP and BDOS.
        self.assertLessEqual(self.at['dph0'] + 4 * 16, 0xFA00 + 7 * 128)
        self.assertEqual(self.at['dirbuf'], 0xFA00 + 7 * 128)


class ImageLayout(unittest.TestCase):
    def test_sector_offset(self):
        # debugger/sector_disk.cpp: (track * 26 + sector - 1) * 128
        self.assertEqual(cpm_disk.offset(0, 1), 0)
        self.assertEqual(cpm_disk.offset(1, 1), 26 * 128)
        self.assertEqual(cpm_disk.offset(76, 26), 256256 - 128)
        self.assertEqual(cpm_disk.SIZE, 77 * 26 * 128)

    def test_data_records_skewed(self):
        # Logical record 1 of track 2 is physical sector 7.
        self.assertEqual(cpm_disk.record_offset(0), cpm_disk.offset(2, 1))
        self.assertEqual(cpm_disk.record_offset(1), cpm_disk.offset(2, 7))
        self.assertEqual(cpm_disk.record_offset(26), cpm_disk.offset(3, 1))

    def test_system_tracks(self):
        with tempfile.TemporaryDirectory() as tmp:
            hexfile = os.path.join(tmp, 'sys.hex')
            with open(hexfile, 'w') as f:
                for addr, byte in ((0xE400, 0x11), (0xFA00, 0x22),
                                   (0xFD7F, 0x33)):
                    s = [1, addr >> 8, addr & 0xFF, 0, byte]
                    s.append(-sum(s) & 0xFF)
                    f.write(':' + bytes(s).hex().upper() + '\n')
                f.write(':00000001FF\n')
            out = os.path.join(tmp, 'a.dsk')
            cpm_disk.make(out, [hexfile], [])
            with open(out, 'rb') as f:
                image = f.read()
        # CCP from track 0 sector 2, the BIOS from track 1 sector 20, up
        # to the end of track 1: what cpm22.asm and wboot read.
        self.assertEqual(image[cpm_disk.offset(0, 2)], 0x11)
        self.assertEqual(image[cpm_disk.offset(1, 20)], 0x22)
        self.assertEqual(image[cpm_disk.offset(1, 26) + 127], 0x33)
        self.assertEqual(image[cpm_disk.offset(0, 1)], 0xE5)

    def test_files_round_trip(self):
        with tempfile.TemporaryDirectory() as tmp:
            big = os.path.join(tmp, 'BIG.DAT')
            data = bytes(range(256)) * 80  # 20K: two extents
            with open(big, 'wb') as f:
                f.write(data)
            out = os.path.join(tmp, 'a.dsk')
            cpm_disk.make(out, [], [big])
            image = cpm_disk.load(out)
        found = cpm_disk.files(image)
        entries = found[(0, 'BIG.DAT')]
        self.assertEqual([e[12] for e in entries], [0, 1])  # EX
        self.assertEqual([e[15] for e in entries], [128, 32])  # RC
        self.assertEqual(entries[0][16], 2)  # first block past the directory
        self.assertEqual(cpm_disk.contents(image, entries), data)

    def test_put_and_era(self):
        with tempfile.TemporaryDirectory() as tmp:
            paths = []
            for name, size in (('KEEP.COM', 3000), ('MOVE.BAS', 700)):
                paths.append(os.path.join(tmp, name))
                with open(paths[-1], 'wb') as f:
                    f.write(bytes([len(name)]) * size)
            out = os.path.join(tmp, 'b.dsk')
            open(out, 'wb').close()  # an empty image is a blank disk
            cpm_disk.put(out, paths)
            cpm_disk.era(out, ['MOVE.BAS'])
            cpm_disk.put(out, [paths[0]])  # replaces, not duplicates
            image = cpm_disk.load(out)
        found = cpm_disk.files(image)
        self.assertEqual(list(found), [(0, 'KEEP.COM')])
        self.assertEqual(cpm_disk.contents(image, found[(0, 'KEEP.COM')])[:3000],
                         bytes([8]) * 3000)


if __name__ == '__main__':
    unittest.main()
