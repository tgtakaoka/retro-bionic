#!/usr/bin/env python3
"""Install Unix V6 from its distribution tape's disks onto an RK11 image.

    debugger/pdp11/tools/v6-install.py PDP11 DISKS IMAGE [FILE...]

Runs open-simh's PDP11 (simh-fetch.py's), an 11/40 with 256 KB and an
RK11 holding the tape's three RK05 disks from DISKS (v6-fetch.py's
rk0.dsk-rk2.dsk), and types V6's setup at its console:

  - boots the tape's rkunix single-user;
  - builds /unix from /usr/sys for the 11/40 (m40.s) with the rk, rp
    and tm drivers, and makes /dev's nodes for them;
  - boots the new /unix and adds the mounts of rk1 on /usr/source and
    rk2 on /usr/doc to /etc/rc;
  - types each FILE into /usr/mandel, reading it back to check it;
  - checks rk0 with icheck and dcheck;
  - boots the disk again from its block 0, as the board will.

Each step's output is checked before the next. IMAGE gets the drives in
the slots the board's RK11 reads, 1024 blocks for a drum and then 6144
a drive; IMAGE.log is the console's transcript.
"""
import os
import re
import sys
import time

import pexpect

BLOCK = 512
RK05_BLOCKS = 4872
DRUM_BLOCKS = 1024
SLOT_BLOCKS = 6144
DRIVES = 3
SAMPLES = '/usr/mandel'
CONF = ['rk', 'rp', 'tm']
NODES = ['rk0 b 0 0', 'rk1 b 0 1', 'rk2 b 0 2', 'rp0 b 1 0', 'mt0 b 3 0',
         'rrk0 c 9 0', 'rrk1 c 9 1', 'rrk2 c 9 2', 'rrp0 c 11 0', 'rmt0 c 12 0']
RC = ['/etc/mount /dev/rk1 /usr/source', '/etc/mount /dev/rk2 /usr/doc']
# After a newline the console sends a CR and DELs as fill.
PROMPT = rb'\n[\r\x7f]*# '


def fail(why):
    sys.exit('v6-install: ' + why)


class V6:
    def __init__(self, pdp11, ini, log):
        self.c = pexpect.spawn(pdp11, [ini], timeout=60, logfile=log, echo=False)
        self.c.expect(rb'sim> ')

    def boot(self, kernel):
        """Drive 0's block 0, then |kernel| single-user to its # prompt."""
        c = self.c
        c.sendline(b'boot rk0')
        c.expect(rb'@')
        c.sendline(kernel.encode())
        c.expect(rb'mem = ')
        c.expect(PROMPT)
        self.sh('stty -lcase')

    def sh(self, line, timeout=120, expect=None, quiet=True):
        """Run |line| at the # prompt; its output, checked."""
        self.c.sendline(line.encode())
        self.c.expect(PROMPT, timeout=timeout)
        out = self.c.before.decode('latin-1').replace('\r', '').replace('\x7f', '')
        # The first line is the command's echo.
        body = out.partition('\n')[2]
        if body:
            body += '\n'
        if 'panic' in out:
            fail('%s: panic' % line)
        if expect is not None and expect not in body:
            fail('%s: no %r in %r' % (line, expect, body))
        if quiet and expect is None and body.strip():
            fail('%s: unexpected output %r' % (line, body))
        return body

    def type(self, text):
        """|text| into a program reading the console, then ^D."""
        self.c.send(text.encode('latin-1') + b'\x04')
        self.c.expect(PROMPT)

    def halt(self):
        for _ in range(3):
            self.sh('sync')
        time.sleep(1)
        self.c.sendcontrol('e')
        self.c.expect(rb'sim> ')

    def exit(self):
        self.c.sendline(b'exit')
        self.c.expect(pexpect.EOF)


