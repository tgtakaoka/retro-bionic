#!/usr/bin/env python3
"""The benches on this machine: names, state, and who may use each.

    bionic-bench.py status            every bench: power, target, holder,
                                      queue
    bionic-bench.py lock NAME         take NAME now, or fail naming its
                                      holder
    bionic-bench.py reserve NAME      queue for NAME, or take it if free;
                                      returns once it is yours
    bionic-bench.py unlock NAME [--force]
                                      give NAME to the next in its queue
    bionic-bench.py env NAME          shell exports that point every bench
                                      tool at NAME
    bionic-bench.py wait NAME         return after the next chip is seated
    bionic-bench.py watch [NAME...] [--mine] [--all] [--deadline SECONDS]
                                      follow state changes: every bench, the
                                      ones named, or (--mine) the ones the
                                      caller holds; --all adds the commands
                                      sent; one line per event, for a
                                      session's Monitor
    bionic-bench.py peek NAME         follow what NAME prints to the
                                      session driving it, without touching
                                      the bench
    bionic-bench.py rename NAME NEW   a clearer name for people
    bionic-bench.py add NAME [KEY=VALUE...]
                                      something else to lock, like a logic
                                      analyzer: kind=logic mcp=URL
    bionic-bench.py set NAME KEY=VALUE...
                                      change a resource's attributes
    bionic-bench.py daemon            the watcher; one per machine

A Teensy seen for the first time is named bench1, bench2, ... in the order
detected, keyed by its serial number in REGISTRY, so a bench keeps its name
until a session renames it.

The holder of a bench owns it: flash, commands, asking for a power cycle,
captures. bionic-control.py, and so every tool built on it, refuses a bench
another session holds; one nobody holds stays open to all. A holder is a
Claude session (or a shell) named by BIONIC_OWNER, else its tmux window;
when its process is gone the lock passes to the next in the queue.

The daemon follows each Teensy on USB: power off drops it, power on brings
it back, a flash passes through the HalfKay bootloader. After any change it
asks the board what is seated, and logs any CrashReport the Teensy prints
on that first look. Its events, its own start and stop, and every lock
change are lines in bench-events.log, which `watch` follows for a session.

Everything else shared lives in RUN_DIR as plain files, so the state
survives the daemon and any session ending:

    benches.json      the daemon's view: power, target, firmware, since
    NAME.hold         holder and queue, changed only under NAME.mutex
    bench-events.log  events

and next to REGISTRY, since a Teensy keeps its image through a reboot,
firmware-SERIAL.json: the hash of the image `bionic-control.py flash` last
programmed and the branch (or commit) it was built from. The hash is the
fingerprint: builds of the same sources in any worktree hash alike. A
flash made any other way leaves the firmware unknown.
"""
import fcntl
import glob
import hashlib
import json
import os
import signal
import socket
import subprocess
import sys
import threading
import time

HERE = os.path.dirname(os.path.abspath(__file__))
# The names outlive a reboot, which empties RUN_DIR.
REGISTRY = os.environ.get('BIONIC_REGISTRY', os.path.expanduser(
        '~/.local/state/bionic-bench/benches.json'))
RUN_DIR = os.environ.get('BIONIC_RUN_DIR', '/tmp/bionic-bench')
STATE = os.path.join(RUN_DIR, 'benches.json')
LOG = os.path.join(RUN_DIR, 'bench-events.log')
TEENSY = '/dev/serial/by-id/usb-Teensyduino_Dual_Serial_%s-if0%d'


# ------------------------------------------------------------- registry
class Registry:
    """REGISTRY, read and changed under its mutex: {"benches": {serial:
    name}, "resources": {name: {key: value}}}. A Teensy seen for the first
    time becomes
    bench<N>, numbered in the order detected; its name stays until renamed."""

    def __enter__(self):
        os.makedirs(os.path.dirname(REGISTRY), exist_ok=True)
        self.fd = os.open(REGISTRY + '.mutex', os.O_CREAT | os.O_RDWR, 0o666)
        fcntl.flock(self.fd, fcntl.LOCK_EX)
        try:
            self.data = json.load(open(REGISTRY))
        except (OSError, ValueError):
            self.data = {'benches': {}, 'resources': {}}
        if isinstance(self.data['resources'], list):
            self.data['resources'] = {n: {} for n in self.data['resources']}
        return self

    def __exit__(self, *exc):
        tmp = REGISTRY + '.tmp'
        with open(tmp, 'w') as f:
            json.dump(self.data, f, indent=1)
        os.replace(tmp, REGISTRY)
        fcntl.flock(self.fd, fcntl.LOCK_UN)
        os.close(self.fd)
        return False

    def name(self, serial):
        benches = self.data['benches']
        if serial not in benches:
            taken = set(benches.values()) | set(self.data['resources'])
            n = len(benches) + 1
            while 'bench%d' % n in taken:
                n += 1
            benches[serial] = 'bench%d' % n
        return benches[serial]


