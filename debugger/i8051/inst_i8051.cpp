#include "inst_i8051.h"

namespace debugger {
namespace i8051 {

namespace {

constexpr uint8_t E(uint8_t len, uint8_t bus) {
    return (len << 4) | (bus << 0);
}

constexpr uint8_t bus_cyc(uint8_t entry) {
    return (entry >> 0) & 0xF;
}

constexpr uint8_t INST_TABLE[] = {
        E(1, 2),  // 00: NOP   0
        E(2, 4),  // 01: AJMP  a11
        E(3, 4),  // 02: LJMP  a16
        E(1, 2),  // 03: RR    A
        E(1, 2),  // 04: INC   A
        E(2, 2),  // 05: INC   a8
        E(1, 2),  // 06: INC   @R0
        E(1, 2),  // 07: INC   @R1
        E(1, 2),  // 08: INC   R0
        E(1, 2),  // 09: INC   R1
        E(1, 2),  // 0A: INC   R2
        E(1, 2),  // 0B: INC   R3
        E(1, 2),  // 0C: INC   R4
        E(1, 2),  // 0D: INC   R5
        E(1, 2),  // 0E: INC   R6
        E(1, 2),  // 0F: INC   R7
        E(3, 4),  // 10: JBC   b8,r8
        E(2, 4),  // 11: ACALL a11
        E(3, 4),  // 12: LCALL a16
        E(1, 2),  // 13: RRC   A
        E(1, 2),  // 14: DEC   A
        E(2, 2),  // 15: DEC   a8
        E(1, 2),  // 16: DEC   @R0
        E(1, 2),  // 17: DEC   @R1
        E(1, 2),  // 18: DEC   R0
        E(1, 2),  // 19: DEC   R1
        E(1, 2),  // 1A: DEC   R2
        E(1, 2),  // 1B: DEC   R3
        E(1, 2),  // 1C: DEC   R4
        E(1, 2),  // 1D: DEC   R5
        E(1, 2),  // 1E: DEC   R6
        E(1, 2),  // 1F: DEC   R7
        E(3, 4),  // 20: JB    b8,r8
        E(2, 4),  // 21: AJMP  a11
        E(1, 4),  // 22: RET   -
        E(1, 2),  // 23: RL    A
        E(2, 2),  // 24: ADD   A,#n8
        E(2, 2),  // 25: ADD   A,a8
        E(1, 2),  // 26: ADD   A,@R0
        E(1, 2),  // 27: ADD   A,@R1
        E(1, 2),  // 28: ADD   A,R0
        E(1, 2),  // 29: ADD   A,R1
        E(1, 2),  // 2A: ADD   A,R2
        E(1, 2),  // 2B: ADD   A,R3
        E(1, 2),  // 2C: ADD   A,R4
        E(1, 2),  // 2D: ADD   A,R5
        E(1, 2),  // 2E: ADD   A,R6
        E(1, 2),  // 2F: ADD   A,R7
        E(3, 4),  // 30: JNB   b8,r8
        E(2, 4),  // 31: ACALL a11
        E(1, 4),  // 32: RETI  -
        E(1, 2),  // 33: RLC   A
        E(2, 2),  // 34: ADDC  A,#n8
        E(2, 2),  // 35: ADDC  A,a8
        E(1, 2),  // 36: ADDC  A,@R0
        E(1, 2),  // 37: ADDC  A,@R1
        E(1, 2),  // 38: ADDC  A,R0
        E(1, 2),  // 39: ADDC  A,R1
        E(1, 2),  // 3A: ADDC  A,R2
        E(1, 2),  // 3B: ADDC  A,R3
        E(1, 2),  // 3C: ADDC  A,R4
        E(1, 2),  // 3D: ADDC  A,R5
        E(1, 2),  // 3E: ADDC  A,R6
        E(1, 2),  // 3F: ADDC  A,R7
        E(2, 4),  // 40: JC    r8
        E(2, 4),  // 41: AJMP  a11
        E(2, 2),  // 42: ORL   a8,A
        E(3, 4),  // 43: ORL   a8,#n8
        E(2, 2),  // 44: ORL   A,#n8
        E(2, 2),  // 45: ORL   A,a8
        E(1, 2),  // 46: ORL   A,@R0
        E(1, 2),  // 47: ORL   A,@R1
        E(1, 2),  // 48: ORL   A,R0
        E(1, 2),  // 49: ORL   A,R1
        E(1, 2),  // 4A: ORL   A,R2
        E(1, 2),  // 4B: ORL   A,R3
        E(1, 2),  // 4C: ORL   A,R4
        E(1, 2),  // 4D: ORL   A,R5
        E(1, 2),  // 4E: ORL   A,R6
        E(1, 2),  // 4F: ORL   A,R7
        E(2, 4),  // 50: JNC   r8
        E(2, 4),  // 51: ACALL a11
        E(2, 2),  // 52: ANL   a8,A
        E(3, 4),  // 53: ANL   a8,#n8
        E(2, 2),  // 54: ANL   A,#n8
        E(2, 2),  // 55: ANL   A,a8
        E(1, 2),  // 56: ANL   A,@R0
        E(1, 2),  // 57: ANL   A,@R1
        E(1, 2),  // 58: ANL   A,R0
        E(1, 2),  // 59: ANL   A,R1
        E(1, 2),  // 5A: ANL   A,R2
        E(1, 2),  // 5B: ANL   A,R3
        E(1, 2),  // 5C: ANL   A,R4
        E(1, 2),  // 5D: ANL   A,R5
        E(1, 2),  // 5E: ANL   A,R6
        E(1, 2),  // 5F: ANL   A,R7
        E(2, 4),  // 60: JZ    r8
        E(2, 4),  // 61: AJMP  a11
        E(2, 2),  // 62: XRL   a8,A
        E(3, 4),  // 63: XRL   a8,#n8
        E(2, 2),  // 64: XRL   A,#n8
        E(2, 2),  // 65: XRL   A,a8
        E(1, 2),  // 66: XRL   A,@R0
        E(1, 2),  // 67: XRL   A,@R1
        E(1, 2),  // 68: XRL   A,R0
        E(1, 2),  // 69: XRL   A,R1
        E(1, 2),  // 6A: XRL   A,R2
        E(1, 2),  // 6B: XRL   A,R3
        E(1, 2),  // 6C: XRL   A,R4
        E(1, 2),  // 6D: XRL   A,R5
        E(1, 2),  // 6E: XRL   A,R6
        E(1, 2),  // 6F: XRL   A,R7
        E(2, 4),  // 70: JNZ   r8
        E(2, 4),  // 71: ACALL a11
        E(2, 4),  // 72: ORL   C,b8
        E(1, 4),  // 73: JMP   @A+DPTR
        E(2, 2),  // 74: MOV   A,#n8
        E(3, 4),  // 75: MOV   a8,#n8
        E(2, 2),  // 76: MOV   @R0,#n8
        E(2, 2),  // 77: MOV   @R1,#n8
        E(2, 2),  // 78: MOV   R0,#n8
        E(2, 2),  // 79: MOV   R1,#n8
        E(2, 2),  // 7A: MOV   R2,#n8
        E(2, 2),  // 7B: MOV   R3,#n8
        E(2, 2),  // 7C: MOV   R4,#n8
        E(2, 2),  // 7D: MOV   R5,#n8
        E(2, 2),  // 7E: MOV   R6,#n8
        E(2, 2),  // 7F: MOV   R7,#n8
        E(2, 4),  // 80: SJMP  r8
        E(2, 4),  // 81: AJMP  a11
        E(2, 4),  // 82: ANL   C,b8
        E(1, 4),  // 83: MOVC  A,@A+PC
        E(1, 8),  // 84: DIV   AB
        E(3, 4),  // 85: MOV   a8,a8
        E(2, 4),  // 86: MOV   a8,@R0
        E(2, 4),  // 87: MOV   a8,@R1
        E(2, 4),  // 88: MOV   a8,R0
        E(2, 4),  // 89: MOV   a8,R1
        E(2, 4),  // 8A: MOV   a8,R2
        E(2, 4),  // 8B: MOV   a8,R3
        E(2, 4),  // 8C: MOV   a8,R4
        E(2, 4),  // 8D: MOV   a8,R5
        E(2, 4),  // 8E: MOV   a8,R6
        E(2, 4),  // 8F: MOV   a8,R7
        E(3, 4),  // 90: MOV   DPTR,#16
        E(2, 4),  // 91: ACALL a11
        E(2, 4),  // 92: MOV   b8,C
        E(1, 4),  // 93: MOVC  A,@A+DPTR
        E(2, 2),  // 94: SUBB  A,#n8
        E(2, 2),  // 95: SUBB  A,a8
        E(1, 2),  // 96: SUBB  A,@R0
        E(1, 2),  // 97: SUBB  A,@R1
        E(1, 2),  // 98: SUBB  A,R0
        E(1, 2),  // 99: SUBB  A,R1
        E(1, 2),  // 9A: SUBB  A,R2
        E(1, 2),  // 9B: SUBB  A,R3
        E(1, 2),  // 9C: SUBB  A,R4
        E(1, 2),  // 9D: SUBB  A,R5
        E(1, 2),  // 9E: SUBB  A,R6
        E(1, 2),  // 9F: SUBB  A,R7
        E(2, 4),  // A0: ORL   C,/b8
        E(2, 4),  // A1: AJMP  a11
        E(2, 2),  // A2: MOV   C,b8
        E(1, 4),  // A3: INC   DPTR
        E(1, 8),  // A4: MUL   AB
        0,        // A5:
        E(2, 4),  // A6: MOV   @R0,a8
        E(2, 4),  // A7: MOV   @R1,a8
        E(2, 4),  // A8: MOV   R0,a8
        E(2, 4),  // A9: MOV   R1,a8
        E(2, 4),  // AA: MOV   R2,a8
        E(2, 4),  // AB: MOV   R3,a8
        E(2, 4),  // AC: MOV   R4,a8
        E(2, 4),  // AD: MOV   R5,a8
        E(2, 4),  // AE: MOV   R6,a8
        E(2, 4),  // AF: MOV   R7,a8
        E(2, 4),  // B0: ANL   C,/b8
        E(2, 4),  // B1: ACALL a11
        E(2, 2),  // B2: CPL   C,b8
        E(1, 2),  // B3: CPL   C
        E(3, 4),  // B4: CJNE  A,#n8,r8
        E(3, 4),  // B5: CJNE  A,a8,r8
        E(3, 4),  // B6: CJNE  @R0,#n8,r8
        E(3, 4),  // B7: CJNE  @R1,#n8,r8
        E(3, 4),  // B8: CJNE  R0,#n8,r8
        E(3, 4),  // B9: CJNE  R1,#n8,r8
        E(3, 4),  // BA: CJNE  R2,#n8,r8
        E(3, 4),  // BB: CJNE  R3,#n8,r8
        E(3, 4),  // BC: CJNE  R4,#n8,r8
        E(3, 4),  // BD: CJNE  R5,#n8,r8
        E(3, 4),  // BE: CJNE  R6,#n8,r8
        E(3, 4),  // BF: CJNE  R7,#n8,r8
        E(2, 4),  // C0: PUSH  a8
        E(2, 4),  // C1: AJMP  a11
        E(2, 2),  // C2: CLR   b8
        E(1, 2),  // C3: CLR   C
        E(1, 2),  // C4: SWAP  A
        E(2, 2),  // C5: XCH   A,a8
        E(1, 2),  // C6: XCH   A,@R0
        E(1, 2),  // C7: XCH   A,@R1
        E(1, 2),  // C8: XCH   A,R0
        E(1, 2),  // C9: XCH   A,R1
        E(1, 2),  // CA: XCH   A,R2
        E(1, 2),  // CB: XCH   A,R3
        E(1, 2),  // CC: XCH   A,R4
        E(1, 2),  // CD: XCH   A,R5
        E(1, 2),  // CE: XCH   A,R6
        E(1, 2),  // CF: XCH   A,R7
        E(2, 4),  // D0: POP   a8
        E(2, 4),  // D1: ACALL a11
        E(2, 2),  // D2: SETB  C,b8
        E(1, 2),  // D3: SETB  C
        E(1, 2),  // D4: DA    A
        E(3, 4),  // D5: DJNZ  a8,r8
        E(1, 2),  // D6: XCHD  A,@R0
        E(1, 2),  // D7: XCHD  A,@R1
        E(2, 4),  // D8: DJNZ  R0,r8
        E(2, 4),  // D9: DJNZ  R1,r8
        E(2, 4),  // DA: DJNZ  R2,r8
        E(2, 4),  // DB: DJNZ  R3,r8
        E(2, 4),  // DC: DJNZ  R4,r8
        E(2, 4),  // DD: DJNZ  R5,r8
        E(2, 4),  // DE: DJNZ  R6,r8
        E(2, 4),  // DF: DJNZ  R7,r8
        E(1, 3),  // E0: MOVX  A,@DPTR
        E(2, 4),  // E1: AJMP  a11
        E(1, 3),  // E2: MOVX  A,@R0
        E(1, 3),  // E3: MOVX  A,@R1
        E(1, 2),  // E4: CLR   A
        E(2, 2),  // E5: MOV   A,a8
        E(1, 2),  // E6: MOV   A,@R0
        E(1, 2),  // E7: MOV   A,@R1
        E(1, 2),  // E8: MOV   A,R0
        E(1, 2),  // E9: MOV   A,R1
        E(1, 2),  // EA: MOV   A,R2
        E(1, 2),  // EB: MOV   A,R3
        E(1, 2),  // EC: MOV   A,R4
        E(1, 2),  // ED: MOV   A,R5
        E(1, 2),  // EE: MOV   A,R6
        E(1, 2),  // EF: MOV   A,R7
        E(1, 3),  // F0: MOVX  @DPTR,A
        E(2, 4),  // F1: ACALL a11
        E(1, 3),  // F2: MOVX  @R0,A
        E(1, 3),  // F3: MOVX  @R1,A
        E(1, 2),  // F4: CPL   A
        E(2, 2),  // F5: MOV   a8,A
        E(1, 2),  // F6: XCHD  A,@R0
        E(1, 2),  // F7: XCHD  A,@R1
        E(1, 2),  // F8: MOV   R0,A
        E(1, 2),  // F9: MOV   R1,A
        E(1, 2),  // FA: MOV   R2,A
        E(1, 2),  // FB: MOV   R3,A
        E(1, 2),  // FC: MOV   R4,A
        E(1, 2),  // FD: MOV   R5,A
        E(1, 2),  // FE: MOV   R6,A
        E(1, 2),  // FF: MOV   R7,A
};

}  // namespace

uint8_t InstI8051::busCycles(uint8_t inst) {
    return bus_cyc(INST_TABLE[inst]);
}

}  // namespace i8051
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
