#!/usr/bin/env python3
"""Assemble MS-DOS from source on the 8086 board.

    debugger/i8086/tools/msdos-build.py tree VERSION FETCHED OUT
    debugger/i8086/tools/msdos-build.py bench VERSION OUT
    debugger/i8086/tools/msdos-build.py check VERSION OUT

VERSION is 2.11 or 2.00. FETCHED is the folder msdos-fetch.py --source
filled (samples/i8086/msdos.fetch).

`tree` writes into OUT/ the files of a build disk: the sources, patched
as below, MASM 5.10, LINK 3.65 and the 2.0 EXE2BIN, BUILD.BAT, and a
CONFIG.SYS and AUTOEXEC.BAT. samples/i8086/Makefile puts them on
bld211.dsk or bld200.dsk, a 32 MB hard disk that boots MS-DOS 2.0.

`bench` builds on the board the session holds (scripts/bionic-bench.py
lock first): with the block cache off, it copies
samples/i8086/bldVER.dsk to the SD card, boots it as C: and runs BUILD
there (samples/i8086/msdosVER_build.scenario), copies the disk back,
turns the cache on and mounts C: and A: as L does, and puts what BUILD
made into OUT/: MSDOS.SYS, and for 2.11 COMMAND.COM, SYSINIT.OBJ,
SYSIMES.OBJ and the utilities in OUT/bin/, for the Makefile's
msdos211*.dsk. It needs the card's i8086/msdos200.hex, msdos200a.dsk
and msdos200c.dsk (docs/samples.md), and takes half an hour.

`check` prints what `bench` put in OUT/ and compares it, as `bench`
does at its end, with the known 2.11 build or the 2.0 release.

2.11: the MSDOS.SYS kernel is John Elliott's tree, Microsoft's with the
console I/O files it lacks; COMMAND.COM, SYSINIT and the utilities 2.0's
disks have are Microsoft's. Patched to build the generic MS-DOS, as
Microsoft's OEMs got it, not the IBM one, whose kernel leaves EXEC out:
STDSW.ASM and COMSW.ASM say MSVER, not IBM, but DOSMES.ASM keeps the UK,
German, French and Japanese country tables and the case map of their
accented letters, while its line editing keys stay the terminal's escape
sequences. COMMAND.ASM's one use of the DOS function WAIT, an
instruction to MASM 5.10, becomes its number. DEBUG, MORE, RECOVER and
PRINT are set to MSVER too. SYSIMES.ASM gains the messages SYSINIT.ASM
needs, which the 2.0 one lacks.

2.00: John Elliott's tree of the kernel alone, which reproduces the
2.0 release's MSDOS.SYS with MASM 4.00; with MASM 5.10 it comes out
3 bytes shorter, three AND word immediates encoded in a byte, which
`bench` reports.
"""
import hashlib
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PROJ = os.path.normpath(os.path.join(HERE, '..', '..', '..'))
SAMPLES = os.path.join(PROJ, 'samples', 'i8086')
SCRIPTS = os.path.join(PROJ, 'scripts')

# The kernel's modules in DOSLINK order.
KERNEL_211 = ('msdos mscode dosmes misc getset dircall alloc dev dir disk '
              'fat rom stdbuf stdcall stdctrlc stdfcb stdproc stdio time '
              'xenix xenix2').split()
KERNEL_200 = ('msdos mscode sleaze getset misc getset2 misc2 dircall '
              'getset3 absdisk alloc dev dir disk fat rom stdbuf stdcall '
              'stdctrlc stdfcb stdproc stdio time xenix').split()
COMMAND = ('command rucode rdata init uinit tcode tcode2 tcode3 tcode4 '
           'tcode5 tucode copy copyproc cparse tdata tspc').split()