def names():
    """Every bench, in the order first detected, then every resource."""
    with Registry() as r:
        return list(r.data['benches'].values()) + list(r.data['resources'])


def resources(**match):
    """{name: attributes} of the resources whose attributes include `match`,
    e.g. kind='logic' for the logic analyzers."""
    with Registry() as r:
        return {n: a for n, a in r.data['resources'].items()
                if all(a.get(k) == v for k, v in match.items())}


def pick(candidates, env):
    """The one of `candidates` to use: named by environment variable `env`,
    else the one the caller holds, else the only one; None if ambiguous."""
    if os.environ.get(env):
        return os.environ[env]
    who = me()
    found = []
    for name in candidates:
        with Hold(name) as h:
            if h.mine(who):
                found.append(name)
    if len(found) == 1:
        return found[0]
    return candidates[0] if len(candidates) == 1 and not found else None


def serial_of(name):
    with Registry() as r:
        for serial, n in r.data['benches'].items():
            if n == name:
                return serial
    return None


def ports(name):
    """(console, halt port) of bench `name`, or None if it is not a bench."""
    serial = serial_of(name)
    if serial is None:
        return None
    return TEENSY % (serial, 0), TEENSY % (serial, 2)


def present():
    """{name: console port} of every bench on USB now, naming any Teensy
    seen for the first time."""
    found = {}
    paths = sorted(glob.glob(TEENSY % ('*', 0)))
    if not paths:
        return found
    with Registry() as r:
        for path in paths:
            serial = path.rsplit('_', 1)[1][:-len('-if00')]
            found[r.name(serial)] = path
    return found


def serial_at(port):
    """The serial number of the Teensy whose USB serial `port` is."""
    real = os.path.realpath(port)
    for path in glob.glob(TEENSY % ('*', 0)) + glob.glob(TEENSY % ('*', 2)):
        if os.path.realpath(path) == real:
            return path.rsplit('_', 1)[1][:-len('-if00')]
    return None


def firmware_path(serial):
    return os.path.join(os.path.dirname(REGISTRY), 'firmware-%s.json' % serial)


def record_firmware(serial, image, proj):
    """Note what was just programmed into Teensy `serial`: the image's hash
    and the commit it was built from."""
    if serial is None:
        return
    with open(image, 'rb') as f:
        digest = hashlib.sha256(f.read()).hexdigest()[:12]
    def git(*args):
        return subprocess.run(['git'] + list(args), cwd=proj,
                              capture_output=True, text=True).stdout.strip()
    commit = git('describe', '--always', '--dirty')
    branch = git('rev-parse', '--abbrev-ref', 'HEAD')
    if branch and branch != 'HEAD' and commit.endswith('-dirty'):
        branch += '-dirty'
    with open(firmware_path(serial), 'w') as f:
        json.dump({'image': digest, 'commit': commit,
                   'branch': branch if branch != 'HEAD' else None,
                   'when': time.time()}, f)


def firmware(serial, since=0.0):
    """`branch (image hash)` last programmed into `serial`, the commit when
    it was built off a branch; None when unknown or older than `since`:
    flashed some other way."""
    try:
        fw = json.load(open(firmware_path(serial)))
    except (OSError, ValueError):
        return None
    if fw['when'] < since:
        return None
    return '%s (%s)' % (fw.get('branch') or fw['commit'], fw['image'])


def peek_address(name):
    """The abstract socket a process driving bench `name` serves its
    console output on."""
    return '\0bionic-peek-%s' % name


