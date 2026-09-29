"""Z86C91 (Z8) plugin for scripts/record-cycles.py; see cycles_z8.py.

The trap is HALT ($7F). Operand bytes are the trap, which as a register
is the general register 7F, and as a working register r7 or r15. Every
run seeds registers 7E and 7F with themselves, so @7F points at itself
and the pair 7E names 7E7F, where JP @RR and CALL @RR go. @rr operands
are rr4, pointing at DATA. STOP stops the CPU at once, and runs last.

    P=debugger/z8/tools/cycles_z86.py
    R=debugger/z8/tools/z86-cycles.jsonl.zst
    scripts/record-cycles.py $P fill
    scripts/record-cycles.py $P run --record $R
    scripts/record-cycles.py $P check --record $R
"""
import importlib.machinery
import os

_base = importlib.machinery.SourceFileLoader(
    'cycles_z8', os.path.join(os.path.dirname(os.path.abspath(__file__)),
                              'cycles_z8.py')).load_module()
ORG, DATA = _base.ORG, _base.DATA

HALT = 0x7F
STOP = 0x6F
PAIR = 0x7E
RP = 0x10                       # r0-r15 are 10-1F; 00-03 are the ports
R7_RR4 = 0x74                   # r7 and rr4, of an @rr form


def special(opc, mnemo, opr, length):
    if mnemo in ('INCW', 'DECW'):
        return [dict(bytes=[opc, PAIR], target=ORG + length)]
    if opr == '@RR':
        return [dict(bytes=[opc, PAIR], target=PAIR << 8 | HALT)]
    if mnemo == 'SRP':
        return [dict(bytes=[opc, RP], target=ORG + length)]
    if 'rr' in opr:
        return [dict(bytes=[opc, R7_RR4], target=ORG + length)]
    return None


# r7 and r15, as pointers, point at themselves.
# HALT reads the byte after it, then stops the bus.
_base.define(globals(), 'z86', ('Z86C91',), HALT,
             base=[('PC', ORG), ('SP', _base.BASE_SP), ('RP', RP), ('FLAGS', 0x00),
                   ('R7', RP + 7), ('R15', RP + 15), ('RR4', DATA)],
             seed=(PAIR, [PAIR, HALT]), special=special, halts=(STOP,), tail=1)
