"""Z88C00 (Super8) plugin for scripts/record-cycles.py; see cycles_z8.py.

The trap is WFI ($3F). Operand bytes are the trap, which as a register
is the general register 3F, and as a working register r3 or r15. Every
run seeds registers 3E and 3F with themselves, so @3F points at itself
and the pair 3E names 3E3F, where JP @RR and CALL @RR go. @rr operands
are rr4, pointing at DATA, once as LDC and once as LDE; the long index
forms also run with rr0, the direct address. Each bit operation runs in
both directions, BTJRF/BTJRT taken and not, CPIJE/CPIJNE equal and not.

    P=debugger/z8/tools/cycles_z88.py
    R=debugger/z8/tools/z88-cycles.jsonl.zst
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

WFI = 0x3F
PAIR = 0x3E
# RP0, and RP1 eight above: r0-r15 are 20-2F. Below C0, where indirect
# access would reach set 2, so @r3 and @r15 point at themselves.
RP = 0x20
IP = 0x3000                     # NEXT reads the fill here
R3_RR4 = 0x34                   # r3 and rr4 of an @rr form; +1 for LDE
R3_RR0 = 0x30                   # rr0 makes the long index form direct
# LDC and LDE, with and without index, increment or decrement.
LDX = (0xA7, 0xB7, 0xC3, 0xD3, 0xE2, 0xE3, 0xE7, 0xF2, 0xF3, 0xF7)
BIT_BOTH = (0x07, 0x27, 0x47, 0x67)     # r0,R,#b and R,#b,r0
WORD = WFI << 8 | WFI


def special(opc, mnemo, opr, length):
    after = ORG + length
    if mnemo in ('INCW', 'DECW'):
        return [dict(bytes=[opc, PAIR], target=after)]
    if opr == '@RR':
        return [dict(bytes=[opc, PAIR], target=PAIR << 8 | WFI)]
    if opr == '#IA':
        return [dict(bytes=[opc, PAIR], target=WORD)]
    if mnemo in ('MULT', 'DIV'):         # src, then dst
        return [dict(bytes=[opc, WFI if opr.endswith('#IM') else PAIR, PAIR], target=after)]
    if mnemo == 'LDW':
        code = [opc, PAIR, WFI, WFI] if opr.endswith('#IML') else [opc, PAIR, PAIR]
        return [dict(bytes=code, target=after)]
    if mnemo == 'SRP':
        return [dict(tag=tag, bytes=[opc, RP + low], target=after)
                for tag, low in (('srp', 0), ('srp0', 2), ('srp1', 8 + 1))]
    if mnemo in ('NEXT', 'ENTER', 'EXIT'):
        return [dict(bytes=[opc], target=WORD)]
    if opc in LDX:
        runs = [dict(tag=tag, bytes=[opc, R3_RR4 + low] + [WFI] * (length - 2), target=after)
                for tag, low in (('ldc', 0), ('lde', 1))]
        if length == 4:
            runs += [dict(tag=tag + 'da', bytes=[opc, R3_RR0 + low, WFI, WFI], target=after)
                     for tag, low in (('ldc', 0), ('lde', 1))]
        return runs
    if opc in BIT_BOTH:
        return [dict(tag='d%d' % d, bytes=[opc, PAIR + d, WFI], target=after) for d in (0, 1)]
    if opc in (0x17, 0x57):             # BCP, BITC
        return [dict(bytes=[opc, PAIR, WFI][:length], target=after)]
    if opc == 0x77:
        return [dict(tag=tag, bytes=[opc, PAIR + d], target=after)
                for tag, d in (('bitr', 0), ('bits', 1))]
    if opc == 0x37:                     # bit 7 of r3
        return [dict(tag='%s:r%02x' % ('ft'[t], r3), bytes=[opc, PAIR + t, WFI],
                     regs={'R3': r3},
                     target=_base.rel(after, WFI) if bool(r3) == bool(t) else after)
                for t in (0, 1) for r3 in (0x00, 0x80)]
    if opc in (0xC2, 0xD2):             # r3 against @r3: itself, or r4
        jump_if_equal = opc == 0xC2
        return [dict(tag=tag, bytes=[opc, 0x33, WFI], regs={'R3': r3},
                     target=_base.rel(after, WFI) if equal == jump_if_equal else after)
                for tag, r3, equal in (('eq', RP + 3, True), ('ne', RP + 4, False))]
    return None


# r3 and r15, as pointers, point at themselves; FLAGS keeps IRET from
# taking the fast interrupt return.
# WFI reads the byte after it, then stops the bus.
_base.define(globals(), 'z88', ('Z88C00',), WFI,
             base=[('PC', ORG), ('SP', _base.BASE_SP), ('IP', IP), ('RP', RP),
                   ('FLAGS', 0x00), ('R3', RP + 3), ('R15', RP + 15), ('RR4', DATA)],
             seed=(PAIR, [PAIR, WFI]), special=special, tail=1)
