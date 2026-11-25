#!/usr/bin/env python3
"""Install 2.11BSD from its distribution tape onto an MSCP disk image.

    debugger/pdp11/tools/bsd211-install.py PDP11 TAPE IMAGE [FILE...]

Runs open-simh's PDP11 (simh-fetch.py's), an 11/73 with 4 MB, an RQDX3 with an RD54 and a
TS11 holding TAPE (bsd211-fetch.py's), and types the installation at
its console as 2.11BSD's setup document has it: the disk label (a root
of 15884 blocks, a swap of 16720, the rest /usr), mkfs, the root from
the tape, the boot block, /usr and the sources by tar, the mounts in
/etc/fstab, and /etc/dtab cut to the MSCP controller, the one device
the kernel is to look for. Each FILE goes into /usr/root, from a tar on
a second tape. Then boots the disk from its block 0 as the board will,
to the single-user prompt, and halts. IMAGE is the RD54's 311200
blocks; IMAGE.log is the console's transcript.
"""
import io
import os
import re
import struct
import sys
import tarfile
import time

import pexpect

HOME = '/usr/root'
RECORD = 10240  # tar's blocking factor 20


def tape_of(files, path):
    """|files| as a tar on a simh tape at |path|: records, a tape mark."""
    buf = io.BytesIO()
    with tarfile.open(fileobj=buf, mode='w', format=tarfile.USTAR_FORMAT) as tar:
        for f in files:
            info = tar.gettarinfo(f, arcname=os.path.basename(f))
            info.uid = info.gid = 0
            info.uname, info.gname = 'root', 'wheel'
            info.mode = 0o644
            with open(f, 'rb') as data:
                tar.addfile(info, data)
    data = buf.getvalue()
    data += b'\0' * (-len(data) % RECORD)
    with open(path, 'wb') as out:
        for i in range(0, len(data), RECORD):
            out.write(struct.pack('<I', RECORD) + data[i:i + RECORD] +
                      struct.pack('<I', RECORD))
        out.write(struct.pack('<I', 0) * 2)

ROOT_BLOCKS = 15884
SWAP_BLOCKS = 16720