COMMAND_INCLUDES = 'comequ comseg comsw ifequ'.split()
# The utilities 2.0's disks have: (program, its modules in link order).
# A .COM is linked as an .EXE and converted by EXE2BIN; the .EXEs link
# under another name, so that EXE2BIN's own stays the 2.0 one.
UTILITIES = [
    ('CHKDSK.COM', 'chkdsk chkproc chkmes'),
    ('DEBUG.COM', 'debug debcom1 debcom2 debuasm debasm debconst debdata '
                  'debmes'),
    ('EDLIN.COM', 'edlin edlproc edlmes'),
    ('MORE.COM', 'more moremes'),
    ('RECOVER.COM', 'recover recmes'),
    ('PRINT.COM', 'print'),
    ('EXE2BIN.EXE', 'exe2bin exemes'),
    ('FC.EXE', 'fc fcmes'),
    ('FIND.EXE', 'find findmes'),
    ('SORT.EXE', 'sort sortmes'),
]
# LINK options: SORT allocates its buffer, so it must not be loaded into
# all of memory, as LINK 3.65 would load it.
LINK_OPTIONS = {'SORT.EXE': '/CP:1'}
UTILITY_INCLUDES = ['debequ']

# sha256 of what the 2.11 build makes, as the P80C86 made it twice.
BUILT_211 = {
    'MSDOS.SYS':
        'a8c35583488c930f2b35203538b65d0744e42a6f653102415ac4dd60887306b3',
    'COMMAND.COM':
        '6cea15fc363ba0a7d56831dce6669f0ff3164822a496459206e9f43c3d93b21f',
    'SYSINIT.OBJ':
        '484a9f4f37d8bd92c61a851837b00937a94b808037f5670c0ba5d0024500d70c',
    'SYSIMES.OBJ':
        '9da8db5a58c5cc40a23d6ae7240987df23415e87dcd9335edb245d4e75434925',
    'bin/CHKDSK.COM':
        'e05bf373b6d889e947d2562793cd81b7862609ebd34b934ad49274b8e94689b9',
    'bin/DEBUG.COM':
        'f1cc9a5d7fdea6d482921b42af21029b49715160ccddf15e0f303b1ce5d8e433',
    'bin/EDLIN.COM':
        'f0dcc6de7770111635b3082b6352b03147ecb2dcdbca42ca529f4943a9b8db5f',
    'bin/MORE.COM':
        '8e2463a9ca082ecd8c8dcbf41381bfeb24b8e55bb04fc2ac68b407b738647451',
    'bin/RECOVER.COM':
        'a7aa6dd7fd692a2ea181181a79adf7658f1d69c0b694442ceb99f62243e63daa',
    'bin/PRINT.COM':
        '79bf31e70a7c4697295df2443509badd37605dbe053ec72ed7d5818a56730242',
    'bin/EXE2BIN.EXE':
        '49f136f04ce531aa75922582aa0c33fd4299cf4d18f0a1789e6006d6d4a46ee5',
    'bin/FC.EXE':
        '96f1f0f401023895006f0cebaf5b158b363d65f845a7607e3d41b66499546bbe',
    'bin/FIND.EXE':
        'bd17c80b98f327719b76669aabcb70fa0a830d320b54bf29ca7539fb00d6ff47',
    'bin/SORT.EXE':
        'fcd8f6e8d0af36fe101e71f45258515133baa17b28aa095b5c05c02ef357f724',
}

# sha256 of the 2.0 release's MSDOS.SYS.
MSDOS_200 = '1edf5190671ed4edcc9e6cc095e5094749c58f1833a2e26e40ba7d3389e276d1'

# Messages SYSINIT.ASM 2.11 prints that the 2.0 SYSIMES.ASM lacks: a
# file name goes between _PRE and _POST.
SYSIMES_211 = b'''\
SYSINITSEG      SEGMENT PUBLIC BYTE 'SYSTEM_INIT'

        PUBLIC  BADOPM,CRLFM,BADSIZ_PRE,BADLD_PRE,BADSIZ_POST,BADLD_POST
        PUBLIC  BADCOM,BADCOUNTRY,SYSSIZE

BADOPM  DB      13,10,"Unrecognized command in CONFIG.SYS"
BADSIZ_POST     LABEL   BYTE
BADLD_POST      LABEL   BYTE
CRLFM   DB      13,10,'$'

BADSIZ_PRE      DB      13,10,"Sector size too large in file $"

BADLD_PRE       DB      13,10,"Bad or missing $"

BADCOM  DB      "Command Interpreter",0

BADCOUNTRY      DB      13,10,"Invalid country code",13,10,'$'

SYSSIZE LABEL   BYTE

SYSINITSEG      ENDS
        END
'''


