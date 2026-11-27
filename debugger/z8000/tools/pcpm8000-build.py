#!/usr/bin/env python3
"""Compile CP/M-8000 1.2 from P-CP/M-Z8K's sources on the Z8002.

    debugger/z8000/tools/pcpm8000-build.py tree CPU FETCHED OUT
    debugger/z8000/tools/pcpm8000-build.py bench CPU OUT
    debugger/z8000/tools/pcpm8000-build.py check CPU OUT

CPU is Z8002, which picks samples/z8002 and Kontron's Z8002 BDOS and
exception glue. FETCHED is the folder cpm8000-fetch.py and its --source
filled (the samples folder's cpm8000.fetch).

`tree` writes into OUT/ the files of a build disk: the CCP, the BDOS
and the glue, patched as below, and the DRI compiler and assembler that
build them. The samples folder's Makefile puts them on bld8002a.dsk,
which the 1.1 system, bld8002.hex, mounts alone as A:.

`bench` builds on the bench the bench tools pick: BIONIC_BENCH, the one
the session holds (scripts/bionic-bench.py lock NAME) or the only one.
Once CPU answers there, with the block cache off, it puts the 1.1
system and the build disk on the SD card, runs the samples folder's
pcpm8000_build.scenario, which loads the system from the card with the
disk beside it, compiles and assembles there, copies the disk back,
and puts the objects in OUT/, for the Makefile's 1.2 system, echoing
what the board prints as it goes. It takes a few minutes. `check` compares OUT/'s objects with the known
build, as `bench` does at its end before it writes OUT/.built.

The patches: the BDOS's and CCP's STDIO.H lost their last lines in the
archive, which Kontron's copy has; the BDOS's sign-on string has a form
feed and a line break inside its quotes; PGMLD.C includes "x.out.h",
no CP/M name, and uses X_SG_DATA, which XOUT.H calls X_SG_DAT;
BIOSDEFS2.Z8K is a 9-letter name. Kontron's Z8002 has one address
space, so its BDOS entry passes a program's parameter as is; here the
lines it left commented out give it the program's data segment,
_usrseg of the BIOS. The CCP compiles with the BDOS's headers, the same
but for declarations it does not use.
"""
import hashlib
import os
import re
import struct
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PROJ = os.path.normpath(os.path.join(HERE, '..', '..', '..'))
SCRIPTS = os.path.join(PROJ, 'scripts')
# Per CPU: its samples folder, which names the build disk, the 1.1 system
# the board builds on and the card folder, and the BDOS and exception
# glue, Kontron's for the Z8002.
CPUS = {
    'Z8002': ('z8002', ('bdosif2', 'exceptn2')),
}

C_MODULES = ('ccp go bdosmain bdosmisc bdosrw conbdos dskutil fileio '
             'iosys pgmld').split()
HEADERS = 'basepage bdosdef biosdef pktio'.split()
TOOLS = ('ZCC.Z8K ZCC1.Z8K ZCC2.Z8K ZCC3.Z8K ASZ8K.Z8K ASZ8K.PD '
         'XCON.Z8K').split()

# sha256 of each object's x.out, the known build.
BUILT = {
    'bdosif2':
        '60784540addf4ae18c441ca6c56572ecbf701480108d8e5ee6e57a39cc067a93',
    'bdosmain':
        '5e90e6a54586abe80f4019629ccff49926a014a2de67678ced9058ac3bc273ab',
    'bdosmisc':
        '9fc194677ec14a12586f1f1f3375f94d7f71fa86a3b8b0ea866ccf9a2f117ca7',
    'bdosrw':
        'ca66dc857fc6a125318b7dd7a8dc18586e204a12fbfe22d43f66d86f99a758ef',
    'ccp':
        'd5350508462ca6c8dae96ff455f03c5e21fefdd39b2dd3a069a4913601d38cac',
    'ccpif':
        '585cfe34b060278ba905f82996c8d563928e306ac168cd4cca1a1f98bc058e4d',
    'conbdos':
        '3e79adaef6e3dd9ee7bd97b09e253b5b73c1c5ffea39f2a7f586462ac841d794',
    'dskutil':
        '806ef7aea33bce3905b8fb4375fec2e13ebf1bb14af16a31f4d3f3f7b0d9613c',
    'exceptn2':
        '3116ba8619a353afae87b631eb95c1e8915d30cbc7661baf7ac2d05f93d97b7e',
    'fileio':
        'c7def0b14d15c55b0e389651c460007152701086224a15855558c21bb9964430',
    'go':
        '7e1bc3a7f976a5ba4cbaa4e4d033165389958e8f7166c40dfa533104cf5046c3',
    'iosys':
        '6ff39fb2121b6af09156140b3c20f09fe78fb5a93e42c9c176b6e8102d20d8a0',
    'pgmld':
        '7ecd1ae63835e2bab360402fc644757df2c1be704bc4ccb26463c593acd3e496',
}


