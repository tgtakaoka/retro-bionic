#!/usr/bin/env python3
"""Link CP/M-68K's MC68010 system on a 68000 board.

    debugger/mc68000/tools/cpm68k10-build.py FETCHED [CAST]

FETCHED is the folder cpm68k-fetch.py filled (samples/mc68000/cpm68k.fetch);
with CAST, the build is recorded there as an asciinema cast.

DRI linked CPM15000.SR for the MC68000, where a program reads SR freely;
on the MC68010 that instruction is privileged, and LO68's -UM68010 links
the BDOS's handler for it. No host linker reads CPMLIB, so the link runs
on the bench the bench tools pick: BIONIC_BENCH, the one the session
holds (scripts/bionic-bench.py lock NAME) or the only one on USB, where
an MC68000, an MC68008P or an MC68008FN must answer. The script puts
cpm68k.s19 and the disks on the card as `make cpm68k-sd` does, boots
CP/M-68K and runs samples/mc68000/cpm68k10_build.scenario, the steps of
cpm68k10.sub at A>, then copies A: back and takes CPM68010.SYS off it
into FETCHED/, for the Makefile's cpm68k10.s19. A minute or two.
"""
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PROJ = os.path.normpath(os.path.join(HERE, '..', '..', '..'))
SAMPLES = os.path.join(PROJ, 'samples', 'mc68000')
SCRIPTS = os.path.join(PROJ, 'scripts')
CPUS = 'MC68000,MC68008P,MC68008FN'


def run(*cmd):
    subprocess.run(cmd, check=True)


def scenario(lines):
    with tempfile.NamedTemporaryFile('w', suffix='.scenario') as f:
        f.write('\n'.join(lines) + '\n')
        f.flush()
        run(os.path.join(SCRIPTS, 'record-scenario.py'), f.name, '--nopace')


def build(cast):
    """Runs the build scenario, recorded as |cast| when given."""
    scenario_file = os.path.join(SAMPLES, 'cpm68k10_build.scenario')
    record = os.path.join(SCRIPTS, 'record-scenario.py')
    if cast:
        run(record, scenario_file, '--save', cast)
    else:
        run(record, scenario_file, '--nopace')


def main(argv):
    if len(argv) not in (1, 2):
        sys.exit(__doc__.strip())
    fetched = argv[0]
    cast = argv[1] if len(argv) == 2 else None
    files = [os.path.join(SAMPLES, f)
             for f in ('cpm68k.s19', 'cpm68ka.dsk', 'cpm68kb.dsk')]
    for path in files:
        if not os.path.exists(path):
            sys.exit(f'{path} missing: run make cpm68k')
    # sample-sd.py checks the CPU, unmounts, and keeps the card's B:.
    run(os.path.join(SCRIPTS, 'sample-sd.py'), '--cpu', CPUS, '--dir',
        'mc68000', *files[:2], '--keep', files[2])
    build(cast)
    scenario([line for d in 'ABCD'
              for line in ('type O', 'wait Disk?', f'command {d}')])
    control = os.path.join(SCRIPTS, 'bionic-control.py')
    with tempfile.TemporaryDirectory() as tmp:
        disk = os.path.join(tmp, 'cpm68ka.dsk')
        run(control, 'sd', 'get', 'mc68000/cpm68ka.dsk', disk)
        os.makedirs(fetched, exist_ok=True)
        run(os.path.join(SCRIPTS, 'cpm-disk.py'), 'get', disk,
            'CPM68010.SYS', os.path.join(fetched, 'CPM68010.SYS'))
    print(f'{fetched}/CPM68010.SYS: linked on the board')


if __name__ == '__main__':
    try:
        main(sys.argv[1:])
    except subprocess.CalledProcessError as e:
        sys.exit(f'{os.path.basename(e.cmd[0])} failed')