def text(data):
    """Source as DOS text: CRLF lines, without the NUL and ^Z padding
    the release's files end in, nor the line feeds with bit 7 set that
    TDATA.ASM has, which MASM 5.10 takes for no line end at all."""
    data = data.rstrip(b'\0\x1a\r\n ').replace(b'\r\x8a', b'\r\n')
    data = data.replace(b'\r\n', b'\n')
    return data.replace(b'\n', b'\r\n') + b'\r\n'


def patch(data, old, new, name, count=1, flags=0):
    """|data| with the |count| lines matching |old| (a regex) replaced."""
    lines = data.split(b'\r\n')
    hits = [i for i, l in enumerate(lines) if re.fullmatch(old, l, flags)]
    if len(hits) != count:
        sys.exit(f'{name}: {len(hits)} lines match {old!r}')
    for i in hits:
        lines[i] = new
    return b'\r\n'.join(lines)


def read(path):
    with open(path, 'rb') as f:
        return f.read()


def batch(lines):
    return ('\r\n'.join(lines) + '\r\n').encode('ascii')


def masm_steps(modules):
    """A module already assembled is skipped, so BUILD picks up where it
    stopped; MASM leaves no object behind a severe error."""
    for m in modules:
        yield f'IF NOT EXIST {m.upper()}.OBJ MASM {m.upper()};'
        yield 'IF ERRORLEVEL 1 GOTO FAIL'


def link_steps(response, exe, out):
    """LINK's errorlevel counts warnings too, such as no stack segment,
    so the files it and EXE2BIN make are what tells."""
    return [f'LINK @{response}',
            f'IF NOT EXIST {exe} GOTO FAIL',
            f'EXE2BIN {exe} {out}',
            f'IF NOT EXIST {out} GOTO FAIL']


def built_name(program):
    """What BUILD makes |program| as: the .COMs as such, an .EXE with its
    first letter swapped for "U"."""
    name, ext = program.split('.')
    return program if ext == 'COM' else 'U' + name[1:] + '.EXE'


def utility_steps():
    for program, modules in UTILITIES:
        modules = modules.split()
        yield from masm_steps(modules)
        name, ext = program.split('.')
        objs = '+'.join(m.upper() for m in modules)
        if ext == 'COM':
            yield f'LINK {objs},{name}.EXE;'
            yield f'IF NOT EXIST {name}.EXE GOTO FAIL'
            yield f'EXE2BIN {name}.EXE {program}'
        else:
            yield (f'LINK {objs},{built_name(program)}'
                   f'{LINK_OPTIONS.get(program, "")};')
        yield f'IF NOT EXIST {built_name(program)} GOTO FAIL'


def build_bat(steps):
    """BUILD.BAT: its last line, either way, is "BUILD END", which the
    build scenario waits for."""
    return batch(['ECHO OFF'] + steps + [
        'ECHO BUILD OK', 'GOTO END', ':FAIL', 'ECHO BUILD FAILED', ':END',
        'ECHO BUILD END'])


