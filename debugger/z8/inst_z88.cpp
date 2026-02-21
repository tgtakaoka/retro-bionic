#include "inst_z88.h"

namespace debugger {
namespace z88 {

const InstZ88 Inst;

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

constexpr uint8_t bus_cycles(uint8_t e) {
    return inst_len(e) + external_cycles(e)
#if EXTERNAL_STACK == 1
           + stack_cycles(e)
#endif
            ;
}

constexpr uint8_t INST_TABLE[] = {
        E(2, 0, 0),  // 00: DEC    r
        E(2, 0, 0),  // 01: DEC    @r
        E(2, 0, 0),  // 02: ADD    r,r
        E(2, 0, 0),  // 03: ADD    r,@r
        E(3, 0, 0),  // 04: ADD    R,R
        E(3, 0, 0),  // 05: ADD    @R,R
        E(3, 0, 0),  // 06: ADD    R,#IM
        E(3, 0, 0),  // 07: BOR    r0,R,#b
        E(2, 0, 0),  // 08: LD     r0,R
        E(2, 0, 0),  // 09: LD     R,r0
        E(2, 0, 0),  // 0A: DJNZ   r0,RA
        E(2, 0, 0),  // 0B: JR     F,RA
        E(2, 0, 0),  // 0C: LD     r0,#IM
        E(3, 0, 0),  // 0D: JP     F,DA
        E(1, 0, 0),  // 0E: INC    r0
        E(1, 2, 0),  // 0F: NEXT   -
        E(2, 0, 0),  // 10: RLC    R
        E(2, 0, 0),  // 11: RLC    @R
        E(2, 0, 0),  // 12: ADC    r,r
        E(2, 0, 0),  // 13: ADC    r,@r
        E(3, 0, 0),  // 14: ADC    R,R
        E(3, 0, 0),  // 15: ADC    R,@R
        E(3, 0, 0),  // 16: ADC    R,#IM
        E(3, 0, 0),  // 17: BCP    r0,R,#b
        E(2, 0, 0),  // 18: LD     r1,R
        E(2, 0, 0),  // 19: LD     R,r1
        E(2, 0, 0),  // 1A: DJNZ   r1,RA
        E(2, 0, 0),  // 1B: JR     LT,RA
        E(2, 0, 0),  // 1C: LD     r1,#IM
        E(3, 0, 0),  // 1D: JP     LT,DA
        E(1, 0, 0),  // 1E: INC    r1
        E(1, 2, 2),  // 1F: ENTER  -
        E(2, 0, 0),  // 20: INC    R
        E(2, 0, 0),  // 21: INC    @R
        E(2, 0, 0),  // 22: SUB    r,r
        E(2, 0, 0),  // 23: SUB    r,@r
        E(3, 0, 0),  // 24: SUB    R,R
        E(3, 0, 0),  // 25: SUB    R,@R
        E(3, 0, 0),  // 26: SUB    R,#IM
        E(3, 0, 0),  // 27: BXOR   r0,R,#b
        E(2, 0, 0),  // 28: LD     r2,R
        E(2, 0, 0),  // 29: LD     R,r2
        E(2, 0, 0),  // 2A: DJNZ   r2,RA
        E(2, 0, 0),  // 2B: JR     LE,RA
        E(2, 0, 0),  // 2C: LD     r2,#IM
        E(3, 0, 0),  // 2D: JP     LE,DA
        E(1, 0, 0),  // 2E: INC    r2
        E(1, 2, 2),  // 2F: EXIT   -
        E(2, 0, 0),  // 30: JP     @RR
        E(2, 0, 0),  // 31: SRP    #IM
        E(2, 0, 0),  // 32: SBC    r,r
        E(2, 0, 0),  // 33: SBC    r,@r
        E(3, 0, 0),  // 34: SBC    R,R
        E(3, 0, 0),  // 35: SBC    R,@R
        E(3, 0, 0),  // 36: SBC    R,#IM
        E(3, 0, 0),  // 37: BTJRF  RA,r,#b
        E(2, 0, 0),  // 38: LD     r3,R
        E(2, 0, 0),  // 39: LD     R,r3
        E(2, 0, 0),  // 3A: DJNZ   r3,RA
        E(2, 0, 0),  // 3B: JR     ULE,RA
        E(2, 0, 0),  // 3C: LD     r3,#IM
        E(3, 0, 0),  // 3D: JP     ULE,DA
        E(1, 0, 0),  // 3E: INC    r3
        E(1, 0, 0),  // 3F: WFI    -
        E(2, 0, 0),  // 40: DA     R
        E(2, 0, 0),  // 41: DA     @R
        E(2, 0, 0),  // 42: OR     r,r
        E(2, 0, 0),  // 43: OR     r,@r
        E(3, 0, 0),  // 44: OR     R,R
        E(3, 0, 0),  // 45: OR     R,@R
        E(3, 0, 0),  // 46: OR     R,#IM
        E(3, 0, 0),  // 47: LDB    r0,R,#b
        E(2, 0, 0),  // 48: LD     r4,R
        E(2, 0, 0),  // 49: LD     R,r4
        E(2, 0, 0),  // 4A: DJNZ   r4,RA
        E(2, 0, 0),  // 4B: JR     OV,RA
        E(2, 0, 0),  // 4C: LD     r4,#IM
        E(3, 0, 0),  // 4D: JP     OV,DA
        E(1, 0, 0),  // 4E: INC    r4
        E(1, 0, 0),  // 4F: SB0    -
        E(2, 0, 1),  // 50: POP    R
        E(2, 0, 1),  // 51: POP    @R
        E(2, 0, 0),  // 52: AND    r,r
        E(2, 0, 0),  // 53: AND    r,@r
        E(3, 0, 0),  // 54: AND    R,R
        E(3, 0, 0),  // 55: AND    R,@R
        E(3, 0, 0),  // 56: AND    R,#IM
        E(2, 0, 0),  // 57: BITC   r,#b
        E(2, 0, 0),  // 58: LD     r5,R
        E(2, 0, 0),  // 59: LD     R,r5
        E(2, 0, 0),  // 5A: DJNZ   r5,RA
        E(2, 0, 0),  // 5B: JR     MI,RA
        E(2, 0, 0),  // 5C: LD     r5,#IM
        E(3, 0, 0),  // 5D: JP     MI,DA
        E(1, 0, 0),  // 5E: INC    r5
        E(1, 0, 0),  // 5F: SB1    -
        E(2, 0, 0),  // 60: COM    R
        E(2, 0, 0),  // 61: COM    @R
        E(2, 0, 0),  // 62: TCM    r,r
        E(2, 0, 0),  // 63: TCM    r,@r
        E(3, 0, 0),  // 64: TCM    R,R
        E(3, 0, 0),  // 65: TCM    R,@R
        E(3, 0, 0),  // 66: TCM    R,#IM
        E(3, 0, 0),  // 67: BAND   r0,R,#b
        E(2, 0, 0),  // 68: LD     r6,R
        E(2, 0, 0),  // 69: LD     R,r6
        E(2, 0, 0),  // 6A: DJNZ   r6,RA
        E(2, 0, 0),  // 6B: JR     Z,RA
        E(2, 0, 0),  // 6C: LD     r6,#IM
        E(3, 0, 0),  // 6D: JP     Z,DA
        E(1, 0, 0),  // 6E: INC    r6
        E(0, 0, 0),  // 6F: -      -
        E(2, 0, 1),  // 70: PUSH   R
        E(2, 0, 1),  // 71: PUSH   @R
        E(2, 0, 0),  // 72: TM     r,r
        E(2, 0, 0),  // 73: TM     r,@r
        E(3, 0, 0),  // 74: TM     R,R
        E(3, 0, 0),  // 75: TM     R,@R
        E(3, 0, 0),  // 76: TM     R,#IM
        E(2, 0, 0),  // 77: BITR   r,#B
        E(2, 0, 0),  // 78: LD     r7,R
        E(2, 0, 0),  // 79: LD     R,r7
        E(2, 0, 0),  // 7A: DJNZ   r7,RA
        E(2, 0, 0),  // 7B: JR     C,RA
        E(2, 0, 0),  // 7C: LD     r7,#IM
        E(3, 0, 0),  // 7D: JP     C,DA
        E(1, 0, 0),  // 7E: INC    r7
        0,           // 7F:
        E(2, 0, 0),  // 80: DECW   RR
        E(2, 0, 0),  // 81: DECW   @R
        E(3, 0, 0),  // 82: PUSHUD @R,R
        E(3, 0, 0),  // 83: PUSHUI @R,R
        E(3, 0, 0),  // 84: MULT   RR,R
        E(3, 0, 0),  // 85: MULT   RR,@IR
        E(3, 0, 0),  // 86: MULT   RR,#IM
        E(3, 0, 0),  // 87: LD     r,x(r)
        E(2, 0, 0),  // 88: LD     r8,R
        E(2, 0, 0),  // 89: LD     R,r8
        E(2, 0, 0),  // 8A: DJNZ   r8,RA
        E(2, 0, 0),  // 8B: JR     RA
        E(2, 0, 0),  // 8C: LD     r8,#IM
        E(3, 0, 0),  // 8D: JP     DA
        E(1, 0, 0),  // 8E: INC    r8
        E(1, 0, 0),  // 8F: DI     -
        E(2, 0, 0),  // 90: RL     R
        E(2, 0, 0),  // 91: RL     @R
        E(3, 0, 0),  // 92: POPUD  R,@R
        E(3, 0, 0),  // 93: POPUI  R,@R
        E(3, 0, 0),  // 94: DIV    RR,R
        E(3, 0, 0),  // 95: DIV    RR,@R
        E(3, 0, 0),  // 96: DIV    RR,#IM
        E(3, 0, 0),  // 97: LD     x(r),r
        E(2, 0, 0),  // 98: LD     r9,R
        E(2, 0, 0),  // 99: LD     R,r9
        E(2, 0, 0),  // 9A: DJNZ   r9,RA
        E(2, 0, 0),  // 9B: JR     GE,RA
        E(2, 0, 0),  // 9C: LD     r9,#IM
        E(3, 0, 0),  // 9D: JP     GE,DA
        E(1, 0, 0),  // 9E: INC    r9
        E(1, 0, 0),  // 9F: EI     -
        E(2, 0, 0),  // A0: INCW   R
        E(2, 0, 0),  // A1: INCW   @R
        E(2, 0, 0),  // A2: CP     r,r
        E(2, 0, 0),  // A3: CP     r,@r
        E(3, 0, 0),  // A4: CP     R,R
        E(3, 0, 0),  // A5: CP     @R,R
        E(3, 0, 0),  // A6: CP     R,#IM
        E(4, 1, 0),  // A7: LDE    r,xl(rr)
        E(2, 0, 0),  // A8: LD     r10,R
        E(2, 0, 0),  // A9: LD     R,r10
        E(2, 0, 0),  // AA: DJNZ   r10,RA
        E(2, 0, 0),  // AB: JR     GT,RA
        E(2, 0, 0),  // AC: LD     r10,#IM
        E(3, 0, 0),  // AD: JP     GT,DA
        E(1, 0, 0),  // AE: INC    r10
        E(1, 1, 2),  // AF: RET    -
        E(2, 0, 0),  // B0: CLR    R
        E(2, 0, 0),  // B1: CLR    @R
        E(2, 0, 0),  // B2: XOR    r,r
        E(2, 0, 0),  // B3: XOR    r,@r
        E(3, 0, 0),  // B4: XOR    R,R
        E(3, 0, 0),  // B5: XOR    R,@R
        E(3, 0, 0),  // B6: XOR    R,#IM
        E(4, 1, 0),  // B7: LDE    xl(rr),r
        E(2, 0, 0),  // B8: LD     r11,R
        E(2, 0, 0),  // B9: LD     R,r11
        E(2, 0, 0),  // BA: DJNZ   r11,RA
        E(2, 0, 0),  // BB: JR     UGT,RA
        E(2, 0, 0),  // BC: LD     r11,#IM
        E(3, 0, 0),  // BD: JP     UGT,DA
        E(1, 0, 0),  // BE: INC    r11
        E(1, 1, 3),  // BF: IRET   -
        E(2, 0, 0),  // C0: RRC    R
        E(2, 0, 0),  // C1: RRC    @R
        E(3, 0, 0),  // C2: CPIJE  r,r,RA
        E(2, 1, 0),  // C3: LDE    r,@rr
        E(3, 0, 0),  // C4: LDW    RR,RR
        E(3, 0, 0),  // C5: LDW    RR,@R
        E(4, 0, 0),  // C6: LDW    RR,#IML
        E(2, 0, 0),  // C7: LD     r,@r
        E(2, 0, 0),  // C8: LD     r12,R
        E(2, 0, 0),  // C9: LD     R,r12
        E(2, 0, 0),  // CA: DJNZ   r12,RA
        E(2, 0, 0),  // CB: JR     NOV,RA
        E(2, 0, 0),  // CC: LD     r12,#IM
        E(3, 0, 0),  // CD: JP     NOV,DA
        E(1, 0, 0),  // CE: INC    r12
        E(1, 0, 0),  // CF: RCF    -
        E(2, 0, 0),  // D0: SRA    R
        E(2, 0, 0),  // D1: SRA    @R
        E(3, 0, 0),  // D2: CPIJNE r,r,RA
        E(2, 1, 0),  // D3: LDE    @rr,r
        E(2, 2, 2),  // D4: CALL   #IA
        0,           // D5:
        E(3, 0, 0),  // D6: LD     @R,#IM
        E(2, 0, 0),  // D7: LD     @r,r
        E(2, 0, 0),  // D8: LD     r13,R
        E(2, 0, 0),  // D9: LD     R,r13
        E(2, 0, 0),  // DA: DJNZ   r13,RA
        E(2, 0, 0),  // DB: JR     PL,RA
        E(2, 0, 0),  // DC: LD     r13,#IM
        E(3, 0, 0),  // DD: JP     PL,DA
        E(1, 0, 0),  // DE: INC    r13
        E(1, 0, 0),  // DF: SCF    -
        E(2, 0, 0),  // E0: RR     R
        E(2, 0, 0),  // E1: RR     @R
        E(2, 1, 0),  // E2: LDED   r,@rr
        E(2, 1, 0),  // E3: LDEI   r,@rr
        E(3, 0, 0),  // E4: LD     R,R
        E(3, 0, 0),  // E5: LD     R,@R
        E(3, 0, 0),  // E6: LD     R,#IM
        E(3, 1, 0),  // E7: LDE    r,xs(rr)
        E(2, 0, 0),  // E8: LD     r14,R
        E(2, 0, 0),  // E9: LD     R,r14
        E(2, 0, 0),  // EA: DJNZ   r14,RA
        E(2, 0, 0),  // EB: JR     NZ,RA
        E(2, 0, 0),  // EC: LD     r14,#IM
        E(3, 0, 0),  // ED: JP     NZ,DA
        E(1, 0, 0),  // EE: INC    r14
        E(1, 0, 0),  // EF: CCF    -
        E(2, 0, 0),  // F0: SWAP   R
        E(2, 0, 0),  // F1: SWAP   @R
        E(2, 1, 0),  // F2: LDEPD  @rr,r
        E(2, 1, 0),  // F3: LDEPI  @rr,r
        E(2, 0, 2),  // F4: CALL   @RR
        E(3, 0, 0),  // F5: LD     @R,R
        E(3, 0, 2),  // F6: CALL   DA
        E(3, 1, 0),  // F7: LDE    xs(rr),r
        E(2, 0, 0),  // F8: LD     r15,R
        E(2, 0, 0),  // F9: LD     R,r15
        E(2, 0, 0),  // FA: DJNZ   r15,RA
        E(2, 0, 0),  // FB: JR     NC,RA
        E(2, 0, 0),  // FC: LD     r15,#IM
        E(3, 0, 0),  // FD: JP     NC,DA
        E(1, 0, 0),  // FE: INC    r15
        E(1, 0, 0),  // FF: NOP    -
};
}  // namespace

uint8_t InstZ88::instLen(uint8_t inst) const {
    return inst_len(INST_TABLE[inst]);
}

uint8_t InstZ88::busCycles(uint8_t inst) const {
    return bus_cycles(INST_TABLE[inst]);
}

bool InstZ88::writeOnly(uint8_t rp0, uint8_t rp1, uint8_t num) {
    const auto addr = (num < 8) ? rp0 + num : rp1 + num;
    if (addr == 0xEF)
        return true;
    // F1, F4, F5, F8, F9, FA, FB
    if ((addr & 0xF0) == 0xF0)
        return (1 << (addr & 0xF)) & 0x0F32;
    return false;
}

}  // namespace z88
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
