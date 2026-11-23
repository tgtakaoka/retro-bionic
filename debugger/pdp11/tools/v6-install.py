#!/usr/bin/env python3
"""Install Unix V6 on the DCJ11's RK05 disks, as V6's setup does.

    debugger/pdp11/tools/v6-install.py [--log PATH] [--save PATH] IMAGE [FILE...]

Run on the bench with the board at its prompt and dcj11/unix_v6.hex on
the SD card. IMAGE is samples/dcj11/unix_v6.dsk (`make unix_v6.dsk`,
the distribution tape's three disks); the install starts from it every
time. Through the console it:

  - makes the card's dcj11/unix_v6.dsk the same as IMAGE with `Y`
    (lrzsz's sb and rb on the host), sending only the chunks that differ;
  - loads the bootstrap and boots the tape's rkunix single-user;
  - builds /unix from /usr/sys for the 11/40 (m40.s) with the rk, rp
    and tm drivers, and makes /dev's nodes for them;
  - boots the new /unix and adds the mounts of rk1 on /usr/source and
    rk2 on /usr/doc to /etc/rc;
  - types FILE... into /usr/mandel, reading each back to check it;
  - checks rk0 with icheck and dcheck, syncs and halts;
  - with --save, reads the installed disk back into PATH, checked by
    its CRCs: the copy to keep, and to sync onto another card.

Each step's output is checked before the next; on a surprise it stops
with the console text, and a run after that starts over from IMAGE.
The console goes to stdout, and to PATH with --log.
"""
import argparse
import os
import select
import sys
import time
from importlib.machinery import SourceFileLoader

HERE = os.path.dirname(os.path.abspath(__file__))
bc = SourceFileLoader(
    'bc', os.path.join(HERE, '../../../scripts/bionic-control.py')).load_module()

# The disk L dcj11/unix_v6.hex mounts.
CARD = 'dcj11/unix_v6.dsk'
# V6's console drops characters typed faster than this.
KEY_GAP = 0.05
SAMPLES = '/usr/mandel'
CONF = ['rk', 'rp', 'tm']
NODES = ['rk0 b 0 0', 'rk1 b 0 1', 'rk2 b 0 2', 'rp0 b 1 0', 'mt0 b 3 0',
         'rrk0 c 9 0', 'rrk1 c 9 1', 'rrk2 c 9 2', 'rrp0 c 11 0', 'rmt0 c 12 0']
RC = ['/etc/mount /dev/rk1 /usr/source', '/etc/mount /dev/rk2 /usr/doc']


class Console:
    def __init__(self, board, log):
        self.board, self.fd, self.log = board, board.fd, log
        self.seen = ''

    def read(self, secs):
        """What V6 sends in |secs|, without its parity bit, CRs or fill."""
        r, _, _ = select.select([self.fd], [], [], secs)
        if not r:
            return ''
        try:
            d = os.read(self.fd, 4096)
        except BlockingIOError:
            return ''
        # Without the parity bit, the fill after each newline is DEL.
        t = bytes(c & 0x7f for c in d if c & 0x7f not in (0, 0x7f, 0x0d))
        t = t.decode('latin-1')
        sys.stdout.write(t)
        sys.stdout.flush()
        if self.log:
            self.log.write(t)
            self.log.flush()
        self.seen += t
        return t

    def until(self, ends, timeout):
        """What came since the last typing, once it ends in one of |ends|."""
        end = time.time() + timeout
        while time.time() < end:
            self.read(0.2)
            if any(self.seen.endswith(e) for e in ends):
                return self.seen
        fail('timed out waiting for %r after %r' % (ends, self.seen[-40:]))

    def type(self, text):
        self.seen = ''
        for ch in text:
            os.write(self.fd, ch.encode('latin-1'))
            time.sleep(KEY_GAP)
            self.read(0)

    def shell(self, line, timeout=120, expect=None, quiet=True):
        """Run |line| at the # prompt; its output, checked."""
        self.type(line + '\n')
        out = self.until(['\n# '], timeout)
        body = out[out.find('\n') + 1:-len('# ')]
        # The debugger reports the CPU's own aborts, as a stack grows.
        body = ''.join(l for l in body.splitlines(True)
                       if not l.startswith('?#ABORT'))
        if expect is not None and expect not in body:
            fail('%s: no %r' % (line, expect))
        if quiet and expect is None and body.strip():
            fail('%s: unexpected output' % line)
        if 'panic' in out:
            fail('panic')
        return body


# The console, once V6 runs: a stop syncs first, so the disk stays
# consistent for a look at what went wrong.
console = None