def main(argv):
    if len(argv) < 3:
        sys.exit(__doc__)
    pdp11, tape, image = argv[:3]
    files = argv[3:]
    pdp11, tape = os.path.abspath(pdp11), os.path.abspath(tape)
    files_tape = os.path.abspath(image + '.files.tap')
    if files:
        tape_of(files, files_tape)
    new = os.path.abspath(image + '.new')
    if os.path.exists(new):
        os.remove(new)
    ini = os.path.abspath(image + '.ini')
    with open(ini, 'w') as f:
        f.write(f"""set cpu 11/73
set cpu 4m
set rq enable
set rq rqdx3
set rq0 rd54
set rq1 disable
set rq2 disable
set rq3 disable
set ts enable
set rl disable
set rp disable
set rx disable
set tq disable
set xq disable
set dz disable
attach rq0 {new}
attach -r ts0 {tape}
""")
    log = open(image + '.log', 'wb')
    t0 = time.time()

    def mark(what):
        print(f'{time.time() - t0:6.0f} s  {what}', flush=True)

    c = pexpect.spawn(pdp11, [ini], timeout=600, logfile=log, encoding=None)
    c.expect(rb'sim> ')
    c.sendline(b'boot ts0')
    c.expect(rb'Boot from ts\(0,0,0\) at 017252[02]')
    c.expect(rb': ')
    mark('tape boot')

    # disklabel, tape file 1: single keys at its menus, Return after a value.
    c.sendline(b'ts(0,1)')
    c.expect(rb'Disk\? ')
    c.sendline(b'ra(0,0)')
    TOP = rb'd\(isplay\) D\(efault\) m\(odify\) w\(rite\) q\(uit\)\? '
    MOD = rb'd\(isplay\) g\(eometry\) m\(isc\) p\(artitions\) q\(uit\)\? '
    PART = rb'd\(isplay\) n\(umber\) s\(elect\) q\(uit\)\? '
    SEL = rb'a b c d e f g h q\(uit\)\? '
    FIELD = rb'd\(isplay\) z\(ero\) t\(ype\) o\(ffset\) s\(ize\) f\(rag\) ' \
            rb'F\(size\) q\(uit\)\? '
    if c.expect([rb'Proceed\? \[y/n\]', TOP]) == 0:  # no label yet
        c.send(b'y')
        c.expect(TOP)
    c.send(b'D')  # the default label, from the controller's unit size
    c.expect(TOP)
    c.send(b'd')
    c.expect(TOP)
    total = int(re.search(rb'a:\s+(\d+)\s+0\s+2\.11BSD', c.before).group(1))
    usr = ROOT_BLOCKS + SWAP_BLOCKS
    mark(f'disk of {total} blocks')
    c.send(b'm')
    c.expect(MOD)
    c.send(b'p')

    def partition(letter, offset, size, fstype):
        c.expect(PART)
        c.send(b's')
        c.expect(SEL)
        c.send(letter)
        for key, prompt, value in ((b'o', rb'offset \[.*\]: ', offset),
                                   (b's', rb'size \[.*\]: ', size),
                                   (b't', rb'fstype \[.*\]: ', fstype)):
            c.expect(FIELD)
            c.send(key)
            c.expect(prompt)
            c.sendline(str(value).encode())
        c.expect(FIELD)
        c.send(b'q')

    partition(b'a', 0, ROOT_BLOCKS, '2.11BSD')
    partition(b'b', ROOT_BLOCKS, SWAP_BLOCKS, 'swap')
    partition(b'c', 0, total, 'unused')
    partition(b'g', usr, total - usr, '2.11BSD')
    c.expect(PART)
    c.send(b'n')
    c.expect(rb'Number of partitions \(8 max\) \[.*\]\? ')
    c.sendline(b'7')
    c.expect(PART)
    c.send(b'q')
    c.expect(MOD)
    c.send(b'q')
    c.expect(TOP)
    c.send(b'w')
    c.expect(TOP)
    c.send(b'q')
    c.expect(rb': ')
    mark('label written')

    # mkfs, tape file 2
    c.sendline(b'ts(0,2)')
    c.expect(rb'file ?system: ')
    c.sendline(b'ra(0,0)')
    for prompt in (rb'file sys size \[\d+\]: ', rb'bytes per inode \[\d+\]: ',
                   rb'interleaving factor \(m; \d+ default\): ',
                   rb'interleaving modulus \(n; \d+ default\): '):
        c.expect(prompt)
        c.sendline(b'')
    c.expect(rb': ')
    mark('root made')

    # restor, tape file 3, the root dump from tape file 5
    c.sendline(b'ts(0,3)')
    c.expect(rb'Tape\? ')
    c.sendline(b'ts(0,5)')
    c.expect(rb'Disk\? ')
    c.sendline(b'ra(0,0)')
    c.expect(rb'Last chance before scribbling on .*\. ')
    c.sendline(b'')
    c.expect(rb'End of tape')
    c.expect(rb': ')
    mark('root restored')

    c.sendline(b'ra(0,0)unix')
    c.expect(rb'erase .* kill .* intr .*\r\n# ', timeout=600)
    mark('GENERIC up single-user')

    def sh(cmd, timeout=900):
        c.sendline(cmd)
        c.expect(rb'\r\n# ', timeout=timeout)

    # The label is block 1; the boot block goes to block 0.
    sh(b'dd if=/mdec/rauboot of=/dev/rra0a bs=512 count=1')
    sh(b'echo "/dev/ra0a\t/\tufs\trw\t1\t1" > /etc/fstab')
    sh(b'echo "/dev/ra0b\tnone\tswap\tsw\t0\t0" >> /etc/fstab')
    sh(b'echo "/dev/ra0g\t/usr\tufs\trw\t1\t2" >> /etc/fstab')
    sh(b'cd /dev; rm -f *mt*; ./MAKEDEV ts0; sync')
    sh(b'newfs ra0g')
    sh(b'mount /dev/ra0g /usr; umask 22; cd /usr')
    sh(b'mt rew; mt -f /dev/rmt12 fsf 6')
    sh(b'tar xpbf 20 /dev/rmt12', timeout=3600)
    mark('/usr unpacked')
    sh(b'mkdir src; cd src; mt -f /dev/rmt12 fsf')
    sh(b'tar xpbf 20 /dev/rmt12', timeout=3600)
    sh(b'mt -f /dev/rmt12 fsf')
    sh(b'tar xpbf 20 /dev/rmt12', timeout=3600)
    mark('sources unpacked')
    sh(b'cd /; chmod 755 / /usr /usr/src /usr/src/sys')
    if files:
        # The files' tape in the drive, while the kernel is stopped.
        c.sendcontrol('e')
        c.expect(rb'sim> ')
        c.sendline(b'detach ts0')
        c.expect(rb'sim> ')
        c.sendline(b'attach -r ts0 ' + files_tape.encode())
        c.expect(rb'sim> ')
        c.sendline(b'continue')
        sh(b'mkdir ' + HOME.encode() + b'; cd ' + HOME.encode() +
           b'; mt rew; tar xpbf 20 /dev/rmt12; ls -l; cd /')
        mark(f'{len(files)} file(s) in {HOME}')
    # The board emulates the console, the line clock and the MSCP disk,
    # so autoconfig is to look for that disk alone.
    sh(b'grep "^ra" /etc/dtab > /etc/dtab.new; mv /etc/dtab.new /etc/dtab')
    sh(b'cat /etc/dtab')
    sh(b'umount /dev/ra0g; sync; sync')
    c.sendline(b'halt')
    c.expect(rb'sim> ', timeout=120)
    mark('installed')

    # From block 0 through rauboot and /boot to the kernel, as the board
    # boots it.
    c.sendline(b'boot rq0')
    c.expect(rb'Boot from ra\(0,0,0\) at 0172150', timeout=120)
    c.expect(rb': ')
    for _ in range(5):  # the simulator's console may drop the first Return
        c.sendline(b'')
        if c.expect([rb'ra0: RD54', pexpect.TIMEOUT], timeout=60) == 0:
            break
    c.expect(rb'\r\n# ', timeout=600)
    mark('booted from the disk')
    c.sendline(b'halt')
    c.expect(rb'HALT instruction', timeout=600)
    c.expect(rb'sim> ', timeout=120)
    c.sendline(b'exit')
    c.expect(pexpect.EOF)
    log.close()
    os.remove(ini)
    if files:
        os.remove(files_tape)
    os.replace(new, image)
    mark(f'{image} written')


if __name__ == '__main__':
    main(sys.argv[1:])