def build_kernel(v6):
    v6.sh('chdir /usr/sys/conf')
    v6.sh('cc mkconf.c', 300)
    v6.sh('mv a.out mkconf')
    # mkconf reads device names up to "done".
    v6.c.sendline(b'./mkconf')
    for name in CONF + ['done']:
        v6.c.sendline(name.encode())
    v6.c.expect(PROMPT)
    if b'cannot find' in v6.c.before:
        fail('mkconf: unknown device')
    v6.sh('as m40.s', 300)
    v6.sh('mv a.out m40.o')
    v6.sh('cc -c c.c', 300)
    v6.sh('as l.s', 300)
    v6.sh('ld -x a.out m40.o c.o ../lib1 ../lib2', 300)
    v6.sh('ls a.out', expect='a.out')
    v6.sh('rm /unix')
    v6.sh('mv a.out /unix')
    for node in NODES:
        v6.sh('/etc/mknod /dev/' + node)


def put_file(v6, path):
    name = os.path.basename(path)
    with open(path) as f:
        text = f.read()
    if '#' in text or '@' in text:
        fail('%s: # and @ are the console\'s erase and kill' % path)
    v6.c.sendline(('cat >%s/%s' % (SAMPLES, name)).encode())
    time.sleep(0.5)
    v6.type(text)
    back = v6.sh('cat %s/%s' % (SAMPLES, name), quiet=False)
    # The console may print tabs as spaces.
    if back not in (text, text.expandtabs()):
        fail('%s: reads back different' % name)


def main(argv):
    if len(argv) < 3:
        sys.exit(__doc__)
    pdp11, disks, image = argv[:3]
    files = argv[3:]
    pdp11 = os.path.abspath(pdp11)
    drives = [os.path.abspath('%s.rk%d' % (image, d)) for d in range(DRIVES)]
    for d, path in enumerate(drives):
        with open(os.path.join(disks, 'rk%d.dsk' % d), 'rb') as f:
            data = f.read()
        with open(path, 'wb') as f:
            f.write(data.ljust(RK05_BLOCKS * BLOCK, b'\0'))
    ini = os.path.abspath(image + '.ini')
    with open(ini, 'w') as f:
        f.write('set cpu 11/40\nset cpu 256k\n')
        # V6 sends a parity bit; 173030 in the switches boots single-user.
        f.write('set tto 7b\ndeposit sr 173030\n')
        for dev in ('rl', 'rp', 'rx', 'tm', 'hk', 'rq', 'tq', 'ts', 'xq', 'dz'):
            f.write('set %s disable\n' % dev)
        f.write('set rk enable\n')
        for d, path in enumerate(drives):
            f.write('attach rk%d %s\n' % (d, path))
    log = open(image + '.log', 'wb')
    t0 = time.time()

    def mark(what):
        print(f'{time.time() - t0:6.0f} s  {what}', flush=True)

    v6 = V6(pdp11, ini, log)
    v6.boot('rkunix')
    mark('rkunix up single-user')
    build_kernel(v6)
    mark('/unix built')
    v6.halt()

    v6.boot('unix')
    mark('/unix up single-user')
    v6.sh('mkdir /usr/doc')
    v6.c.sendline(b'cat >>/etc/rc')
    time.sleep(0.5)
    v6.type(''.join(line + '\n' for line in RC))
    v6.sh('cat /etc/rc', expect=RC[-1], quiet=False)
    if files:
        v6.sh('mkdir ' + SAMPLES)
        for path in files:
            put_file(v6, path)
        mark(f'{len(files)} file(s) in {SAMPLES}')
    v6.sh('sync')
    v6.sh('icheck /dev/rrk0', expect='free', quiet=False)
    v6.sh('dcheck /dev/rrk0', quiet=False)
    v6.halt()
    mark('installed')

    # From block 0 to the new kernel, as the board boots it.
    v6.boot('unix')
    v6.sh('ls ' + SAMPLES, quiet=False)
    v6.halt()
    v6.exit()
    log.close()
    mark('booted from the disk')

    new = image + '.new'
    with open(new, 'wb') as out:
        out.write(bytes(DRUM_BLOCKS * BLOCK))
        for path in drives:
            with open(path, 'rb') as f:
                out.write(f.read().ljust(SLOT_BLOCKS * BLOCK, b'\0'))
    os.replace(new, image)
    for path in drives + [ini]:
        os.remove(path)
    mark(f'{image} written')


if __name__ == '__main__':
    main(sys.argv[1:])
