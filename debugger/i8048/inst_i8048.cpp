#include "inst_i8048.h"

namespace debugger {
namespace i8048 {

namespace {

constexpr uint8_t E(uint8_t len, uint8_t cyc, uint8_t flags) {
    return ((flags - 1) << 4) | (cyc << 2) | (len << 0);
}

constexpr uint8_t inst_len(uint8_t entry) {
    return (entry >> 0) & 3;
}

constexpr uint8_t bus_cyc(uint8_t entry) {
    return (entry >> 2) & 3;
}

constexpr uint8_t inst_flags(uint8_t entry) {
    return ((entry >> 4) & 7) + 1;
}

constexpr uint8_t INST_TABLE[] = {
        E(1, 1, 1),  // 00: NOP   -
        E(1, 1, 2),  // 01: HALT  -
        E(1, 2, 8),  // 02: OUTL  BUS,A
        E(2, 2, 1),  // 03: ADD   A,#n8
        E(2, 2, 1),  // 04: JMP   a11
        E(1, 1, 1),  // 05: EN    I
        0,           // 06:
        E(1, 1, 1),  // 07: DEC   A
        E(1, 2, 8),  // 08: INS   A,BUS
        E(1, 2, 1),  // 09: IN    A,P1
        E(1, 2, 1),  // 0A: IN    A,P2
        0,           // 0B:
        E(1, 2, 1),  // 0C: MOVD  A,P4
        E(1, 2, 1),  // 0D: MOVD  A,P5
        E(1, 2, 1),  // 0E: MOVD  A,P6
        E(1, 2, 1),  // 0F: MOVD  A,P7
        E(1, 1, 1),  // 10: INC   @R0
        E(1, 1, 1),  // 11: INC   @R1
        E(2, 2, 1),  // 12: JB    0,a8
        E(2, 2, 1),  // 13: ADDC  A,#n8
        E(2, 2, 1),  // 14: CALL  a11
        E(1, 1, 1),  // 15: DIS   I
        E(2, 2, 1),  // 16: JTF   a8
        E(1, 1, 1),  // 17: INC   A
        E(1, 1, 1),  // 18: INC   R0
        E(1, 1, 1),  // 19: INC   R1
        E(1, 1, 1),  // 1A: INC   R2
        E(1, 1, 1),  // 1B: INC   R3
        E(1, 1, 1),  // 1C: INC   R4
        E(1, 1, 1),  // 1D: INC   R5
        E(1, 1, 1),  // 1E: INC   R6
        E(1, 1, 1),  // 1F: INC   R7
        E(1, 1, 1),  // 20: XCH   A,@R0
        E(1, 1, 1),  // 21: XCH   A,@R1
        0,           // 22:
        E(2, 2, 1),  // 23: MOV   A,#n8
        E(2, 2, 1),  // 24: JMP   a11
        E(1, 1, 1),  // 25: EN    TCNTI
        E(2, 2, 1),  // 26: JNT0  a8
        E(1, 1, 1),  // 27: CLR   A
        E(1, 1, 1),  // 28: XCH   A,R0
        E(1, 1, 1),  // 29: XCH   A,R1
        E(1, 1, 1),  // 2A: XCH   A,R2
        E(1, 1, 1),  // 2B: XCH   A,R3
        E(1, 1, 1),  // 2C: XCH   A,R4
        E(1, 1, 1),  // 2D: XCH   A,R5
        E(1, 1, 1),  // 2E: XCH   A,R6
        E(1, 1, 1),  // 2F: XCH   A,R7
        E(1, 1, 1),  // 30: XCHD  A,@R0
        E(1, 1, 1),  // 31: XCHD  A,@R1
        E(2, 2, 1),  // 32: JB    1,a8
        0,           // 33:
        E(2, 2, 1),  // 34: CALL  a11
        E(1, 1, 1),  // 35: DIS   TCNTI
        E(2, 2, 1),  // 36: JT0   a8
        E(1, 1, 1),  // 37: CPL   A
        0,           // 38:
        E(1, 2, 1),  // 39: OUTL  P1,A
        E(1, 2, 1),  // 3A: OUTL  P2,A
        0,           // 3B:
        E(1, 2, 1),  // 3C: MOVD  P4,A
        E(1, 2, 1),  // 3D: MOVD  P5,A
        E(1, 2, 1),  // 3E: MOVD  P6,A
        E(1, 2, 1),  // 3F: MOVD  P7,A
        E(1, 1, 1),  // 40: ORL   A,@R0
        E(1, 1, 1),  // 41: ORL   A,@R1
        E(1, 1, 1),  // 42: MOV   A,T
        E(2, 2, 1),  // 43: ORL   A,#n8
        E(2, 2, 1),  // 44: JMP   a11
        E(1, 1, 1),  // 45: STRT  CNT
        E(2, 2, 1),  // 46: JNT1  a8
        E(1, 1, 1),  // 47: SWAP  A
        E(1, 1, 1),  // 48: ORL   A,R0
        E(1, 1, 1),  // 49: ORL   A,R1
        E(1, 1, 1),  // 4A: ORL   A,R2
        E(1, 1, 1),  // 4B: ORL   A,R3
        E(1, 1, 1),  // 4C: ORL   A,R4
        E(1, 1, 1),  // 4D: ORL   A,R5
        E(1, 1, 1),  // 4E: ORL   A,R6
        E(1, 1, 1),  // 4F: ORL   A,R7
        E(1, 1, 1),  // 50: ANL   A,@R0
        E(1, 1, 1),  // 51: ANL   A,@R1
        E(2, 2, 1),  // 52: JB    2,a8
        E(2, 2, 1),  // 53: ANL   A,#n8
        E(2, 2, 1),  // 54: CALL  a11
        E(1, 1, 1),  // 55: STRT  T
        E(2, 2, 1),  // 56: JT1   a8
        E(1, 1, 1),  // 57: DA    A
        E(1, 1, 1),  // 58: ANL   A,R0
        E(1, 1, 1),  // 59: ANL   A,R1
        E(1, 1, 1),  // 5A: ANL   A,R2
        E(1, 1, 1),  // 5B: ANL   A,R3
        E(1, 1, 1),  // 5C: ANL   A,R4
        E(1, 1, 1),  // 5D: ANL   A,R5
        E(1, 1, 1),  // 5E: ANL   A,R6
        E(1, 1, 1),  // 5F: ANL   A,R7
        E(1, 1, 1),  // 60: ADD   A,@R0
        E(1, 1, 1),  // 61: ADD   A,@R1
        E(1, 1, 1),  // 62: MOV   T,A
        E(1, 1, 4),  // 63: MOV   A,P1
        E(2, 2, 1),  // 64: JMP   a11
        E(1, 1, 1),  // 65: STOP  TCNT
        0,           // 66:
        E(1, 1, 1),  // 67: RRC   A
        E(1, 1, 1),  // 68: ADD   A,R0
        E(1, 1, 1),  // 69: ADD   A,R1
        E(1, 1, 1),  // 6A: ADD   A,R2
        E(1, 1, 1),  // 6B: ADD   A,R3
        E(1, 1, 1),  // 6C: ADD   A,R4
        E(1, 1, 1),  // 6D: ADD   A,R5
        E(1, 1, 1),  // 6E: ADD   A,R6
        E(1, 1, 1),  // 6F: ADD   A,R7
        E(1, 1, 1),  // 70: ADDC  A,@R0
        E(1, 1, 1),  // 71: ADDC  A,@R1
        E(2, 2, 1),  // 72: JB    3,a8
        E(1, 1, 4),  // 73: MOV   A,P2
        E(2, 2, 1),  // 74: CALL  a11
        E(1, 1, 1),  // 75: ENT0  CLK
        E(2, 2, 1),  // 76: JF1   a8
        E(1, 1, 1),  // 77: RR    A
        E(1, 1, 1),  // 78: ADDC  A,R0
        E(1, 1, 1),  // 79: ADDC  A,R1
        E(1, 1, 1),  // 7A: ADDC  A,R2
        E(1, 1, 1),  // 7B: ADDC  A,R3
        E(1, 1, 1),  // 7C: ADDC  A,R4
        E(1, 1, 1),  // 7D: ADDC  A,R5
        E(1, 1, 1),  // 7E: ADDC  A,R6
        E(1, 1, 1),  // 7F: ADDC  A,R7
        E(1, 2, 1),  // 80: MOVX  A,@R0
        E(1, 2, 1),  // 81: MOVX  A,@R1
        E(1, 1, 4),  // 82: HLTS  -
        E(1, 2, 1),  // 83: RET   -
        E(2, 2, 1),  // 84: JMP   a11
        E(1, 1, 1),  // 85: CLR   F0
        E(2, 2, 1),  // 86: JNI   a8
        0,           // 87:
        E(2, 2, 8),  // 88: ORL   BUS,#n8
        E(2, 2, 1),  // 89: ORL   P1,#n8
        E(2, 2, 1),  // 8A: ORL   P2,#n8
        0,           // 8B:
        E(1, 2, 1),  // 8C: ORLD  P4,A
        E(1, 2, 1),  // 8D: ORLD  P5,A
        E(1, 2, 1),  // 8E: ORLD  P6,A
        E(1, 2, 1),  // 8F: ORLD  P7,A
        E(1, 2, 1),  // 90: MOVX  @R0,A
        E(1, 2, 1),  // 91: MOVX  @R1,A
        E(2, 2, 1),  // 92: JB    4,a8
        E(1, 2, 1),  // 93: RETR  -
        E(2, 2, 1),  // 94: CALL  a11
        E(1, 1, 1),  // 95: CPL   F0
        E(2, 2, 1),  // 96: JNZ   a8
        E(1, 1, 1),  // 97: CLR   C
        E(2, 2, 8),  // 98: ANL   BUS,#n8
        E(2, 2, 1),  // 99: ANL   P1,#n8
        E(2, 2, 1),  // 9A: ANL   P2,#n8
        0,           // 9B:
        E(1, 2, 1),  // 9C: ANLD  P4,A
        E(1, 2, 1),  // 9D: ANLD  P5,A
        E(1, 2, 1),  // 9E: ANLD  P6,A
        E(1, 2, 1),  // 9F: ANLD  P7,A
        E(1, 1, 1),  // A0: MOV   @R0,A
        E(1, 1, 1),  // A1: MOV   @R1,A
        E(1, 1, 4),  // A2: FLT   -
        E(1, 2, 1),  // A3: MOVP  A,@A
        E(2, 2, 1),  // A4: JMP   a11
        E(1, 1, 1),  // A5: CLR   F1
        0,           // A6:
        E(1, 1, 1),  // A7: CPL   C
        E(1, 1, 1),  // A8: MOV   R0,A
        E(1, 1, 1),  // A9: MOV   R1,A
        E(1, 1, 1),  // AA: MOV   R2,A
        E(1, 1, 1),  // AB: MOV   R3,A
        E(1, 1, 1),  // AC: MOV   R4,A
        E(1, 1, 1),  // AD: MOV   R5,A
        E(1, 1, 1),  // AE: MOV   R6,A
        E(1, 1, 1),  // AF: MOV   R7,A
        E(2, 2, 1),  // B0: MOV   @R0,#n8
        E(2, 2, 1),  // B1: MOV   @R1,#n8
        E(2, 2, 1),  // B2: JB    5,a8
        E(1, 2, 1),  // B3: JMPP  @A
        E(2, 2, 1),  // B4: CALL  a11
        E(1, 1, 1),  // B5: CPL   F1
        E(2, 2, 1),  // B6: JF0   a8
        0,           // B7:
        E(2, 2, 1),  // B8: MOV   R0,#n8
        E(2, 2, 1),  // B9: MOV   R1,#n8
        E(2, 2, 1),  // BA: MOV   R2,#n8
        E(2, 2, 1),  // BB: MOV   R3,#n8
        E(2, 2, 1),  // BC: MOV   R4,#n8
        E(2, 2, 1),  // BD: MOV   R5,#n8
        E(2, 2, 1),  // BE: MOV   R6,#n8
        E(2, 2, 1),  // BF: MOV   R7,#n8
        E(1, 1, 4),  // C0: DEC   @R0
        E(1, 1, 4),  // C1: DEC   @R1
        E(1, 1, 4),  // C2: FLTT  -
        E(1, 2, 4),  // C3: MOVP1 P,@R3
        E(2, 2, 1),  // C4: JMP   a11
        E(1, 1, 1),  // C5: SEL   RB0
        E(2, 2, 1),  // C6: JZ    a8
        E(1, 1, 1),  // C7: MOV   A,PSW
        E(1, 1, 1),  // C8: DEC   R0
        E(1, 1, 1),  // C9: DEC   R1
        E(1, 1, 1),  // CA: DEC   R2
        E(1, 1, 1),  // CB: DEC   R3
        E(1, 1, 1),  // CC: DEC   R4
        E(1, 1, 1),  // CD: DEC   R5
        E(1, 1, 1),  // CE: DEC   R6
        E(1, 1, 1),  // CF: DEC   R7
        E(1, 1, 1),  // D0: XRL   A,@R0
        E(1, 1, 1),  // D1: XRL   A,@R1
        E(2, 2, 1),  // D2: JB    6,a8
        E(2, 2, 1),  // D3: XRL   A,#n8
        E(2, 2, 1),  // D4: CALL  a11
        E(1, 1, 1),  // D5: SEL   RB1
        0,           // D6:
        E(1, 1, 1),  // D7: MOV   PSW,A
        E(1, 1, 1),  // D8: XRL   A,R0
        E(1, 1, 1),  // D9: XRL   A,R1
        E(1, 1, 1),  // DA: XRL   A,R2
        E(1, 1, 1),  // DB: XRL   A,R3
        E(1, 1, 1),  // DC: XRL   A,R4
        E(1, 1, 1),  // DD: XRL   A,R5
        E(1, 1, 1),  // DE: XRL   A,R6
        E(1, 1, 1),  // DF: XRL   A,R7
        E(2, 2, 4),  // E0: DJNZ  @R0,a8
        E(2, 2, 4),  // E1: DJNZ  @R1,a8
        E(1, 1, 4),  // E2: FRES  -
        E(1, 2, 1),  // E3: MOVP3 A,@A
        E(2, 2, 1),  // E4: JMP   a11
        E(1, 1, 1),  // E5: SEL   MB0
        E(2, 2, 1),  // E6: JNC   a8
        E(1, 1, 1),  // E7: RL    A
        E(2, 2, 1),  // E8: DJNZ  R0,a8
        E(2, 2, 1),  // E9: DJNZ  R1,a8
        E(2, 2, 1),  // EA: DJNZ  R2,a8
        E(2, 2, 1),  // EB: DJNZ  R3,a8
        E(2, 2, 1),  // EC: DJNZ  R4,a8
        E(2, 2, 1),  // ED: DJNZ  R5,a8
        E(2, 2, 1),  // EE: DJNZ  R6,a8
        E(2, 2, 1),  // EF: DJNZ  R7,a8
        E(1, 1, 1),  // F0: MOV   A,@R0
        E(1, 1, 1),  // F1: MOV   A,@R1
        E(2, 2, 1),  // F2: JB    7,a8
        E(1, 2, 4),  // F3: MOV   P1,@R3
        E(2, 2, 1),  // F4: CALL  a11
        E(1, 1, 1),  // F5: SEL   MB1
        E(2, 2, 1),  // F6: JC    a8
        E(1, 1, 1),  // F7: RLC   A
        E(1, 1, 1),  // F8: MOV   A,R0
        E(1, 1, 1),  // F9: MOV   A,R1
        E(1, 1, 1),  // FA: MOV   A,R2
        E(1, 1, 1),  // FB: MOV   A,R3
        E(1, 1, 1),  // FC: MOV   A,R4
        E(1, 1, 1),  // FD: MOV   A,R5
        E(1, 1, 1),  // FE: MOV   A,R6
        E(1, 1, 1),  // FF: MOV   A,R7
};
}  // namespace

uint8_t InstI8048::instLength(uint8_t inst) const {
    const auto entry = INST_TABLE[inst];
    if (entry == 0)
        return 0;
    const auto flags = inst_flags(entry);
    if ((flags & _flags) == 0)
        return 0;
    return inst_len(entry);
}

uint8_t InstI8048::busCycles(uint8_t inst) const {
    const auto entry = INST_TABLE[inst];
    if ((inst & ~0x10) == 0x83) {  // RET/RETR
        if (_flags & F_MSM39)
            return 1;
    }
    return bus_cyc(entry);
}

}  // namespace i8048
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