def halfkay():
    """True while a Teensy sits in its HalfKay bootloader, being flashed."""
    for dev in glob.glob('/sys/bus/usb/devices/*/idProduct'):
        try:
            if open(dev).read().strip() == '0478' and open(
                    os.path.join(os.path.dirname(dev), 'idVendor')
                    ).read().strip() == '16c0':
                return True
        except OSError:
            pass
    return False


# ---------------------------------------------------------------- locks
def me():
    """(owner, pid) of the caller: the pid is its Claude session's, or the
    shell's, whose end frees the lock."""
    pid = int(os.environ.get('CLAUDE_PID') or os.getppid())
    owner = os.environ.get('BIONIC_OWNER')
    if not owner and os.environ.get('TMUX_PANE'):
        owner = subprocess.run(
                ['tmux', 'display', '-p', '-t', os.environ['TMUX_PANE'],
                 '#{window_name}'], capture_output=True, text=True
                ).stdout.strip()
    return owner or 'pid%d' % pid, pid


def alive(entry):
    try:
        os.kill(entry['pid'], 0)
        return True
    except (OSError, KeyError, TypeError):
        return False


def say(name, what):
    os.makedirs(RUN_DIR, exist_ok=True)
    with open(LOG, 'a') as f:
        f.write('%s %s %s\n' % (time.strftime('%H:%M:%S'), name, what))


# The first word of an event that changes a bench's state; the rest of the
# log is the commands sessions send.
STATE_EVENTS = {'power', 'gone', 're-enumerated', 'flashed', 'flash',
                'seated', 'no', 'crash', 'locked', 'unlocked', 'queued',
                'renamed', 'state', 'recovered', 'started', 'stopped'}


def follow():
    """Yield (time, name, event) for each line logged from now on; a log that
    shrinks, as after a reboot empties RUN_DIR, is read again from its
    start. Yields None every half second it is quiet."""
    os.makedirs(RUN_DIR, exist_ok=True)
    open(LOG, 'a').close()
    f = open(LOG)
    f.seek(0, 2)
    while True:
        line = f.readline()
        if line:
            parts = line.rstrip('\n').split(None, 2)
            if len(parts) == 3:
                yield tuple(parts)
            continue
        try:
            if os.path.getsize(LOG) < f.tell():
                f.close()
                f = open(LOG)
        except OSError:
            pass
        yield None
        time.sleep(0.5)


class Hold:
    """NAME.hold, read and changed under NAME.mutex:
    {"holder": {owner, pid, since} or null, "queue": [...]}."""

    def __init__(self, name):
        self.name = name
        self.path = os.path.join(RUN_DIR, name + '.hold')

    def __enter__(self):
        os.makedirs(RUN_DIR, exist_ok=True)
        self.fd = os.open(os.path.join(RUN_DIR, self.name + '.mutex'),
                          os.O_CREAT | os.O_RDWR, 0o666)
        fcntl.flock(self.fd, fcntl.LOCK_EX)
        try:
            self.data = json.load(open(self.path))
        except (OSError, ValueError):
            self.data = {'holder': None, 'queue': []}
        self.settle()
        return self

    def __exit__(self, *exc):
        tmp = self.path + '.tmp'
        with open(tmp, 'w') as f:
            json.dump(self.data, f)
        os.replace(tmp, self.path)
        fcntl.flock(self.fd, fcntl.LOCK_UN)
        os.close(self.fd)
        return False

    def settle(self):
        """Drop a holder or a waiter whose session is gone, and pass the
        lock on to the head of the queue."""
        d = self.data
        if d['holder'] and not alive(d['holder']):
            say(self.name, 'unlocked: %s is gone' % d['holder']['owner'])
            d['holder'] = None
        d['queue'] = [w for w in d['queue'] if alive(w)]
        if not d['holder'] and d['queue']:
            d['holder'] = dict(d['queue'].pop(0), since=time.time())
            say(self.name, 'locked by %s, from the queue'
                % d['holder']['owner'])

    def mine(self, who):
        h = self.data['holder']
        return h is not None and h['pid'] == who[1]


def holder(name):
    """The live holder of `name`, or None."""
    with Hold(name) as h:
        return h.data['holder']