def tree(version, fetched, out):
    src = os.path.join(fetched, 'src')
    files = {}
    if version == '2.11':
        jce = os.path.join(src, 'jce211')
        for name in os.listdir(jce):
            if name.endswith('.ASM'):
                files[name] = text(read(os.path.join(jce, name)))
        files['STDSW.ASM'] = patch(files['STDSW.ASM'],
                rb'MSVER\s+EQU\s+false', b'MSVER   EQU     true', 'STDSW.ASM')
        files['STDSW.ASM'] = patch(files['STDSW.ASM'],
                rb'IBM\s+EQU\s+true', b'IBM     EQU     false', 'STDSW.ASM')
        files['DOSMES.ASM'] = patch(files['DOSMES.ASM'],
                rb'IF\tIBM *', b'IF\tTRUE', 'DOSMES.ASM', 3)
        ms = os.path.join(src, 'ms')
        for m in COMMAND + COMMAND_INCLUDES + ['sysinit']:
            name = m.upper() + '.ASM'
            files[name] = text(read(os.path.join(ms, name)))
        files['COMSW.ASM'] = patch(files['COMSW.ASM'],
                rb'IBMVER\s+EQU\s+true\s*(;.*)?',
                b'IBMVER  EQU     false', 'COMSW.ASM')
        files['COMSW.ASM'] = patch(files['COMSW.ASM'],
                rb'MSVER\s+EQU\s+false\s*(;.*)?',
                b'MSVER   EQU     true', 'COMSW.ASM')
        # WAIT is an instruction to MASM 5.10, which then sizes the line
        # differently in its two passes.
        files['COMMAND.ASM'] = patch(files['COMMAND.ASM'],
                rb'\s+MOV\s+AH,WAIT\s*', b'        MOV     AH,77   ; WAIT',
                'COMMAND.ASM')
        files['SYSIMES.ASM'] = text(SYSIMES_211.replace(b'\n', b'\r\n'))
        for program, modules in UTILITIES:
            for m in modules.split() + UTILITY_INCLUDES:
                name = m.upper() + '.ASM'
                source = 'PRINT_v211.ASM' if m == 'print' else name
                files[name] = text(read(os.path.join(ms, source)))
        for name, switches in (('DEBEQU.ASM', ('IBMVER', 'MSVER')),
                               ('MORE.ASM', ('IBMVER', 'MSVER')),
                               ('RECOVER.ASM', ('IBMVER',)),
                               ('PRINT.ASM', ('IBM', 'MSVER'))):
            for switch in switches:
                value = b'true' if switch == 'MSVER' else b'false'
                files[name] = patch(files[name],
                        rb'%s\s+EQU\s+(true|false)\s*(;.*)?' % switch.encode(),
                        switch.encode().ljust(8) + b'EQU     ' + value,
                        name, flags=re.I)
        files['DOSLINK'] = batch([
            ' '.join(KERNEL_211[:9]) + ' +',
            ' '.join(KERNEL_211[9:17]) + ' +',
            ' '.join(KERNEL_211[17:]),
            'MSDOS.EXE;'])
        files['CMDLINK'] = batch([
            ' '.join(COMMAND[:8]) + ' +',
            ' '.join(COMMAND[8:]),
            'CMD.EXE;'])
        steps = (list(masm_steps(KERNEL_211)) +
                 link_steps('DOSLINK', 'MSDOS.EXE', 'MSDOS.BIN') +
                 list(masm_steps(COMMAND)) +
                 link_steps('CMDLINK', 'CMD.EXE', 'CMD.COM') +
                 list(masm_steps(['sysinit', 'sysimes'])) +
                 list(utility_steps()))
    elif version == '2.00':
        jce = os.path.join(src, 'jce200')
        for name in os.listdir(jce):
            if name.endswith('.ASM'):
                files[name] = text(read(os.path.join(jce, name)))
        files['DOSLINK'] = batch([
            ' '.join(KERNEL_200[:10]) + ' +',
            ' '.join(KERNEL_200[10:22]) + ' +',
            ' '.join(KERNEL_200[22:]),
            'MSDOS.EXE;'])
        steps = (list(masm_steps(KERNEL_200)) +
                 link_steps('DOSLINK', 'MSDOS.EXE', 'MSDOS.BIN'))
    else:
        sys.exit(f'{version}: 2.11 or 2.00')
    files['BUILD.BAT'] = build_bat(steps)
    files['CONFIG.SYS'] = batch(['FILES=20', 'BUFFERS=20'])
    files['AUTOEXEC.BAT'] = batch(['VER'])
    tools = os.path.join(src, 'tools')
    for name in ('MASM.EXE', 'LINK.EXE'):
        files[name] = read(os.path.join(tools, name))
    files['EXE2BIN.EXE'] = read(os.path.join(fetched, 'bin', 'EXE2BIN.EXE'))
    if os.path.isdir(out):
        shutil.rmtree(out)
    os.makedirs(out)
    for name, data in sorted(files.items()):
        with open(os.path.join(out, name), 'wb') as f:
            f.write(data)
    open(os.path.join(out, '.tree'), 'w').close()


def sha256(path):
    return hashlib.sha256(read(path)).hexdigest()


def run(*argv, **kw):
    print('+', ' '.join(argv), flush=True)
    return subprocess.run(argv, check=True, **kw)


def console(lines):
    """Types |lines|, scenario directives, at the board's prompt."""
    with tempfile.NamedTemporaryFile('w', suffix='.scenario') as f:
        f.write('\n'.join(lines) + '\n')
        f.flush()
        run(os.path.join(SCRIPTS, 'record-scenario.py'), f.name, '--nopace')


