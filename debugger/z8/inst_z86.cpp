#include "inst_z86.h"

namespace debugger {
namespace z86 {

const InstZ86 Inst;

namespace {

constexpr uint8_t E(
        uint8_t inst_len, uint8_t external_cycles, uint8_t stack_cycles) {
    return static_cast<uint8_t>(
            inst_len | (external_cycles << 4) | (stack_cycles << 6));
}

constexpr uint8_t inst_len(uint8_t e) {
    return e & 7;
}

constexpr uint8_t external_cycles(uint8_t e) {
    return (e >> 4) & 3;
}

#if EXTERNAL_STACK == 1
constexpr uint8_t stack_cycles(uint8_t e) {
    return (e >> 6) & 3;
}
#endif

static constexpr uint8_t bus_cycles(uint8_t e) {
    return inst_len(e) + external_cycles(e)
#if EXTERNAL_STACK == 1
           + stack_cycles(e)
#endif
            ;
}

constexpr uint8_t INST_TABLE[] = {
        E(2, 0, 0),  // 00: DEC  R
        E(2, 0, 0),  // 01: DEC  IR
        E(2, 0, 0),  // 02: ADD  r,r
        E(2, 0, 0),  // 03: ADD  r,Ir
        E(3, 0, 0),  // 04: ADD  R,R
        E(3, 0, 0),  // 05: ADD  R.IR
        E(3, 0, 0),  // 06: ADD  R,IM
        E(3, 0, 0),  // 07: ADD  IR,IM
        E(2, 0, 0),  // 08: LD   r0,R
        E(2, 0, 0),  // 09: LD   R,r0
        E(2, 0, 0),  // 0A: DJNZ r0,RA
        E(2, 0, 0),  // 0B: JR   F,RA
        E(2, 0, 0),  // 0C: LD   r0,IM
        E(3, 0, 0),  // 0D: JP   F,DA
        E(1, 0, 0),  // 0E: INC  r0
        0,           // 0F:
        E(2, 0, 0),  // 10: RLC  R
        E(2, 0, 0),  // 11: RLC  IR
        E(2, 0, 0),  // 12: ADC  r,r
        E(2, 0, 0),  // 13: ADC  r,Ir
        E(3, 0, 0),  // 14: ADC  R,R
        E(3, 0, 0),  // 15: ADC  R.IR
        E(3, 0, 0),  // 16: ADC  R,IM
        E(3, 0, 0),  // 17: ADC  IR,IM
        E(2, 0, 0),  // 18: LD   r1,R
        E(2, 0, 0),  // 19: LD   R,r1
        E(2, 0, 0),  // 1A: DJNZ r1,RA
        E(2, 0, 0),  // 1B: JR   LT,RA
        E(2, 0, 0),  // 1C: LD   r1,IM
        E(3, 0, 0),  // 1D: JP   LT,DA
        E(1, 0, 0),  // 1E: INC  r1
        0,           // 1F:
        E(2, 0, 0),  // 20: INC  R
        E(2, 0, 0),  // 21: INC  IR
        E(2, 0, 0),  // 22: SUB  r,r
        E(2, 0, 0),  // 23: SUB  r,Ir
        E(3, 0, 0),  // 24: SUB  R,R
        E(3, 0, 0),  // 25: SUB  R.IR
        E(3, 0, 0),  // 26: SUB  R,IM
        E(3, 0, 0),  // 27: SUB  IR,IM
        E(2, 0, 0),  // 28: LD   r2,R
        E(2, 0, 0),  // 29: LD   R,r2
        E(2, 0, 0),  // 2A: DJNZ r2,RA
        E(2, 0, 0),  // 2B: JR   LE,RA
        E(2, 0, 0),  // 2C: LD   r2,IM
        E(3, 0, 0),  // 2D: JP   LE,DA
        E(1, 0, 0),  // 2E: INC  r2
        0,           // 2F:
        E(2, 0, 0),  // 30: JP   IRR
        E(2, 0, 0),  // 31: SRP  IM
        E(2, 0, 0),  // 32: SBC  r,r
        E(2, 0, 0),  // 33: SBC  r,Ir
        E(3, 0, 0),  // 34: SBC  R,R
        E(3, 0, 0),  // 35: SBC  R.IR
        E(3, 0, 0),  // 36: SBC  R,IM
        E(3, 0, 0),  // 37: SBC  IR,IM
        E(2, 0, 0),  // 38: LD   r3,R
        E(2, 0, 0),  // 39: LD   R,r3
        E(2, 0, 0),  // 3A: DJNZ r3,RA
        E(2, 0, 0),  // 3B: JR   ULE,RA
        E(2, 0, 0),  // 3C: LD   r3,IM
        E(3, 0, 0),  // 3D: JP   ULE,DA
        E(1, 0, 0),  // 3E: INC  r3
        E(0, 0, 0),  // 3F: -    -
        E(2, 0, 0),  // 40: DAA  R
        E(2, 0, 0),  // 41: DAA  IR
        E(2, 0, 0),  // 42: OR   r,r
        E(2, 0, 0),  // 43: OR   r,Ir
        E(3, 0, 0),  // 44: OR   R,R
        E(3, 0, 0),  // 45: OR   R.IR
        E(3, 0, 0),  // 46: OR   R,IM
        E(3, 0, 0),  // 47: OR   IR,IM
        E(2, 0, 0),  // 48: LD   r4,R
        E(2, 0, 0),  // 49: LD   R,r4
        E(2, 0, 0),  // 4A: DJNZ r4,RA
        E(2, 0, 0),  // 4B: JR   OV,RA
        E(2, 0, 0),  // 4C: LD   r4,IM
        E(3, 0, 0),  // 4D: JP   OV,DA
        E(1, 0, 0),  // 4E: INC  r4
        0,           // 4F:
        E(2, 0, 1),  // 50: POP  R
        E(2, 0, 1),  // 51: POP  IR
        E(2, 0, 0),  // 52: AND  r,r
        E(2, 0, 0),  // 53: AND  r,Ir
        E(3, 0, 0),  // 54: AND  R,R
        E(3, 0, 0),  // 55: AND  R.IR
        E(3, 0, 0),  // 56: AND  R,IM
        E(3, 0, 0),  // 57: AND  IR,IM
        E(2, 0, 0),  // 58: LD   r5,R
        E(2, 0, 0),  // 59: LD   R,r5
        E(2, 0, 0),  // 5A: DJNZ r5,RA
        E(2, 0, 0),  // 5B: JR   MI,RA
        E(2, 0, 0),  // 5C: LD   r5,IM
        E(3, 0, 0),  // 5D: JP   MI,DA
        E(1, 0, 0),  // 5E: INC  r5
        0,           // 5F:
        E(2, 0, 0),  // 60: COM  R
        E(2, 0, 0),  // 61: COM  IR
        E(2, 0, 0),  // 62: TCM  r,r
        E(2, 0, 0),  // 63: TCM  r,Ir
        E(3, 0, 0),  // 64: TCM  R,R
        E(3, 0, 0),  // 65: TCM  R.IR
        E(3, 0, 0),  // 66: TCM  R,IM
        E(3, 0, 0),  // 67: TCM  IR,IM
        E(2, 0, 0),  // 68: LD   r6,R
        E(2, 0, 0),  // 69: LD   R,r6
        E(2, 0, 0),  // 6A: DJNZ r6,RA
        E(2, 0, 0),  // 6B: JR   Z,RA
        E(2, 0, 0),  // 6C: LD   r6,IM
        E(3, 0, 0),  // 6D: JP   Z,DA
        E(1, 0, 0),  // 6E: INC  r6
        E(1, 0, 0),  // 6F: STOP -
        E(2, 0, 1),  // 70: PUSH R
        E(2, 0, 1),  // 71: PUSH IR
        E(2, 0, 0),  // 72: TM   r,r
        E(2, 0, 0),  // 73: TM   r,Ir
        E(3, 0, 0),  // 74: TM   R,R
        E(3, 0, 0),  // 75: TM   R.IR
        E(3, 0, 0),  // 76: TM   R,IM
        E(3, 0, 0),  // 77: TM   IR,IM
        E(2, 0, 0),  // 78: LD   r7,R
        E(2, 0, 0),  // 79: LD   R,r7
        E(2, 0, 0),  // 7A: DJNZ r7,RA
        E(2, 0, 0),  // 7B: JR   C,RA
        E(2, 0, 0),  // 7C: LD   r7,IM
        E(3, 0, 0),  // 7D: JP   C,DA
        E(1, 0, 0),  // 7E: INC  r7
        E(1, 0, 0),  // 7F: HALT -
        E(2, 0, 0),  // 80: DECW RR
        E(2, 0, 0),  // 81: DECW IR
        E(2, 1, 0),  // 82: LDE  r,Irr
        E(2, 1, 0),  // 83: LDEI Ir,Irr
        0,           // 84:
        0,           // 85:
        0,           // 86:
        0,           // 87:
        E(2, 0, 0),  // 88: LD   r8,R
        E(2, 0, 0),  // 89: LD   R,r8
        E(2, 0, 0),  // 8A: DJNZ r8,RA
        E(2, 0, 0),  // 8B: JR   RA
        E(2, 0, 0),  // 8C: LD   r8,IM
        E(3, 0, 0),  // 8D: JP   DA
        E(1, 0, 0),  // 8E: INC  r8
        E(1, 0, 0),  // 8F: DI   -
        E(2, 0, 0),  // 90: RL   R
        E(2, 0, 0),  // 91: RL   IR
        E(2, 1, 0),  // 92: LDE  Irr,r
        E(2, 1, 0),  // 93: LDEI Irr,Ir
        0,           // 94:
        0,           // 95:
        0,           // 96:
        0,           // 97:
        E(2, 0, 0),  // 98: LD   r9,R
        E(2, 0, 0),  // 99: LD   R,r9
        E(2, 0, 0),  // 9A: DJNZ r9,RA
        E(2, 0, 0),  // 9B: JR   GE,RA
        E(2, 0, 0),  // 9C: LD   r9,IM
        E(3, 0, 0),  // 9D: JP   GE,DA
        E(1, 0, 0),  // 9E: INC  r9
        E(1, 0, 0),  // 9F: EI   -
        E(2, 0, 0),  // A0: INCW RR
        E(2, 0, 0),  // A1: INCW IR
        E(2, 0, 0),  // A2: CP   r,r
        E(2, 0, 0),  // A3: CP   r,Ir
        E(3, 0, 0),  // A4: CP   R,R
        E(3, 0, 0),  // A5: CP   R.IR
        E(3, 0, 0),  // A6: CP   R,IM
        E(3, 0, 0),  // A7: CP   IR,IM
        E(2, 0, 0),  // A8: LD   r10,R
        E(2, 0, 0),  // A9: LD   R,r10
        E(2, 0, 0),  // AA: DJNZ r10,RA
        E(2, 0, 0),  // AB: JR   GT,RA
        E(2, 0, 0),  // AC: LD   r10,IM
        E(3, 0, 0),  // AD: JP   GT,DA
        E(1, 0, 0),  // AE: INC  r10
        E(1, 1, 2),  // AF: RET  -
        E(2, 0, 0),  // B0: CLR  R
        E(2, 0, 0),  // B1: CLR  IR
        E(2, 0, 0),  // B2: XOR  r,r
        E(2, 0, 0),  // B3: XOR  r,Ir
        E(3, 0, 0),  // B4: XOR  R,R
        E(3, 0, 0),  // B5: XOR  R.IR
        E(3, 0, 0),  // B6: XOR  R,IM
        E(3, 0, 0),  // B7: XOR  IR,IM
        E(2, 0, 0),  // B8: LD   r11,R
        E(2, 0, 0),  // B9: LD   R,r11
        E(2, 0, 0),  // BA: DJNZ r11,RA
        E(2, 0, 0),  // BB: JR   UGT,RA
        E(2, 0, 0),  // BC: LD   r11,IM
        E(3, 0, 0),  // BD: JP   UGT,DA
        E(1, 0, 0),  // BE: INC  r11
        E(1, 1, 3),  // BF: IRET -
        E(2, 0, 0),  // C0: RRC  R
        E(2, 0, 0),  // C1: RRC  IR
        E(2, 1, 0),  // C2: LDC  r,Irr
        E(2, 1, 0),  // C3: LDCI Ir,Irr
        0,           // C4:
        0,           // C5:
        0,           // C6:
        E(3, 0, 0),  // C7: LD   r,X
        E(2, 0, 0),  // C8: LD   r12,R
        E(2, 0, 0),  // C9: LD   R,r12
        E(2, 0, 0),  // CA: DJNZ r12,RA
        E(2, 0, 0),  // CB: JR   NOV,RA
        E(2, 0, 0),  // CC: LD   r12,IM
        E(3, 0, 0),  // CD: JP   NOV,DA
        E(1, 0, 0),  // CE: INC  r12
        E(1, 0, 0),  // CF: RCF  -
        E(2, 0, 0),  // D0: SRA  R
        E(2, 0, 0),  // D1: SRA  IR
        E(2, 1, 0),  // D2: LDC  Irr,r
        E(2, 1, 0),  // D3: LDCI Irr,Ir
        E(2, 1, 2),  // D4: CALL IRR
        E(0, 0, 0),  // D5: -    -
        E(3, 0, 2),  // D6: CALL DA
        E(3, 0, 0),  // D7: LD   X,r
        E(2, 0, 0),  // D8: LD   r13,R
        E(2, 0, 0),  // D9: LD   R,r13
        E(2, 0, 0),  // DA: DJNZ r13,RA
        E(2, 0, 0),  // DB: JR   PL,RA
        E(2, 0, 0),  // DC: LD   r13,IM
        E(3, 0, 0),  // DD: JP   PL,DA
        E(1, 0, 0),  // DE: INC  r13
        E(1, 0, 0),  // DF: SCF  -
        E(2, 0, 0),  // E0: RR   R
        E(2, 0, 0),  // E1: RR   IR
        0,           // E2:
        E(2, 0, 0),  // E3: LD   r,Ir
        E(3, 0, 0),  // E4: LD   R,R
        E(3, 0, 0),  // E5: LD   R,IR
        E(3, 0, 0),  // E6: LD   R,IM
        E(3, 0, 0),  // E7: LD   IR,IM
        E(2, 0, 0),  // E8: LD   r14,R
        E(2, 0, 0),  // E9: LD   R,r14
        E(2, 0, 0),  // EA: DJNZ r14,RA
        E(2, 0, 0),  // EB: JR   NE,RA
        E(2, 0, 0),  // EC: LD   r14,IM
        E(3, 0, 0),  // ED: JP   NE,DA
        E(1, 0, 0),  // EE: INC  r14
        E(1, 0, 0),  // EF: CCF  -
        E(2, 0, 0),  // F0: SWAP R
        E(2, 0, 0),  // F1: SWAP IR
        0,           // F2:
        E(2, 0, 0),  // F3: LD   Ir,r
        0,           // F4:
        E(3, 0, 0),  // F5: LD   IR,R
        0,           // F6:
        0,           // F7:
        E(2, 0, 0),  // F8: LD   r15,R
        E(2, 0, 0),  // F9: LD   R,rr5
        E(2, 0, 0),  // FA: DJNZ r15,RA
        E(2, 0, 0),  // FB: JR   NC,RA
        E(2, 0, 0),  // FC: LD   r15,IM
        E(3, 0, 0),  // FD: JP   NC,DA
        E(1, 0, 0),  // FE: INC  r15
        E(1, 0, 0),  // FF: NOP  -
};
}  // namespace

uint8_t InstZ86::instLen(uint8_t inst) const {
    return inst_len(INST_TABLE[inst]);
}

uint8_t InstZ86::busCycles(uint8_t inst) const {
    return bus_cycles(INST_TABLE[inst]);
}

bool InstZ86::writeOnly(uint8_t rp, uint8_t num) {
    // 00, 01, 02, 03
    if (rp == 0x00 && num < 4)
        return true;
    // F0, F3, F4, F5, F6, F7, F8, F9
    if (rp == 0xF0) {
        return (1 << num) & 0x03E9;
    }
    return false;
}

}  // namespace z86
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