def read(path):
    with open(path, 'rb') as f:
        return f.read()


def text(data):
    """Source as CP/M text: CRLF lines, up to the ^Z."""
    data = data.split(b'\x1a')[0].rstrip(b'\0\r\n ')
    data = data.replace(b'\r\n', b'\n').replace(b'\r', b'\n')
    return data.replace(b'\n', b'\r\n') + b'\r\n'


def patch(data, old, new, name):
    """|data| with the one occurrence of |old| replaced."""
    if data.count(old) != 1:
        sys.exit(f'{name}: {data.count(old)} matches of {old!r}')
    return data.replace(old, new)


def stdio(src):
    """The BDOS's STDIO.H, its lost end from Kontron's copy."""
    mine = text(read(os.path.join(src, 'bdos', 'stdio.h'))).split(b'\r\n')
    whole = text(read(os.path.join(src, 'newos', 'stdio.h'))).split(b'\r\n')
    tail = b'#define\tabs(x)'
    cut = next(i for i, l in enumerate(mine) if l.startswith(tail))
    return b'\r\n'.join(mine[:cut] +
                        whole[next(i for i, l in enumerate(whole)
                                   if l.startswith(tail)):])


def cpu_of(name):
    """The samples folder and the assembled modules of CPU |name|."""
    if name.upper() not in CPUS:
        sys.exit(f'{name}: not one of {" ".join(CPUS)}')
    folder, glue = CPUS[name.upper()]
    return folder, ['ccpif'] + list(glue)


def tree(cpu, fetched, out):
    folder, modules = cpu_of(cpu)
    src = os.path.join(fetched, 'src')
    files = {'STDIO.H': stdio(src)}
    for h in HEADERS:
        files[f'{h.upper()}.H'] = text(read(os.path.join(src, 'bdos',
                                                         f'{h}.h')))
    for m in C_MODULES:
        d = 'ccp' if m in ('ccp', 'go') else 'bdos'
        files[f'{m.upper()}.C'] = text(read(os.path.join(src, d, f'{m}.c')))
    files['CCPDEF.H'] = text(read(os.path.join(src, 'ccp', 'ccpdef.h')))
    files['BDOSMISC.C'] = patch(
        files['BDOSMISC.C'], b'prt_line("\x0c\r\n\\r\\nCPM',
        b'prt_line("\\r\\nCPM', 'BDOSMISC.C')
    files['PGMLD.C'] = patch(files['PGMLD.C'], b'"x.out.h"', b'"xout.h"',
                             'PGMLD.C')
    files['XOUT.H'] = (text(read(os.path.join(src, 'newos', 'xout.h'))) +
                       b'#define X_SG_DATA X_SG_DAT\r\n')
    files['CCPIF.8KN'] = text(read(os.path.join(src, 'ccp', 'ccpif.z8k')))
    z8002_glue(src, files)
    for t in TOOLS:
        files[t] = read(os.path.join(fetched, 'a', t))
    os.makedirs(out, exist_ok=True)
    for name in os.listdir(out):
        if name not in files and not name.startswith('.'):
            os.remove(os.path.join(out, name))
    for name, data in files.items():
        with open(os.path.join(out, name), 'wb') as f:
            f.write(data)
    open(os.path.join(out, '.tree'), 'w').close()


def z8002_glue(src, files):
    """Kontron's Z8002 glue, BIOSDEFS2.Z8K under an 8.3 name, and the BDOS
    entry giving a program's parameter its data segment."""
    files['BIOSDEF2.Z8K'] = text(read(os.path.join(src, 'newos',
                                                   'biosdefs2.z8k')))
    for m in ('bdosif2', 'exceptn2'):
        files[f'{m.upper()}.8KN'] = patch(
            text(read(os.path.join(src, 'newos', f'{m}.z8k'))),
            b'"biosdefs2.z8k"', b'"biosdef2.z8k"', f'{m}.z8k')
    files['BDOSIF2.8KN'] = patch(files['BDOSIF2.8KN'], (
        b';\tld\tr2,_sysseg\t; assume system mode\r\n'
        b';\tld\tr0,scfcw+co(r15)\r\n'
        b';\tbit\tr0,#14\t\t; Was user system?\r\n'
        b';\tjr\tnz,callC\t; Yes.\r\n'
        b';\tld\tr2,_usrseg\t; user  pseudo segment\r\n'), (
        b'\tld\tr0,scfcw+co(r15)\r\n'
        b'\tbit\tr0,#14\t\t; Was user system?\r\n'
        b'\tjr\tnz,callC\t; Yes: rr6 is physical\r\n'
        b'\tld\tr2,_usrseg\t; user data segment\r\n'), 'bdosif2.z8k')


