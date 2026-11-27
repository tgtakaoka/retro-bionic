#!/usr/bin/env python3
"""Pins what scripts/bionic-control.py reads out of the debugger's replies.

    python3 -m unittest discover -s test/scripts
"""
import os
import unittest
from importlib.machinery import SourceFileLoader

PROJ = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
bc = SourceFileLoader(
    'bc', os.path.join(PROJ, 'scripts', 'bionic-control.py')).load_module()

# O's listing with drives mounted (disk_drives.cpp printDrive, then
# disk_image.cpp printCache), with none, and on a target without a disk.
MOUNTED = '''Disk? 
  A    hd8       i8080/cpm80a.dsk
  B    hd8       i8080/cpm80b.dsk 0.3 MB cached
  rk0  rk05      dcj11/unix_v6.dsk+512K cached
  cache 8.0 MB write-through
> '''
NONE = '''Disk? 
  no disk mounted
  cache off
> '''
NO_DISK = '''Disk? 
?no disk
> '''


class DrivesOf(unittest.TestCase):
    def test_mounted(self):
        self.assertEqual(bc.drives_of(MOUNTED), ['A', 'B', 'rk0'])

    def test_none_mounted(self):
        self.assertEqual(bc.drives_of(NONE), [])

    def test_no_disk(self):
        self.assertEqual(bc.drives_of(NO_DISK), [])


if __name__ == '__main__':
    unittest.main()
