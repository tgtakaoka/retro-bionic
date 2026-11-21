#!/usr/bin/env python3
"""Pins the CP/M disk layout that samples/i8080/cpm80bios.asm,
samples/i8086/cpm86bios.asm, scripts/cpm-disk.py and
debugger/sector_disk.cpp share.

    python3 -m unittest discover -s test/cpm
"""
import importlib.util
import os
import re
import tempfile
import unittest

PROJ = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
SAMPLES = os.path.join(PROJ, 'samples', 'i8080')
SAMPLES86 = os.path.join(PROJ, 'samples', 'i8086')

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
# hd8's, which every CP/M shares: SPT 256, BSH 6, BLM 63, EXM 3, DSM 1015,
# DRM 1023, AL0 F0H, AL1 0, CKS 0, OFF 2.
HD8_DPB = bytes([0, 1, 6, 63, 3, 0xF7, 0x03, 0xFF, 0x03, 0xF0, 0x00, 0, 0, 2, 0])


def labels(lst):
    found = {}
    with open(lst) as f:
        lines = f.readlines()
    for line in lines:
        m = re.match(r'(?:\(\d+\))?\s*([0-9A-F]{4}) :\s+(?:[0-9A-F]{2} )*\s*(\w+):',
                     line)
        if m:
            found[m.group(2)] = int(m.group(1), 16)
    return found


def memory(*hexes):
    mem = bytearray(b'\xe5' * 0x10000)
    for path in hexes:
        cpm_disk.read_hex(path, mem)
    return mem