def bench(version, out):
    tag = {'2.11': '211', '2.00': '200'}.get(version)
    if tag is None:
        sys.exit(f'{version}: 2.11 or 2.00')
    disk = os.path.join(SAMPLES, f'bld{tag}.dsk')
    if not os.path.exists(disk):
        sys.exit(f'{disk}: make bld{tag}.dsk first')
    control = os.path.join(SCRIPTS, 'bionic-control.py')
    card = f'i8086/bld{tag}.dsk'
    # The block cache stays off from before the copy to the card until
    # the copy back: with it on, the board's writes and the copies have
    # been seen to miss the card (2026-11-25).
    console(['type O', 'wait Disk?', 'command C',
             'type O', 'wait Disk?', 'command cache off'])
    run(control, 'sd', 'put', disk, card)
    scenario = os.path.join(SAMPLES, f'msdos{tag}_build.scenario')
    os.makedirs(out, exist_ok=True)
    log = os.path.join(out, 'build.log')
    with open(log, 'wb') as f:
        p = subprocess.Popen([os.path.join(SCRIPTS, 'record-scenario.py'),
                              scenario], stdout=subprocess.PIPE)
        for chunk in iter(lambda: p.stdout.read1(4096), b''):
            sys.stdout.buffer.write(chunk)
            sys.stdout.buffer.flush()
            f.write(chunk)
            f.flush()
        if p.wait():
            sys.exit(f'{scenario}: failed, see {log}')
    if b'BUILD OK' not in read(log):
        sys.exit(f'BUILD failed, see {log}')
    built = os.path.join(out, f'bld{tag}.dsk')
    run(control, 'sd', 'get', card, built)
    console(['type O', 'wait Disk?', 'command cache on',
             'type O', 'wait Disk?', 'command C i8086/msdos200c.dsk',
             'type O', 'wait Disk?', 'command A i8086/msdos200a.dsk'])
    get = os.path.join(SCRIPTS, 'msdos-disk.py')
    for name, as_name in outputs(version):
        if os.path.dirname(as_name):
            os.makedirs(os.path.join(out, os.path.dirname(as_name)),
                        exist_ok=True)
        run(get, 'get', built, name, os.path.join(out, as_name))
    check(version, out)


def outputs(version):
    """(name on the build disk, name in OUT) of what BUILD makes."""
    names = [('MSDOS.BIN', 'MSDOS.SYS')]
    if version == '2.11':
        names += [('CMD.COM', 'COMMAND.COM'), ('SYSINIT.OBJ', 'SYSINIT.OBJ'),
                  ('SYSIMES.OBJ', 'SYSIMES.OBJ')]
        names += [(built_name(p), os.path.join('bin', p))
                  for p, _ in UTILITIES]
    return names


def check(version, out):
    """Prints each output's sha256, and how the build compares with the
    known 2.11 one or the 2.0 release."""
    differ = []
    for _, as_name in outputs(version):
        path = os.path.join(out, as_name)
        print(f'{sha256(path)}  {as_name} ({os.path.getsize(path)} bytes)')
        if version == '2.11' and sha256(path) != BUILT_211[as_name]:
            differ.append(as_name)
    if version == '2.11':
        print(f'differ from the known build: {" ".join(differ)}' if differ
              else 'all as the known build')
    else:
        built = read(os.path.join(out, 'MSDOS.SYS'))
        if hashlib.sha256(built).hexdigest() == MSDOS_200:
            print('MSDOS.SYS is the 2.0 release\'s')
        else:
            release = os.path.join(SAMPLES, 'msdos.fetch', 'MSDOS.SYS')
            print(f'MSDOS.SYS is {len(built)} bytes, the release\'s '
                  f'{os.path.getsize(release)}: MASM 5.10 encodes three AND '
                  f'word immediates that fit a byte shorter than MASM 4.00 '
                  f'did')


def main(argv):
    if len(argv) == 4 and argv[0] == 'tree':
        tree(*argv[1:])
    elif len(argv) == 3 and argv[0] == 'bench':
        bench(*argv[1:])
    elif len(argv) == 3 and argv[0] == 'check' and argv[1] in ('2.11', '2.00'):
        check(*argv[1:])
    else:
        sys.exit(__doc__)


if __name__ == '__main__':
    main(sys.argv[1:])
