#include "inst_ins8060.h"

namespace debugger {
namespace ins8060 {

namespace {
constexpr uint8_t INST_TABLE[] = {
        1,  // 00: HALT -
        1,  // 01: XAE  -
        1,  // 02: CCL  -
        1,  // 03: SCL  -
        1,  // 04: DINT -
        1,  // 05: IEN  -
        1,  // 06: CSA  -
        1,  // 07: CAS  -
        1,  // 08: NOP  -
        0,  // 09: -    -
        0,  // 0A: -    -
        0,  // 0B: -    -
        0,  // 0C: -    -
        0,  // 0D: -    -
        0,  // 0E: -    -
        0,  // 0F: -    -
        0,  // 10: -    -
        0,  // 11: -    -
        0,  // 12: -    -
        0,  // 13: -    -
        0,  // 14: -    -
        0,  // 15: -    -
        0,  // 16: -    -
        0,  // 17: -    -
        0,  // 18: -    -
        1,  // 19: SIO  -
        0,  // 1A: -    -
        0,  // 1B: -    -
        1,  // 1C: SR   -
        1,  // 1D: SRL  -
        1,  // 1E: RR   -
        1,  // 1F: RRL  -
        0,  // 20: -    -
        0,  // 21: -    -
        0,  // 22: -    -
        0,  // 23: -    -
        0,  // 24: -    -
        0,  // 25: -    -
        0,  // 26: -    -
        0,  // 27: -    -
        0,  // 28: -    -
        0,  // 29: -    -
        0,  // 2A: -    -
        0,  // 2B: -    -
        0,  // 2C: -    -
        0,  // 2D: -    -
        0,  // 2E: -    -
        0,  // 2F: -    -
        1,  // 30: XPAL PC
        1,  // 31: XPAL P1
        1,  // 32: XPAL P2
        1,  // 33: XPAL P3
        1,  // 34: XPAH PC
        1,  // 35: XPAH P1
        1,  // 36: XPAH P2
        1,  // 37: XPAH P3
        0,  // 38: -    -
        0,  // 39: -    -
        0,  // 3A: -    -
        0,  // 3B: -    -
        1,  // 3C: XPPC PC
        1,  // 3D: XPPC P1
        1,  // 3E: XPPC P2
        1,  // 3F: XPPC P3
        1,  // 40: LDE  -
        0,  // 41: -    -
        0,  // 42: -    -
        0,  // 43: -    -
        0,  // 44: -    -
        0,  // 45: -    -
        0,  // 46: -    -
        0,  // 47: -    -
        0,  // 48: -    -
        0,  // 49: -    -
        0,  // 4A: -    -
        0,  // 4B: -    -
        0,  // 4C: -    -
        0,  // 4D: -    -
        0,  // 4E: -    -
        0,  // 4F: -    -
        1,  // 50: ANE  -
        0,  // 51: -    -
        0,  // 52: -    -
        0,  // 53: -    -
        0,  // 54: -    -
        0,  // 55: -    -
        0,  // 56: -    -
        0,  // 57: -    -
        1,  // 58: ORE  -
        0,  // 59: -    -
        0,  // 5A: -    -
        0,  // 5B: -    -
        0,  // 5C: -    -
        0,  // 5D: -    -
        0,  // 5E: -    -
        0,  // 5F: -    -
        1,  // 60: XRE  -
        0,  // 61: -    -
        0,  // 62: -    -
        0,  // 63: -    -
        0,  // 64: -    -
        0,  // 65: -    -
        0,  // 66: -    -
        0,  // 67: -    -
        1,  // 68: DAE  -
        0,  // 69: -    -
        0,  // 6A: -    -
        0,  // 6B: -    -
        0,  // 6C: -    -
        0,  // 6D: -    -
        0,  // 6E: -    -
        0,  // 6F: -    -
        1,  // 70: ADE  -
        0,  // 71: -    -
        0,  // 72: -    -
        0,  // 73: -    -
        0,  // 74: -    -
        0,  // 75: -    -
        0,  // 76: -    -
        0,  // 77: -    -
        1,  // 78: CAE  -
        0,  // 79: -    -
        0,  // 7A: -    -
        0,  // 7B: -    -
        0,  // 7C: -    -
        0,  // 7D: -    -
        0,  // 7E: -    -
        0,  // 7F: -    -
        0,  // 80: -    -
        0,  // 81: -    -
        0,  // 82: -    -
        0,  // 83: -    -
        0,  // 84: -    -
        0,  // 85: -    -
        0,  // 86: -    -
        0,  // 87: -    -
        0,  // 88: -    -
        0,  // 89: -    -
        0,  // 8A: -    -
        0,  // 8B: -    -
        0,  // 8C: -    -
        0,  // 8D: -    -
        0,  // 8E: -    -
        2,  // 8F: DLY  n
        2,  // 90: JMP  addr
        2,  // 91: JMP  d(P1)
        2,  // 92: JMP  d(P2)
        2,  // 93: JMP  d(P3)
        2,  // 94: JP   addr
        2,  // 95: JP   d(P1)
        2,  // 96: JP   d(P2)
        2,  // 97: JP   d(P3)
        2,  // 98: JZ   addr
        2,  // 99: JZ   d(P1)
        2,  // 9A: JZ   d(P2)
        2,  // 9B: JZ   d(P3)
        2,  // 9C: JNZ  addr
        2,  // 9D: JNZ  d(P1)
        2,  // 9E: JNZ  d(P2)
        2,  // 9F: JNZ  d(P3)
        0,  // A0: -    -
        0,  // A1: -    -
        0,  // A2: -    -
        0,  // A3: -    -
        0,  // A4: -    -
        0,  // A5: -    -
        0,  // A6: -    -
        0,  // A7: -    -
        2,  // A8: ILD  addr
        2,  // A9: ILD  d(P1)
        2,  // AA: ILD  d(P2)
        2,  // AB: ILD  d(P3)
        0,  // AC: -    -
        0,  // AD: -    -
        0,  // AE: -    -
        0,  // AF: -    -
        0,  // B0: -    -
        0,  // B1: -    -
        0,  // B2: -    -
        0,  // B3: -    -
        0,  // B4: -    -
        0,  // B5: -    -
        0,  // B6: -    -
        0,  // B7: -    -
        2,  // B8: DLD  addr
        2,  // B9: DLD  d(P1)
        2,  // BA: DLD  d(P2)
        2,  // BB: DLD  d(P3)
        0,  // BC: -    -
        0,  // BD: -    -
        0,  // BE: -    -
        0,  // BF: -    -
        2,  // C0: LD   addr
        2,  // C1: LD   d(P1)
        2,  // C2: LD   d(P2)
        2,  // C3: LD   d(P3)
        2,  // C4: LDI  n
        2,  // C5: LD   @d(P1)
        2,  // C6: LD   @d(P2)
        2,  // C7: LD   @d(P3)
        2,  // C8: ST   addr
        2,  // C9: ST   d(P1)
        2,  // CA: ST   d(P2)
        2,  // CB: ST   d(P3)
        0,  // CC: -    -
        2,  // CD: ST   @d(P1)
        2,  // CE: ST   @d(P2)
        2,  // CF: ST   @d(P3)
        2,  // D0: AND  addr
        2,  // D1: AND  d(P1)
        2,  // D2: AND  d(P2)
        2,  // D3: AND  d(P3)
        2,  // D4: ANI  n
        2,  // D5: AND  @d(P1)
        2,  // D6: AND  @d(P2)
        2,  // D7: AND  @d(P3)
        2,  // D8: OR   addr
        2,  // D9: OR   d(P1)
        2,  // DA: OR   d(P2)
        2,  // DB: OR   d(P3)
        2,  // DC: ORI  n
        2,  // DD: OR   @d(P1)
        2,  // DE: OR   @d(P2)
        2,  // DF: OR   @d(P3)
        2,  // E0: XOR  addr
        2,  // E1: XOR  d(P1)
        2,  // E2: XOR  d(P2)
        2,  // E3: XOR  d(P3)
        2,  // E4: XRI  n
        2,  // E5: XOR  @d(P1)
        2,  // E6: XOR  @d(P2)
        2,  // E7: XOR  @d(P3)
        2,  // E8: DAD  addr
        2,  // E9: DAD  d(P1)
        2,  // EA: DAD  d(P2)
        2,  // EB: DAD  d(P3)
        2,  // EC: DAI  n
        2,  // ED: DAD  @d(P1)
        2,  // EE: DAD  @d(P2)
        2,  // EF: DAD  @d(P3)
        2,  // F0: ADD  addr
        2,  // F1: ADD  d(P1)
        2,  // F2: ADD  d(P2)
        2,  // F3: ADD  d(P3)
        2,  // F4: ADI  n
        2,  // F5: ADD  @d(P1)
        2,  // F6: ADD  @d(P2)
        2,  // F7: ADD  @d(P3)
        2,  // F8: CAD  addr
        2,  // F9: CAD  d(P1)
        2,  // FA: CAD  d(P2)
        2,  // FB: CAD  d(P3)
        2,  // FC: CAI  n
        2,  // FD: CAD  @d(P1)
        2,  // FE: CAD  @d(P2)
        2,  // FF: CAD  @d(P3)
};
}

uint8_t InstIns8060::instLen(uint8_t inst) {
    return INST_TABLE[inst];
}

}  // namespace ins8060
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