def held():
    """The benches the caller holds."""
    who, found = me(), []
    for name in names():
        if ports(name):
            with Hold(name) as h:
                if h.mine(who):
                    found.append(name)
    return found


def mine():
    """The bench the caller holds, or None; a session holding several
    names one per command with BIONIC_BENCH."""
    found = held()
    return found[0] if len(found) == 1 else None


def check(name):
    """Exit unless the caller may use bench `name`: nobody holds it, or the
    caller does."""
    who = me()
    with Hold(name) as h:
        if h.data['holder'] is None or h.mine(who):
            return
        sys.exit('%s is locked by %s; `scripts/bionic-bench.py reserve %s` '
                 'queues for it' % (name, h.data['holder']['owner'], name))


# --------------------------------------------------------------- daemon
def teensys():
    """{serial: console port} of every Teensy on USB now."""
    return {path.rsplit('_', 1)[1][:-len('-if00')]: path
            for path in glob.glob(TEENSY % ('*', 0))}


def daemon():
    """Log each bench's power, flashes and the target seated; keep
    benches.json and pass on locks whose holder is gone. Benches are
    followed by serial number, so a rename takes effect at once."""
    from importlib.machinery import SourceFileLoader
    bc = SourceFileLoader('bc', os.path.join(HERE, 'bionic-control.py')
                          ).load_module()
    bc.LOG_AS = None   # its looks at who is seated are not commands
    state, guard = {}, threading.Lock()

    def name(serial):
        with Registry() as r:
            return r.name(serial)

    def note(serial, **kw):
        with guard:
            state.setdefault(serial, {'firmware': None}).update(kw)
            seen = {name(s): v for s, v in state.items()}
            tmp = STATE + '.tmp'
            with open(tmp, 'w') as f:
                json.dump(seen, f, indent=1)
            os.replace(tmp, STATE)

    probing = threading.Lock()   # bionic-control's flock is per process

    def answer(serial, port, budget=120.0):
        """Poll until the board names its target: it boots for a moment
        after enumerating, and a session may be using it."""
        how, until = 'gone', time.time() + budget
        note(serial, firmware=firmware(serial, flashed_at.get(serial, 0.0)))
        while time.time() < until:
            time.sleep(2.0)
            if not os.path.exists(port):
                return
            with probing:
                try:
                    board = bc.Board(bc._open(port=port, name=name(serial),
                                              query=True))
                except SystemExit:   # a session is using the board
                    how = 'in use'
                    continue
                try:
                    time.sleep(1.0)
                    # the first open after a crash gets its CrashReport
                    first = board._drain(1.5, 0.4)
                    if b'CrashReport' in first:
                        report(serial, first)
                    how = board.state()
                    who = board.identify() if how == 'prompt' else None
                finally:
                    board.close()
                    bc.release()
            if who:
                note(serial, target=who)
                say(name(serial), 'seated %s, firmware %s' % (
                        who, state[serial]['firmware'] or 'unknown'))
                return
        note(serial, target=None)
        say(name(serial), 'no target: %s' % how)

    def report(serial, text):
        """Keep a CrashReport and log where it is, with its first fact."""
        text = text.decode('ascii', 'replace').replace('\r', '')
        path = os.path.join(RUN_DIR, 'crash-%s-%s.txt' % (
                name(serial), time.strftime('%Y%m%d-%H%M%S')))
        with open(path, 'w') as f:
            f.write(text)
        lines = [l.strip() for l in text.split('CrashReport', 1)[1]
                 .splitlines() if l.strip()]
        fact = next((l for l in lines if 'address' in l),
                    lines[0] if lines else '')
        say(name(serial), 'crash report: %s (%s)' % (fact, path))

    def probe(serial, port):
        threading.Thread(target=answer, args=(serial, port),
                         daemon=True).start()

    fresh = not os.path.exists(LOG)
    say('daemon', 'started pid %d%s' % (
            os.getpid(), ', run dir was empty' if fresh else ''))

    def stopped(*_):
        say('daemon', 'stopped pid %d' % os.getpid())
        os._exit(0)

    signal.signal(signal.SIGTERM, stopped)
    signal.signal(signal.SIGINT, stopped)

    on, gone, flashed, flashed_at = teensys(), {}, set(), {}
    with Registry() as r:
        known = list(r.data['benches'])
    for serial in known:
        if serial not in on:
            note(serial, power='off', since=time.time(), target=None)
    for serial, port in sorted(on.items(), key=lambda i: name(i[0])):
        note(serial, power='on', since=time.time())
        say(name(serial), 'power on')
        probe(serial, port)
    tick = 0.0
    while True:
        now = teensys()
        if gone and halfkay():
            flashed.update(gone)
        for serial in sorted(set(on) - set(now)):
            gone[serial] = time.time()
            note(serial, power='off', since=time.time())
            say(name(serial), 'gone from USB')
        for serial in sorted(set(now) - set(on)):
            left = gone.pop(serial, None)
            if serial in flashed:
                what = 'flashed'
                # a record older than this flash is of the image before it
                flashed_at[serial] = left - 5.0
                note(serial, flashed=flashed_at[serial])
            elif left is not None and time.time() - left < 3.0:
                what = 're-enumerated'   # a firmware crash
            else:
                what = 'power on'
            flashed.discard(serial)
            note(serial, power='on', since=time.time())
            say(name(serial), what if left is None else
                '%s after %.0fs' % (what, time.time() - left))
            probe(serial, now[serial])
        on = now
        if time.time() > tick:   # pass on locks of sessions that ended
            tick = time.time() + 5.0
            for n in names():
                with Hold(n):
                    pass
        time.sleep(0.2)


