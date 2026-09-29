"""MC68HC05C0: cycles_mc146805e2.py with mc68hc05.txt, for
scripts/record-cycles.py.

    P=debugger/mc6805/tools/cycles_mc68hc05c0.py
    R=debugger/mc6805/tools/mc68hc05c0-cycles.jsonl.zst
    C=samples/mc68hc05/bench/channels.toml:mc68hc05c0
    scripts/record-cycles.py $P fill
    scripts/record-cycles.py $P run --record $R --channels $C --capture debug
    scripts/record-cycles.py $P check --record $R
"""
import importlib.machinery
import os

_base = importlib.machinery.SourceFileLoader(
    'cycles_mc146805e2', os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                      'cycles_mc146805e2.py')).load_module()


def _strobe(row, col):
    if row[col('#RD')] == '0':
        return 'R'
    return 'W' if row[col('#WR')] == '0' else None


# Below 240 only 1C-2F is plain external memory: 00 and 04 are Port A's,
# 30-3F stretch the bus cycle, the rest is on-chip I/O and RAM, with SP
# within C0-FF. Direct operands and X point into 1C-2F. The debugger
# leaves STOP disabled (CNFGR STPEN clear), where it resets the chip.
_base.define(globals(), 'mc68hc05', ('MC68HC05C0',), max_addr=0xFFFF,
             internal=lambda addr: addr < 0x1C or 0x30 <= addr < 0x240,
             acia=range(0xFFE0, 0xFFF0), sleep={'8F'}, skip={'8E'},
             opr={'X': 0x20, 'd8': 0x28, 'n8': 0x08, 'a16': 0x1100},
             stack=range(0xC0, 0x100), strobe=_strobe, min_width=60e-9)