class BiosTables(unittest.TestCase):
    # The BIOS on 8-bit ports and the one on the Z180's 16-bit ports.
    BUILDS = [SAMPLES, os.path.join(PROJ, 'samples', 'z180')]

    def builds(self):
        for d in self.BUILDS:
            with self.subTest(build=os.path.relpath(d, PROJ)):
                yield (memory(os.path.join(d, 'cpm80bios.hex')),
                       labels(os.path.join(d, 'cpm80bios.lst')), d)

    def test_skew(self):
        for mem, at, _ in self.builds():
            self.assertEqual(list(mem[at['xlt']:at['xlt'] + 26]), DRI_TRANS)
        self.assertEqual(cpm_disk.SKEW, DRI_TRANS)

    def test_dpb(self):
        for mem, at, _ in self.builds():
            self.assertEqual(bytes(mem[at['dpb']:at['dpb'] + 15]), DRI_DPB)
        self.assertEqual((cpm_disk.SPT, cpm_disk.DSM, cpm_disk.DRM,
                          cpm_disk.OFF, cpm_disk.BLOCK),
                         (26, 242, 63, 2, 1024))

    def test_hd8_dpb(self):
        for mem, at, _ in self.builds():
            self.assertEqual(bytes(mem[at['dpb_hd8']:at['dpb_hd8'] + 15]), HD8_DPB)
        f = cpm_disk.HD8
        self.assertEqual((f.spt, f.block, f.exm, f.dsm, f.drm, f.dir_blocks, f.off),
                         (256, 8192, 3, 1015, 1023, 4, 2))
        # The data tracks hold exactly DSM+1 blocks.
        self.assertEqual((f.tracks - f.off) * f.spt * 128 // f.block, f.dsm + 1)

    def test_bios_fits_its_sectors(self):
        # cpm80.inc: the BIOS ends track 1, after the boot sector and the
        # 44 sectors of CCP and BDOS; its scratch areas sit above it, up to
        # the top of memory, with an hd8-sized allocation map a drive.
        for _, at, d in self.builds():
            self.assertLessEqual(at['alv'] + 4 * 127, 0x10000)
            self.assertGreaterEqual(at['alv'] - at['dirbuf'], 128 + 4 * 16)
            top = 0
            with open(os.path.join(d, 'cpm80bios.hex')) as f:
                for line in f:
                    rec = bytes.fromhex(line.strip()[1:])
                    if rec[3] == 0:
                        top = max(top, ((rec[1] << 8) | rec[2]) + rec[0])
            self.assertLessEqual(top, at['dirbuf'])
            self.assertLessEqual(top, 0xFA00 + 7 * 128)


class Bios86Tables(unittest.TestCase):
    # read_hex() skips the extended address record, so the memory is the
    # system segment.
    def setUp(self):
        self.mem = memory(os.path.join(SAMPLES86, 'cpm86bios.hex'))
        self.at = labels(os.path.join(SAMPLES86, 'cpm86bios.lst'))

    def test_skew(self):
        xlt = self.at['xlt']
        self.assertEqual(list(self.mem[xlt:xlt + 26]), DRI_TRANS)

    def test_dpb(self):
        dpb = self.at['dpb']
        self.assertEqual(bytes(self.mem[dpb:dpb + 15]), DRI_DPB)

    def test_hd8_dpb(self):
        dpb = self.at['dpb_hd8']
        self.assertEqual(bytes(self.mem[dpb:dpb + 15]), HD8_DPB)

    def test_alv_fits_hd8(self):
        # One bit a block: hd8's DSM 1015 takes 127 bytes a drive.
        alv = [self.mem[self.at['dph0'] + 16 * d + 14] |
               self.mem[self.at['dph0'] + 16 * d + 15] << 8 for d in range(4)]
        self.assertEqual(alv[0], self.at['alv'])
        self.assertTrue(all(b - a >= 1015 // 8 + 1 for a, b in zip(alv, alv[1:])))

    def test_jump_table(self):
        # CP/M-86 System Guide: 21 3-byte jumps at 2500H, INIT to SETIOB.
        names = ['init', 'wboot', 'const', 'conin', 'conout', 'list',
                 'punch', 'reader', 'home', 'seldsk', 'settrk', 'setsec',
                 'setdma', 'read', 'write', 'listst', 'sectran', 'setdmab',
                 'getsegb', 'getiob', 'setiob']
        for i, name in enumerate(names):
            at = 0x2500 + 3 * i
            self.assertEqual(self.mem[at], 0xE9, name)
            target = (at + 3 + (self.mem[at + 1] | self.mem[at + 2] << 8)) & 0xFFFF
            self.assertEqual(target, self.at[name], name)

    def test_reset_vector(self):
        # F000:FFF0 is FFFF0H: JMPF F000:init.
        init = self.at['init']
        self.assertEqual(bytes(self.mem[0xFFF0:0xFFF5]),
                         bytes([0xEA, init & 0xFF, init >> 8, 0x00, 0xF0]))
        with open(os.path.join(SAMPLES86, 'cpm86bios.hex')) as f:
            self.assertEqual(f.readline().strip(), ':02000004000FEB')
        self.assertEqual(cpm_disk.CCP86_SEG, 0xF000)

    def test_ccp86(self):
        with tempfile.TemporaryDirectory() as tmp:
            sys_ = os.path.join(tmp, 'CPM.SYS')
            code = bytearray(range(256)) * 0x26
            code[0x0A] = 8  # an autoload command
            with open(sys_, 'wb') as f:
                f.write(bytes([1, 0x60, 0x02]).ljust(128, b'\0') + code)
            out = os.path.join(tmp, 'ccp.hex')
            cpm_disk.ccp86(sys_, out)
            with open(out) as f:
                lines = f.read().split()
            mem = memory(out)
        self.assertEqual(lines[0], ':02000004000FEB')
        self.assertTrue(all(len(line) < 80 for line in lines))
        self.assertEqual(mem[0x0A], 0)
        self.assertEqual(mem[0x24FF], code[0x24FF])
        self.assertEqual(mem[0x2500], 0xE5)


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
        # to the end of track 1: what cpm80.asm and wboot read.
        self.assertEqual(image[cpm_disk.offset(0, 2)], 0x11)
        self.assertEqual(image[cpm_disk.offset(1, 20)], 0x22)
        self.assertEqual(image[cpm_disk.offset(1, 26) + 127], 0x33)
        self.assertEqual(image[cpm_disk.offset(0, 1)], 0xE5)

    def test_sys_keeps_uncovered_bytes(self):
        image = bytearray(b'\xe5' * cpm_disk.SIZE)
        with tempfile.TemporaryDirectory() as tmp:
            first, second = (os.path.join(tmp, n) for n in ('1.hex', '2.hex'))
            for path, addr, byte in ((first, 0xE400, 0x11), (first, 0xFA00, 0x22),
                                     (second, 0xFA00, 0x33)):
                rec = [1, addr >> 8, addr & 0xFF, 0, byte]
                with open(path, 'a') as f:
                    f.write(':' + bytes(rec + [-sum(rec) & 0xFF]).hex() + '\n')
            cpm_disk.put_system(image, [first])
            cpm_disk.put_system(image, [second], cpm_disk.get_system(image))
        self.assertEqual(image[cpm_disk.offset(0, 2)], 0x11)  # CCP kept
        self.assertEqual(image[cpm_disk.offset(1, 20)], 0x33)  # BIOS new

    def test_format_by_size(self):
        self.assertIs(cpm_disk.format_of(0), cpm_disk.IBM3740)
        self.assertIs(cpm_disk.format_of(256256), cpm_disk.IBM3740)
        self.assertIs(cpm_disk.format_of(8 * 1024 * 1024), cpm_disk.HD8)
        for size in (256257, 8 * 1024 * 1024 - 1, 8 * 1024 * 1024 + 1):
            with self.assertRaises(SystemExit):
                cpm_disk.format_of(size)

    def test_hd8_layout(self):
        f = cpm_disk.HD8
        # Sectors from 0, no skew: record n of track 2 is LSN 512 + n.
        self.assertEqual(f.offset(0, 0), 0)
        self.assertEqual(f.offset(255, 255), 8 * 1024 * 1024 - 128)
        self.assertEqual(f.record_offset(0), 512 * 128)
        self.assertEqual(f.record_offset(257), (512 + 257) * 128)
        # The system at ibm-3740's numbers: sector 1 of track 1 is LSN 257.
        offsets = dict(cpm_disk.system_offsets(f))
        self.assertEqual(offsets[0], 2 * 128)
        self.assertEqual(offsets[25], 257 * 128)

    def test_hd8_files_round_trip(self):
        with tempfile.TemporaryDirectory() as tmp:
            big = os.path.join(tmp, 'BIG.DAT')
            data = bytes(range(256)) * 600  # 150K: three 64K entries
            with open(big, 'wb') as f:
                f.write(data)
            out = os.path.join(tmp, 'h.dsk')
            cpm_disk.make(out, [], [big], cpm_disk.HD8)
            self.assertEqual(os.path.getsize(out), 8 * 1024 * 1024)
            image = cpm_disk.load(out)
        entries = cpm_disk.files(image)[(0, 'BIG.DAT')]
        # 1200 records: 512 an entry; extents 3, 7 and the 10th (9).
        self.assertEqual([e[12] for e in entries], [3, 7, 9])
        self.assertEqual([e[15] for e in entries], [128, 128, 48])
        self.assertEqual(cpm_disk.entry_blocks(entries[0], cpm_disk.HD8)[0], 4)
        self.assertEqual(cpm_disk.contents(image, entries), data)

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