# ------------------------------------------------------------------ cli
def taken(name):
    print('%s is yours' % name)
    print('follow its changes: scripts/bionic-bench.py watch --mine '
          '(one line per event, e.g. under a Monitor)', file=sys.stderr)


def watch(args):
    """Print each state event of the chosen benches as it is logged, and
    the daemon's own start and stop, until the deadline if one is given."""
    every = '--all' in args
    only_mine = '--mine' in args
    deadline = None
    if '--deadline' in args:
        i = args.index('--deadline')
        deadline = time.time() + float(args[i + 1])
        del args[i:i + 2]
    wanted = [n for n in args if not n.startswith('--')]
    for n in wanted:
        if n not in names():
            sys.exit('no bench %s; `scripts/bionic-bench.py status` lists '
                     'them' % n)
    # Ownership follows the log's own lock lines, not NAME.hold read when a
    # line is seen, which may already be a later state.
    who = me()
    ours = set(held()) if only_mine else set()
    for ev in follow():
        if deadline is not None and time.time() > deadline:
            print('%s watch deadline' % time.strftime('%H:%M:%S'),
                  flush=True)
            return
        if ev is None:
            continue
        stamp, bench, event = ev
        if bench != 'daemon':
            if not every and event.split(None, 1)[0] not in STATE_EVENTS:
                continue
            if wanted and bench not in wanted:
                continue
            if only_mine:
                if event.startswith('locked by '):
                    owner = event[len('locked by '):].split(',', 1)[0]
                    if owner == who[0]:
                        ours.add(bench)
                    else:
                        ours.discard(bench)
                if bench not in ours:
                    continue
                if event.startswith('unlocked'):
                    ours.discard(bench)
        print('%s %s %s' % (stamp, bench, event), flush=True)


def status():
    try:
        seen = json.load(open(STATE))
    except (OSError, ValueError):
        seen = {}
    for name in names():
        s = seen.get(name, {})
        with Hold(name) as h:
            d = h.data
        holds = d['holder']['owner'] if d['holder'] else '-'
        queue = ' '.join(w['owner'] for w in d['queue']) or '-'
        if ports(name) is None:
            attrs = ' '.join('%s=%s' % kv for kv in
                             resources().get(name, {}).items())
            print('%-8s %-66s held by %s, queue %s'
                  % (name, attrs, holds, queue))
            continue
        # read now, not from the daemon: a record may be newer than its view
        fw = firmware(serial_of(name), s.get('flashed', 0.0))
        print('%-8s power %-3s target %-12s firmware %-24s '
              'held by %s, queue %s'
              % (name, s.get('power', '?'), s.get('target') or '-',
                 fw or 'unknown', holds, queue))