def fail(why):
    global console
    con, console = console, None
    if con:
        con.type('sync\n')
        end = time.time() + 5
        while time.time() < end and not con.seen.endswith('\n# '):
            con.read(0.2)
    sys.exit('\nv6-install: ' + why)


def sync_card(board, image):
    """The card's disk made the same as |image|, chunk by chunk."""
    print('v6-install: sync %s to %s' % (image, CARD))
    sent = board.sd_put(image, CARD, sync=True)
    print('v6-install: %d bytes sent' % sent)


def save_card(board, path):
    """The card's disk read back into |path|, checked by its CRCs."""
    print('v6-install: save %s to %s' % (CARD, path))
    board.sd_get(CARD, path)
    size, crcs = board.sd_crcs(CARD)
    with open(path, 'rb') as f:
        data = f.read()
    if len(data) != size or bc.chunk_crcs(data) != crcs:
        raise RuntimeError('%s differs from the card' % path)


def boot(board, con, kernel):
    """The bootstrap from the SD card, which mounts the disks, then
    |kernel| single-user to its # prompt."""
    board.send(b'R')
    reply = board.send(b'L', idle=0.3) + board.send(b'dcj11/unix_v6.hex\r', wait=5, idle=1)
    if b'not found' in reply:
        fail('dcj11/unix_v6.hex not on the SD card')
    board.send(b'R')
    # A reset keeps SP, which after power-up may point into the I/O
    # page, where the go's PC and PS can't be stacked.
    board.send(b'=')
    board.send(b'SP 1000\r')
    board.send(b'G', idle=0.2)
    con.type('\r')
    con.until(['@'], 10)
    con.type(kernel + '\r')
    out = con.until(['\n# '], 60)
    if 'mem =' not in out:
        fail('%s did not boot' % kernel)
    global console
    console = con
    con.shell('stty -lcase')


def halt(board, con):
    con.shell('sync')
    con.shell('sync')
    con.shell('sync')
    time.sleep(1)
    global console
    console = None
    board.abort()
    board.send(wait=4.0, idle=1.0)


def build_kernel(con):
    con.shell('chdir /usr/sys/conf')
    con.shell('cc mkconf.c', 300)
    con.shell('mv a.out mkconf')
    # mkconf reads device names up to "done".
    con.type('./mkconf\n')
    time.sleep(0.5)
    con.type(''.join(name + '\n' for name in CONF + ['done']))
    if 'cannot find' in con.until(['\n# '], 60):
        fail('mkconf: unknown device')
    con.shell('as m40.s', 300)
    con.shell('mv a.out m40.o')
    con.shell('cc -c c.c', 300)
    con.shell('as l.s', 300)
    con.shell('ld -x a.out m40.o c.o ../lib1 ../lib2', 300)
    con.shell('ls a.out', expect='a.out')
    con.shell('rm /unix')
    con.shell('mv a.out /unix')
    for node in NODES:
        con.shell('/etc/mknod /dev/' + node)


def put_file(con, path):
    name = os.path.basename(path)
    with open(path) as f:
        text = f.read()
    if '#' in text or '@' in text:
        fail('%s: # and @ are the console\'s erase and kill' % path)
    con.type('cat >%s/%s\n' % (SAMPLES, name))
    time.sleep(0.5)
    # The prompt after ^D follows the text's last newline.
    con.type(text + '\x04')
    con.until(['\n# '], 60)
    back = con.shell('cat %s/%s' % (SAMPLES, name), quiet=False)
    # The console may print tabs as spaces.
    if back not in (text, text.expandtabs()):
        fail('%s: reads back different' % name)


def main():
    p = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--log')
    p.add_argument('--save')
    p.add_argument('image')
    p.add_argument('files', nargs='*')
    a = p.parse_args()
    log = open(a.log, 'w') if a.log else None
    board = bc.Board.open()
    con = Console(board, log)

    try:
        sync_card(board, a.image)
        boot(board, con, 'rkunix')
        build_kernel(con)
        halt(board, con)

        boot(board, con, 'unix')
        con.shell('mkdir /usr/doc')
        con.type('cat >>/etc/rc\n')
        time.sleep(0.5)
        con.type(''.join(line + '\n' for line in RC) + '\x04')
        con.until(['\n# '], 30)
        con.shell('cat /etc/rc', expect=RC[-1], quiet=False)
        if a.files:
            con.shell('mkdir ' + SAMPLES)
            for path in a.files:
                put_file(con, path)
        con.shell('sync')
        con.shell('icheck /dev/rrk0', expect='free', quiet=False)
        con.shell('dcheck /dev/rrk0', quiet=False)
        halt(board, con)
        if a.save:
            save_card(board, a.save)
    except RuntimeError as e:
        fail(str(e))
    board.close()
    print('\nv6-install: done')


if __name__ == '__main__':
    main()