def xout(data):
    """An x.out file without the padding to its last record."""
    _, nseg, init, reloc, symb = struct.unpack('>HHIII', data[:16])
    return data[:16 + 4 * nseg + init + reloc + symb]


def shown(arg):
    """|arg| as the echo shows it: a path relative to where make runs."""
    return os.path.relpath(arg) if os.path.isabs(arg) else arg


def run(*argv, echo=True, **kw):
    if echo:
        print('+', ' '.join(shown(a) for a in argv), flush=True)
    return subprocess.run(argv, check=True, **kw)


def scenario(lines):
    """Runs |lines|, scenario directives, with what the board prints
    going to stdout as it comes; echoes the debugger commands they type."""
    typed = [line.split(' ', 1)[1] for line in lines
             if line.startswith('command ')]
    print('+ O', ', O '.join(typed), flush=True)
    with tempfile.NamedTemporaryFile('w', suffix='.scenario') as f:
        f.write('\n'.join(lines) + '\n')
        f.flush()
        run(os.path.join(SCRIPTS, 'record-scenario.py'), f.name, '--nopace',
            echo=False)


def bench(cpu, out):
    folder, modules = cpu_of(cpu)
    samples = os.path.join(PROJ, 'samples', folder)
    stem = 'bld' + folder[1:]
    disk = os.path.join(samples, f'{stem}a.dsk')
    system = os.path.join(samples, f'{stem}.hex')
    for path in (disk, system):
        if not os.path.exists(path):
            sys.exit(f'{path} missing: run make pcpm8000')
    control = os.path.join(SCRIPTS, 'bionic-control.py')
    p = subprocess.run([control, 'probe'], capture_output=True, text=True)
    if p.returncode or f'({cpu.upper()})' not in p.stdout:
        sys.exit(f'no {cpu.upper()} at a bench prompt: '
                 f'{(p.stdout + p.stderr).strip()}')
    card = f'{folder}/{stem}a.dsk'
    # The block cache stays off from before the copy to the card until
    # the copy back, as for MS-DOS's build (2026-11-25). The scenario
    # loads the 1.1 system from the card, which mounts the disk beside it.
    scenario(['type O', 'wait Disk?', 'command cache off'])
    run(os.path.join(SCRIPTS, 'sample-sd.py'), '--cpu', cpu.upper(),
        '--dir', folder, system, disk)
    run(os.path.join(SCRIPTS, 'record-scenario.py'),
        os.path.join(samples, 'pcpm8000_build.scenario'), '--nopace')
    scenario([line for d in 'ABCD'
              for line in ('type O', 'wait Disk?', f'command {d}')])
    os.makedirs(out, exist_ok=True)
    built = os.path.join(out, f'{stem}a.dsk')
    run(control, 'sd', 'get', card, built)
    scenario(['type O', 'wait Disk?', 'command cache on'])
    get = os.path.join(SCRIPTS, 'cpm-disk.py')
    for m in C_MODULES + modules:
        path = os.path.join(out, f'{m}.o')
        run(get, 'get', built, f'{m.upper()}.O', path)
        data = xout(read(path))
        with open(path, 'wb') as f:
            f.write(data)
    check(cpu, out)
    open(os.path.join(out, '.built'), 'w').close()


def check(cpu, out):
    """Prints each object's sha256 and whether the build is the known
    one."""
    _, modules = cpu_of(cpu)
    differ = []
    for m in C_MODULES + modules:
        path = os.path.join(out, f'{m}.o')
        if not os.path.exists(path):
            differ.append(m)
            print(f'{"missing":64}  {m}.o')
            continue
        digest = hashlib.sha256(xout(read(path))).hexdigest()
        print(f'{digest}  {m}.o')
        if digest != BUILT[m]:
            differ.append(m)
    if differ:
        sys.exit(f'differ from the known build: {" ".join(differ)}')
    print('all as the known build')


def main(argv):
    if len(argv) == 4 and argv[0] == 'tree':
        tree(*argv[1:])
    elif len(argv) == 3 and argv[0] == 'bench':
        try:
            bench(*argv[1:])
        except subprocess.CalledProcessError as e:
            sys.exit(f'{os.path.basename(e.cmd[0])} failed')
    elif len(argv) == 3 and argv[0] == 'check':
        check(*argv[1:])
    else:
        sys.exit(__doc__)


if __name__ == '__main__':
    main(sys.argv[1:])
