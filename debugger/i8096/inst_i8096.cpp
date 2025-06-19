#include "inst_i8096.h"
#include "debugger.h"
#include "mems_i8096.h"

namespace debugger {
namespace i8096 {

namespace {

constexpr uint8_t E(uint8_t len, uint8_t var, uint8_t bus) {
    return (bus << 6) | (var << 4) | len;
}

constexpr uint8_t bus_cycles(uint8_t entry) {
    return (entry >> 6) & 3;
}

constexpr bool indexed(uint8_t entry) {
    return (entry & 0x10) != 0;
}

constexpr uint_fast8_t inst_len(uint8_t entry) {
    return entry & 0xF;
}

constexpr uint8_t PAGE00_TABLE[] = {
        E(2, 0, 0),  // 00: SKIP  b
        E(2, 0, 0),  // 01: CLR   w
        E(2, 0, 0),  // 02: NOT   w
        E(2, 0, 0),  // 03: NEG   w
        0,           // 04
        E(2, 0, 0),  // 05: DEC   w
        E(2, 0, 0),  // 06: EXT   l
        E(2, 0, 0),  // 07: INC   w
        E(3, 0, 0),  // 08: SHR   w,c
        E(3, 0, 0),  // 09: SHL   w,c
        E(3, 0, 0),  // 0A: SHRA  w,c
        0,           // 0B
        E(3, 0, 0),  // 0C: SHRL  w,c
        E(3, 0, 0),  // 0D: SHLL  w,c
        E(3, 0, 0),  // 0E: SHRAL w,c
        E(3, 0, 0),  // 0F: NORM  l,b
        0,           // 10
        E(2, 0, 0),  // 11: CLRB  b
        E(2, 0, 0),  // 12: NOTB  b
        E(2, 0, 0),  // 13: NEGB  b
        0,           // 14
        E(2, 0, 0),  // 15: DECB  b
        E(2, 0, 0),  // 16: EXTB  w
        E(2, 0, 0),  // 17: INCB  b
        E(2, 0, 0),  // 18: SHRB  b,c
        E(2, 0, 0),  // 19: SHLB  b,c
        E(2, 0, 0),  // 1A: SHRAB b,c
        0,           // 1B
        0,           // 1C
        0,           // 1D
        0,           // 1E
        0,           // 1F
        E(2, 0, 0),  // 20: SJMP  r
        E(2, 0, 0),  // 21: SJMP  r
        E(2, 0, 0),  // 22: SJMP  r
        E(2, 0, 0),  // 23: SJMP  r
        E(2, 0, 0),  // 24: SJMP  r
        E(2, 0, 0),  // 25: SJMP  r
        E(2, 0, 0),  // 26: SJMP  r
        E(2, 0, 0),  // 27: SJMP  r
        E(2, 0, 2),  // 28: SCALL r
        E(2, 0, 2),  // 29: SCALL r
        E(2, 0, 2),  // 2A: SCALL r
        E(2, 0, 2),  // 2B: SCALL r
        E(2, 0, 2),  // 2C: SCALL r
        E(2, 0, 2),  // 2D: SCALL r
        E(2, 0, 2),  // 2E: SCALL r
        E(2, 0, 2),  // 2F: SCALL r
        E(3, 0, 0),  // 30: JBC   0,r
        E(3, 0, 0),  // 31: JBC   1,r
        E(3, 0, 0),  // 32: JBC   2,r
        E(3, 0, 0),  // 33: JBC   3,r
        E(3, 0, 0),  // 34: JBC   4,r
        E(3, 0, 0),  // 35: JBC   5,r
        E(3, 0, 0),  // 36: JBC   6,r
        E(3, 0, 0),  // 37: JBC   7,r
        E(3, 0, 0),  // 38: JBS   0,r
        E(3, 0, 0),  // 39: JBS   1,r
        E(3, 0, 0),  // 3A: JBS   2,r
        E(3, 0, 0),  // 3B: JBS   3,r
        E(3, 0, 0),  // 3C: JBS   4,r
        E(3, 0, 0),  // 3D: JBS   5,r
        E(3, 0, 0),  // 3E: JBS   6,r
        E(3, 0, 0),  // 3F: JBS   7,r
        E(4, 0, 0),  // 40: AND   w,w,w
        E(5, 0, 0),  // 41: AND   w,w,#
        E(4, 0, 2),  // 42: AND   w,w,[w]
        E(5, 1, 2),  // 43: AND   w,w,n[w]
        E(4, 0, 0),  // 44: ADD   w,w,w
        E(5, 0, 0),  // 45: ADD   w,w,#
        E(4, 0, 2),  // 46: ADD   w,w,[w]
        E(5, 1, 2),  // 47: ADD   w,w,n[w]
        E(4, 0, 0),  // 48: SUB   w,w,w
        E(5, 0, 0),  // 49: SUB   w,w,#
        E(4, 0, 2),  // 4A: SUB   w,w,[w]
        E(5, 1, 2),  // 4B: SUB   w,w,n[w]
        E(4, 0, 0),  // 4C: MULU  w,w,w
        E(5, 0, 0),  // 4D: MULU  w,w,#
        E(4, 0, 2),  // 4E: MULU  w,w,[w]
        E(5, 1, 2),  // 4F: MULU  w,w,n[w]
        E(4, 0, 0),  // 50: ANDB  b,b,b
        E(4, 0, 0),  // 51: ANDB  b,b,#
        E(4, 0, 1),  // 52: ANDB  b,b,[w]
        E(5, 1, 1),  // 53: ANDB  b,b,n[w]
        E(4, 0, 0),  // 54: ADDB  b,b,b
        E(4, 0, 0),  // 55: ADDB  b,b,#
        E(4, 0, 1),  // 56: ADDB  b,b,[w]
        E(5, 1, 1),  // 57: ADDB  b,b,n[w]
        E(4, 0, 0),  // 58: SUBB  b,b,b
        E(4, 0, 0),  // 59: SUBB  b,b,#
        E(4, 0, 1),  // 5A: SUBB  b,b,[w]
        E(5, 1, 1),  // 5B: SUBB  b,b,n[w]
        E(4, 0, 0),  // 5C: MULUB w,b,b
        E(4, 0, 0),  // 5D: MULUB w,b,#
        E(4, 0, 1),  // 5E: MULUB w,b,[w]
        E(5, 1, 1),  // 5F: MULUB w,b,n[w]
        E(3, 0, 0),  // 60: AND   w,w
        E(4, 0, 0),  // 61: AND   w,#
        E(3, 0, 2),  // 62: AND   w,[w]
        E(4, 1, 2),  // 63: AND   w,n[w]
        E(3, 0, 0),  // 64: ADD   w,w
        E(4, 0, 0),  // 65: ADD   w,#
        E(3, 0, 2),  // 66: ADD   w,[w]
        E(4, 1, 2),  // 67: ADD   w,n[w]
        E(3, 0, 0),  // 68: SUB   w,w
        E(4, 0, 0),  // 69: SUB   w,#
        E(3, 0, 2),  // 6A: SUB   w,[w]
        E(4, 1, 2),  // 6B: SUB   w,n[w]
        E(3, 0, 0),  // 6C: MULU  l,w
        E(4, 0, 0),  // 6D: MULU  l,#
        E(3, 0, 2),  // 6E: MULU  l,[w]
        E(4, 1, 2),  // 6F: MULY  l,n[w]
        E(3, 0, 0),  // 70: ANDB  b,b
        E(3, 0, 0),  // 71: ANDB  b,#
        E(3, 0, 1),  // 72: ANDB  b,[w]
        E(4, 1, 1),  // 73: ANDB  b,n[w]
        E(3, 0, 0),  // 74: ADDB  b,b
        E(3, 0, 0),  // 75: ADDB  b,#
        E(3, 0, 1),  // 76: ADDB  b,[w]
        E(4, 1, 1),  // 77: ADDB  b,n[w]
        E(3, 0, 0),  // 78: SUBB  b,b
        E(3, 0, 0),  // 79: SUBB  b,#
        E(3, 0, 1),  // 7A: SUBB  b,[w]
        E(4, 1, 1),  // 7B: SUBB  b,n[w]
        E(3, 0, 0),  // 7C: MULUB w,b
        E(3, 0, 0),  // 7D: MULUB w,#
        E(3, 0, 1),  // 7E: MULUB w,[w]
        E(4, 1, 1),  // 7F: MULUB w,n[w]
        E(3, 0, 0),  // 80: OR    w,w
        E(4, 0, 0),  // 81: OR    w,#
        E(3, 0, 2),  // 82: OR    w,[w]
        E(4, 1, 2),  // 83: OR    w,n[w]
        E(3, 0, 0),  // 84: XOR   w,w
        E(4, 0, 0),  // 85: XOR   w,#
        E(3, 0, 2),  // 86: XOR   w,[w]
        E(4, 1, 2),  // 87: XOR   w,n[w]
        E(3, 0, 0),  // 88: CMP   w,w
        E(4, 0, 0),  // 89: CMP   w,#
        E(3, 0, 2),  // 8A: CMP   w,[w]
        E(4, 1, 2),  // 8B: CMP   w,n[w]
        E(3, 0, 0),  // 8C: DIV   l,w
        E(4, 0, 0),  // 8D: DIV   l,#
        E(3, 0, 2),  // 8E: DIV   l,[w]
        E(4, 1, 2),  // 8F: DIV   l,n[w]
        E(3, 0, 0),  // 90: ORB   b,b
        E(3, 0, 0),  // 91: ORB   b,#
        E(3, 0, 1),  // 92: ORB   b,[w]
        E(4, 1, 1),  // 93: ORB   b,n[w]
        E(3, 0, 0),  // 94: XORB  b,b
        E(3, 0, 0),  // 95: XORB  b,#
        E(3, 0, 1),  // 96: XORB  b,[w]
        E(4, 1, 1),  // 97: XORB  b,n[w]
        E(3, 0, 0),  // 98: CMPB  b,b
        E(3, 0, 0),  // 99: CMPB  b,#
        E(3, 0, 1),  // 9A: CMPB  b,[w]
        E(4, 1, 1),  // 9B: CMPB  b,n[w]
        E(3, 0, 0),  // 9C: DIVB  w,b
        E(3, 0, 0),  // 9D: DIVB  w,#
        E(3, 0, 1),  // 9E: DIVB  w,[w]
        E(4, 1, 1),  // 9F: DIVB  w,n[w]
        E(3, 0, 0),  // A0: LD    w,w
        E(4, 0, 0),  // A1: LD    w,#
        E(3, 0, 2),  // A2: LD    w,[w]
        E(4, 1, 2),  // A3: LD    w,n[w]
        E(3, 0, 0),  // A4: ADDC  w,w
        E(4, 0, 0),  // A5: ADDC  w,#
        E(3, 0, 2),  // A6: ADDC  w,[w]
        E(4, 1, 2),  // A7: ADDC  w,n[w]
        E(3, 0, 0),  // A8: SUBC  w,w
        E(4, 0, 0),  // A9: SUBC  w,#
        E(3, 0, 2),  // AA: SUBC  w,[w]
        E(4, 1, 2),  // AB: SUBC  w,n[w]
        E(3, 0, 0),  // AC: LDBZE w,b
        E(3, 0, 0),  // AD: LDBZE w,#
        E(3, 0, 1),  // AE: LDBZE w,[w]
        E(4, 1, 1),  // AF: LDBZE w,n[w]
        E(3, 0, 0),  // B0: LDB   b,b
        E(3, 0, 0),  // B1: LDB   b,#
        E(3, 0, 1),  // B2: LDB   b,[w]
        E(4, 1, 1),  // B3: LDB   b,n[w]
        E(3, 0, 0),  // B4: ADDCB b,b
        E(3, 0, 0),  // B5: ADDCB b,#
        E(3, 0, 1),  // B6: ADDCB b,[w]
        E(4, 1, 1),  // B7: ADDCB b,n[w]
        E(3, 0, 0),  // B8: SUBCB b,b
        E(3, 0, 0),  // B9: SUBCB b,#
        E(3, 0, 1),  // BA: SUBCB b,[w]
        E(4, 1, 1),  // BB: SUBCB b,n[w]
        E(3, 0, 0),  // BC: LDBSE w,b
        E(3, 0, 0),  // BD: LDBSE w,#
        E(3, 0, 1),  // BE: LDBSE w,[w]
        E(4, 1, 1),  // BF: LDBSE w,n[w]
        E(3, 0, 0),  // C0: ST    w,w
        0,           // C1
        E(3, 0, 2),  // C2: ST    w,[w]
        E(4, 1, 2),  // C3: ST    w,n[w]
        E(3, 0, 0),  // C4: STB   b,b
        0,           // C5
        E(3, 0, 1),  // C6: STB   b,[w]
        E(4, 1, 1),  // C7: STB   b,n[w]
        E(2, 0, 2),  // C8: PUSH  w
        E(3, 0, 2),  // C9: PUSH  #
        E(2, 0, 4),  // CA: PUSH  [w]
        E(3, 1, 4),  // CB: PUSH  n[w]
        E(2, 0, 2),  // CC: POP   w
        0,           // CD
        E(2, 0, 4),  // CE: POP   [w]
        E(3, 1, 4),  // CF: POP   n[w]
        E(2, 0, 0),  // D0: JNST  r
        E(2, 0, 0),  // D1: JNH   r
        E(2, 0, 0),  // D2: JGT   r
        E(2, 0, 0),  // D3: JNC   r
        E(2, 0, 0),  // D4: JNVT  r
        E(2, 0, 0),  // D5: JNV   r
        E(2, 0, 0),  // D6: JGE   r
        E(2, 0, 0),  // D7: JNE   r
        E(2, 0, 0),  // D8: JST   r
        E(2, 0, 0),  // D9: JH    r
        E(2, 0, 0),  // DA: JLE   r
        E(2, 0, 0),  // DB: JC    r
        E(2, 0, 0),  // DC: JVT   r
        E(2, 0, 0),  // DD: JV    r
        E(2, 0, 0),  // DE: JLT   r
        E(2, 0, 0),  // DF: JE    r
        E(3, 0, 0),  // E0: DJNZ  w,r
        0,           // E1
        0,           // E2
        E(2, 0, 0),  // E3: BR    [w]
        0,           // E4
        0,           // E5
        0,           // E6
        E(3, 0, 0),  // E7: LJMP  rr
        0,           // E8
        0,           // E9
        0,           // EA
        0,           // EB
        0,           // EC
        0,           // ED
        0,           // EE
        E(3, 0, 2),  // EF: LCALL rr
        E(1, 0, 2),  // F0: RET   -
        0,           // F1
        E(1, 0, 2),  // F2: PUSHF -
        E(1, 0, 2),  // F3: POPF  -
        0,           // F4
        0,           // F5
        0,           // F6
        E(1, 0, 4),  // F7: TRAP  -
        E(1, 0, 0),  // F8: CLRC  -
        E(1, 0, 0),  // F9: SETC  -
        E(1, 0, 0),  // FA: DI    -
        E(1, 0, 0),  // FB: EI    -
        E(1, 0, 0),  // FC: CLRVT -
        E(1, 0, 0),  // FD: NOP   -
        0,           // FE
        E(1, 0, 1),  // FF: RST   -
};

constexpr uint8_t PAGEFE_TABLE[] = {
        E(5, 0, 0),  // 4C: MUL   l,w,w
        E(6, 0, 0),  // 4D: MUL   l,w,#
        E(5, 0, 2),  // 4E: MUL   l,w,[w]
        E(6, 1, 2),  // 4F: MUL   l,w,n[w]
        E(5, 0, 0),  // 5C: MULB  w,b,b
        E(5, 0, 0),  // 5D: MULB  w,b,#
        E(5, 0, 1),  // 5E: MULB  w,b,[w]
        E(6, 1, 1),  // 5F: MULB  w,b,n[w]
        E(4, 0, 0),  // 6C: MUL   l,w
        E(5, 0, 0),  // 6D: MUL   l,#
        E(4, 0, 2),  // 6E: MUL   l,[w]
        E(5, 1, 2),  // 6F: MUL   l,n[w]
        E(4, 0, 0),  // 7C: MULB  w,b
        E(4, 0, 0),  // 7D: MULB  w,#
        E(4, 0, 1),  // 7E: MULB  w,[w]
        E(5, 1, 1),  // 7F: MULB  w,n[w]
        E(4, 0, 0),  // 8C: DIV   l,w
        E(5, 0, 0),  // 8D: DIV   l,#
        E(4, 0, 2),  // 8E: DIV   l,[w]
        E(5, 1, 2),  // 8F: DIV   l,n[w]
        E(4, 0, 0),  // 9C: DIVB  w,b
        E(4, 0, 0),  // 9D: DIVB  w,#
        E(4, 0, 1),  // 9E: DIVB  w,[w]
        E(5, 1, 1),  // 9F: DIVB  w,n[w]
};

}  // namespace

InstI8096::InstI8096(MemsI8096 *mems) : _mems(mems) {}

bool InstI8096::set(uint16_t pc) {
    auto opc = _mems->read(pc++);
    if (opc == 0xFE) {
        opc = _mems->read(pc++);
        const auto hi = opc >> 4;
        const auto lo = (opc >> 2) & 3;
        if (hi < 0x4 || hi >= 0xA || lo != 3)
            return 0;
        const auto index = ((hi - 0x4) << 2) + (opc & 3);
        _entry = PAGEFE_TABLE[index];
    } else {
        _entry = PAGE00_TABLE[opc];
    }
    if (_entry == 0)
        return false;
    _indexed = indexed(_entry) && (_mems->read(pc) & 1);
    return true;
}

uint_fast8_t InstI8096::instLength() const {
    auto cycles = inst_len(_entry);
    if (_indexed)
        cycles++;
    return cycles;
}

uint_fast8_t InstI8096::busCycles() const {
    return bus_cycles(_entry);
}

}  // namespace i8096
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