def main():
    a = sys.argv[1:]
    if not a or a[0] == 'status':
        status()
        return
    cmd, name = a[0], a[1] if len(a) > 1 else None
    if cmd == 'daemon':
        daemon()
        return
    if cmd == 'watch':
        watch(a[1:])
        return
    if name is None:
        sys.exit(__doc__)
    if cmd == 'add':
        with Registry() as r:
            if name in r.data['benches'].values() or \
                    name in r.data['resources']:
                sys.exit('%s exists' % name)
            r.data['resources'][name] = dict(
                    kv.split('=', 1) for kv in a[2:] if '=' in kv)
        return
    if name not in names():
        sys.exit('no bench %s; `scripts/bionic-bench.py status` lists them'
                 % name)
    if cmd == 'set':
        with Registry() as r:
            if name not in r.data['resources']:
                sys.exit('%s is a bench; only resources take attributes'
                         % name)
            r.data['resources'][name].update(
                    kv.split('=', 1) for kv in a[2:] if '=' in kv)
        return
    who = me()
    entry = {'owner': who[0], 'pid': who[1], 'since': time.time()}
    if cmd == 'lock':
        with Hold(name) as h:
            if h.mine(who):
                print('%s is already yours' % name)
            elif h.data['holder'] or h.data['queue']:
                sys.exit('%s is locked by %s%s' % (
                        name, (h.data['holder'] or {}).get('owner', 'nobody'),
                        ', queue %d' % len(h.data['queue'])
                        if h.data['queue'] else ''))
            else:
                h.data['holder'] = entry
                say(name, 'locked by %s' % who[0])
                taken(name)
    elif cmd == 'reserve':
        with Hold(name) as h:
            if h.mine(who):
                print('%s is already yours' % name)
                return
            if not h.data['holder'] and not h.data['queue']:
                h.data['holder'] = entry
                say(name, 'locked by %s' % who[0])
                taken(name)
                return
            if all(w['pid'] != who[1] for w in h.data['queue']):
                h.data['queue'].append(entry)
                say(name, 'queued %s' % who[0])
            print('%s: queued behind %s' % (name, h.data['holder']['owner']))
        while True:
            time.sleep(2.0)
            with Hold(name) as h:
                if h.mine(who):
                    taken(name)
                    return
    elif cmd == 'unlock':
        force = '--force' in a
        with Hold(name) as h:
            if not h.data['holder']:
                print('%s is not locked' % name)
            elif h.mine(who) or force:
                say(name, 'unlocked by %s' % who[0])
                h.data['holder'] = None
                h.settle()
            else:
                sys.exit('%s is locked by %s; --force takes it away'
                         % (name, h.data['holder']['owner']))
    elif cmd == 'rename':
        new = a[2] if len(a) > 2 else sys.exit(__doc__)
        if new in names():
            sys.exit('%s exists' % new)
        with Hold(name) as h:
            if h.data['holder'] and not h.mine(who) or h.data['queue']:
                sys.exit('%s is in use; its holder renames it' % name)
            with Registry() as r:
                for serial, n in r.data['benches'].items():
                    if n == name:
                        r.data['benches'][serial] = new
                if name in r.data['resources']:
                    r.data['resources'][new] = r.data['resources'].pop(name)
            if os.path.exists(h.path):
                os.replace(h.path, os.path.join(RUN_DIR, new + '.hold'))
            h.path = os.path.join(RUN_DIR, new + '.hold')
        say(name, 'renamed %s' % new)
    elif cmd == 'peek':
        out = sys.stdout.buffer
        while True:   # each tool that drives the bench serves its own
            sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            try:
                sock.connect(peek_address(name))
            except OSError:
                sock.close()
                time.sleep(0.5)
                continue
            while True:
                d = sock.recv(65536)
                if not d:
                    break
                out.write(d)
                out.flush()
            sock.close()
    elif cmd == 'env':
        p = ports(name)
        if p is None:
            sys.exit('%s has no Teensy' % name)
        print('export BIONIC_BENCH=%s BIONIC_PORT=%s BIONIC_HALT_PORT=%s'
              % (name, p[0], p[1]))
    elif cmd == 'wait':
        for ev in follow():
            if ev and ev[1] == name and ev[2].startswith('seated '):
                print(ev[2][len('seated '):])
                return
    else:
        sys.exit(__doc__)


if __name__ == '__main__':
    main()
