#include "inst_f3850.h"

namespace debugger {
namespace f3850 {

namespace {

constexpr uint8_t inst_len(uint8_t e) {
    return e & 3;
}

constexpr uint8_t bus_cycles(uint8_t e) {
    return (e >> 2) & 7;
}

constexpr uint8_t E(uint8_t len, uint8_t cycles) {
    // len: 0~3, cycles: 0~2
    return (cycles << 2) | len;
}

constexpr uint8_t INST_TABLE[] = {
        E(1, 0),  // 00: LR   A,KU
        E(1, 0),  // 01: LR   A,KL
        E(1, 0),  // 02: LR   A,QU
        E(1, 0),  // 03: LR   A,QL
        E(1, 0),  // 04: LR   KU,A
        E(1, 0),  // 05: LR   KL,A
        E(1, 0),  // 06: LR   QU,A
        E(1, 0),  // 07: LR   QL,A
        E(1, 2),  // 08: LR   K,P
        E(1, 2),  // 09: LR   P,K
        E(1, 0),  // 0A: LR   A,IS
        E(1, 0),  // 0B: LR   IS,A
        E(1, 2),  // 0C: PK   -
        E(1, 2),  // 0D: LR   P0,Q
        E(1, 2),  // 0E: LR   Q,DC
        E(1, 2),  // 0F: LR   DC,Q
        E(1, 2),  // 10: LR   DC,H
        E(1, 2),  // 11: LR   H,DC
        E(1, 0),  // 12: SR   1
        E(1, 0),  // 13: SL   1
        E(1, 0),  // 14: SR   4
        E(1, 0),  // 15: SL   4
        E(1, 1),  // 16: LM   -
        E(1, 1),  // 17: ST   -
        E(1, 0),  // 18: COM  -
        E(1, 0),  // 19: LNK  -
        E(1, 1),  // 1A: DI   -
        E(1, 1),  // 1B: EI   -
        E(1, 1),  // 1C: POP  -
        E(1, 1),  // 1D: LR   W,J
        E(1, 0),  // 1E: LR   J,W
        E(1, 0),  // 1F: INC  -
        E(2, 0),  // 20: LI   aa
        E(2, 0),  // 21: NI   aa
        E(2, 0),  // 22: OI   aa
        E(2, 0),  // 23: XI   aa
        E(2, 0),  // 24: AI   aa
        E(2, 0),  // 25: CI   aa
        E(2, 1),  // 26: IN   pp
        E(2, 1),  // 27: OUT  pp
        E(3, 2),  // 28: PI   iijj
        E(3, 1),  // 29: JMP  iijj
        E(3, 2),  // 2A: DCI  iijj
        E(1, 0),  // 2B: NOP  -
        E(1, 1),  // 2C: XDC  -
        E(0, 0),  // 2D: -    -
        E(0, 0),  // 2E: -    -
        E(0, 0),  // 2F: -    -
        E(1, 0),  // 30: DS   0
        E(1, 0),  // 31: DS   1
        E(1, 0),  // 32: DS   2
        E(1, 0),  // 33: DS   3
        E(1, 0),  // 34: DS   4
        E(1, 0),  // 35: DS   5
        E(1, 0),  // 36: DS   6
        E(1, 0),  // 37: DS   7
        E(1, 0),  // 38: DS   8
        E(1, 0),  // 39: DS   J
        E(1, 0),  // 3A: DS   HU
        E(1, 0),  // 3B: DS   HL
        E(1, 0),  // 3C: DS   S
        E(1, 0),  // 3D: DS   I
        E(1, 0),  // 3E: DS   D
        E(0, 0),  // 3F: -    -
        E(1, 0),  // 40: LR   A,0
        E(1, 0),  // 41: LR   A,1
        E(1, 0),  // 42: LR   A,2
        E(1, 0),  // 43: LR   A,3
        E(1, 0),  // 44: LR   A,4
        E(1, 0),  // 45: LR   A,5
        E(1, 0),  // 46: LR   A,6
        E(1, 0),  // 47: LR   A,7
        E(1, 0),  // 48: LR   A,8
        E(1, 0),  // 49: LR   A,J
        E(1, 0),  // 4A: LR   A,HU
        E(1, 0),  // 4B: LR   A,HL
        E(1, 0),  // 4C: LR   A,S
        E(1, 0),  // 4D: LR   A,I
        E(1, 0),  // 4E: LR   A,D
        E(0, 0),  // 4F: -    -
        E(1, 0),  // 50: LR   0,A
        E(1, 0),  // 51: LR   1,A
        E(1, 0),  // 52: LR   2,A
        E(1, 0),  // 53: LR   3,A
        E(1, 0),  // 54: LR   4,A
        E(1, 0),  // 55: LR   5,A
        E(1, 0),  // 56: LR   6,A
        E(1, 0),  // 57: LR   7,A
        E(1, 0),  // 58: LR   8,A
        E(1, 0),  // 59: LR   J,A
        E(1, 0),  // 5A: LR   HU,A
        E(1, 0),  // 5B: LR   HL,A
        E(1, 0),  // 5C: LR   S,A
        E(1, 0),  // 5D: LR   I,A
        E(1, 0),  // 5E: LR   D,A
        E(0, 0),  // 5F: -    -
        E(1, 0),  // 60: LISU 0
        E(1, 0),  // 61: LISU 1
        E(1, 0),  // 62: LISU 2
        E(1, 0),  // 63: LISU 3
        E(1, 0),  // 64: LISU 4
        E(1, 0),  // 65: LISU 5
        E(1, 0),  // 66: LISU 6
        E(1, 0),  // 67: LISU 7
        E(1, 0),  // 68: LISL 0
        E(1, 0),  // 69: LISL 1
        E(1, 0),  // 6A: LISL 2
        E(1, 0),  // 6B: LISL 3
        E(1, 0),  // 6C: LISL 4
        E(1, 0),  // 6D: LISL 5
        E(1, 0),  // 6E: LISL 6
        E(1, 0),  // 6F: LISL 7
        E(1, 0),  // 70: LIS  0
        E(1, 0),  // 71: LIS  1
        E(1, 0),  // 72: LIS  2
        E(1, 0),  // 73: LIS  3
        E(1, 0),  // 74: LIS  4
        E(1, 0),  // 75: LIS  5
        E(1, 0),  // 76: LIS  6
        E(1, 0),  // 77: LIS  7
        E(1, 0),  // 78: LIS  8
        E(1, 0),  // 79: LIS  9
        E(1, 0),  // 7A: LIS  10
        E(1, 0),  // 7B: LIS  11
        E(1, 0),  // 7C: LIS  12
        E(1, 0),  // 7D: LIS  13
        E(1, 0),  // 7E: LIS  14
        E(1, 0),  // 7F: LIS  15
        E(2, 1),  // 80: BT   0,ii
        E(2, 1),  // 81: BP   ii
        E(2, 1),  // 82: BC   ii
        E(2, 1),  // 83: BT   3,ii
        E(2, 1),  // 84: BZ   ii
        E(2, 1),  // 85: BT   5,ii
        E(2, 1),  // 86: BT   6,ii
        E(2, 1),  // 87: BT   7,ii
        E(1, 1),  // 88: AM   -
        E(1, 1),  // 89: AMD  -
        E(1, 1),  // 8A: NM   -
        E(1, 1),  // 8B: OM   -
        E(1, 1),  // 8C: XM   -
        E(1, 1),  // 8D: CM   -
        E(1, 1),  // 8E: ADC  -
        E(2, 0),  // 8F: BR7  ij
        E(2, 1),  // 90: BR   ij
        E(2, 1),  // 91: BM   ij
        E(2, 1),  // 92: BNC  ij
        E(2, 1),  // 93: BF   3,ij
        E(2, 1),  // 94: BNZ  ij
        E(2, 1),  // 95: BF   5,ij
        E(2, 1),  // 96: BF   6,ij
        E(2, 1),  // 97: BF   7,ij
        E(2, 1),  // 98: BNO  ij
        E(2, 1),  // 99: BF   9,ij
        E(2, 1),  // 9A: BF   10,ij
        E(2, 1),  // 9B: BF   11,ij
        E(2, 1),  // 9C: BF   12,ij
        E(2, 1),  // 9D: BF   13,ij
        E(2, 1),  // 9E: BF   14,ij
        E(2, 1),  // 9F: BF   15,ij
        E(1, 1),  // A0: INS  0
        E(1, 1),  // A1: INS  1
        E(1, 0),  // A2: -    -
        E(1, 0),  // A3: -    -
        E(1, 2),  // A4: INS  4
        E(1, 2),  // A5: INS  5
        E(1, 2),  // A6: INS  6
        E(1, 2),  // A7: INS  7
        E(1, 2),  // A8: INS  8
        E(1, 2),  // A9: INS  9
        E(1, 2),  // AA: INS  10
        E(1, 2),  // AB: INS  11
        E(1, 2),  // AC: INS  12
        E(1, 2),  // AD: INS  13
        E(1, 2),  // AE: INS  14
        E(1, 2),  // AF: INS  15
        E(1, 1),  // B0: OUTS 0
        E(1, 1),  // B1: OUTS 1
        E(1, 0),  // B2: -    -
        E(1, 0),  // B3: -    -
        E(1, 2),  // B4: OUTS 4
        E(1, 2),  // B5: OUTS 5
        E(1, 2),  // B6: OUTS 6
        E(1, 2),  // B7: OUTS 7
        E(1, 2),  // B8: OUTS 8
        E(1, 2),  // B9: OUTS 9
        E(1, 2),  // BA: OUTS 10
        E(1, 2),  // BB: OUTS 11
        E(1, 2),  // BC: OUTS 12
        E(1, 2),  // BD: OUTS 13
        E(1, 2),  // BE: OUTS 14
        E(1, 2),  // BF: OUTS 15
        E(1, 0),  // C0: AS   0
        E(1, 0),  // C1: AS   1
        E(1, 0),  // C2: AS   2
        E(1, 0),  // C3: AS   3
        E(1, 0),  // C4: AS   4
        E(1, 0),  // C5: AS   5
        E(1, 0),  // C6: AS   6
        E(1, 0),  // C7: AS   7
        E(1, 0),  // C8: AS   8
        E(1, 0),  // C9: AS   J
        E(1, 0),  // CA: AS   HU
        E(1, 0),  // CB: AS   HL
        E(1, 0),  // CC: AS   S
        E(1, 0),  // CD: AS   I
        E(1, 0),  // CE: AS   D
        E(0, 0),  // CF: -    -
        E(1, 1),  // D0: ASD  0
        E(1, 1),  // D1: ASD  1
        E(1, 1),  // D2: ASD  2
        E(1, 1),  // D3: ASD  3
        E(1, 1),  // D4: ASD  4
        E(1, 1),  // D5: ASD  5
        E(1, 1),  // D6: ASD  6
        E(1, 1),  // D7: ASD  7
        E(1, 1),  // D8: ASD  8
        E(1, 1),  // D9: ASD  J
        E(1, 1),  // DA: ASD  HU
        E(1, 1),  // DB: ASD  HL
        E(1, 1),  // DC: ASD  S
        E(1, 1),  // DD: ASD  I
        E(1, 1),  // DE: ASD  D
        E(0, 0),  // DF: -    -
        E(1, 0),  // E0: XS   0
        E(1, 0),  // E1: XS   1
        E(1, 0),  // E2: XS   2
        E(1, 0),  // E3: XS   3
        E(1, 0),  // E4: XS   4
        E(1, 0),  // E5: XS   5
        E(1, 0),  // E6: XS   6
        E(1, 0),  // E7: XS   7
        E(1, 0),  // E8: XS   8
        E(1, 0),  // E9: XS   J
        E(1, 0),  // EA: XS   HU
        E(1, 0),  // EB: XS   HL
        E(1, 0),  // EC: XS   S
        E(1, 0),  // ED: XS   I
        E(1, 0),  // EE: XS   D
        E(0, 0),  // EF: -    -
        E(1, 0),  // F0: NS   0
        E(1, 0),  // F1: NS   1
        E(1, 0),  // F2: NS   2
        E(1, 0),  // F3: NS   3
        E(1, 0),  // F4: NS   4
        E(1, 0),  // F5: NS   5
        E(1, 0),  // F6: NS   6
        E(1, 0),  // F7: NS   7
        E(1, 0),  // F8: NS   8
        E(1, 0),  // F9: NS   J
        E(1, 0),  // FA: NS   HU
        E(1, 0),  // FB: NS   HL
        E(1, 0),  // FC: NS   S
        E(1, 0),  // FD: NS   I
        E(1, 0),  // FE: NS   D
        E(0, 0),  // FF: -    -
};

}  // namespace

uint8_t InstF3850::instLength(uint8_t inst) {
    return inst_len(INST_TABLE[inst]);
}

uint8_t InstF3850::busCycles(uint8_t inst) {
    return bus_cycles(INST_TABLE[inst]);
}

}  // namespace f3850
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
