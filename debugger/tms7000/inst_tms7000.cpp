#include "inst_tms7000.h"
#include <string.h>
#include "debugger.h"
#include "mems_tms7000.h"
#include "regs_tms7000.h"

namespace debugger {
namespace tms7000 {

namespace {

constexpr uint8_t E(uint8_t len, uint8_t bus) {
    return (len << 4) | bus;
}

constexpr uint8_t inst_len(uint8_t entry) {
    return entry >> 4;
}

constexpr uint8_t bus_cycles(uint8_t entry) {
    return entry & 0xF;
}

constexpr uint8_t INST_TABLE[] = {
        E(1, 0),  // 00: NOP   -
        E(1, 0),  // 01: IDLE  -
        0,        // 02
        0,        // 03
        0,        // 04
        E(1, 0),  // 05: EINT  -
        E(1, 0),  // 06: DINT  -
        E(1, 0),  // 07: SETC  -
        E(1, 0),  // 08: POP   ST
        E(1, 0),  // 09: STSP  -
        E(1, 0),  // 0A: RETS  -
        E(1, 0),  // 0B: RETI  -
        0,        // 0C
        E(1, 0),  // 0D: LDSP  -
        E(1, 0),  // 0E: PUSH  ST
        0,        // 0F
        0,        // 10
        0,        // 11
        E(2, 0),  // 12: MOV   Rs,A
        E(2, 0),  // 13: AND   Rs,A
        E(2, 0),  // 14: OR    Rs,A
        E(2, 0),  // 15: XOR   Rs,A
        E(3, 0),  // 16: BTJO  Rn,A,r8
        E(3, 0),  // 17: BTJZ  Rn,A,r8
        E(2, 0),  // 18: ADD   Rs,A
        E(2, 0),  // 19: ADC   Rs,A
        E(2, 0),  // 1A: SUB   Rs,A
        E(2, 0),  // 1B: SBB   Rs,A
        E(2, 0),  // 1C: MPY   Rs,A
        E(2, 0),  // 1D: CMP   Rs,A
        E(2, 0),  // 1E: DAC   Rs,A
        E(2, 0),  // 1F: DSB   Rs,A
        0,        // 20
        0,        // 21
        E(2, 0),  // 22: MOV   %n,A
        E(2, 0),  // 23: AND   %n,A
        E(2, 0),  // 24: OR    %n,A
        E(2, 0),  // 25: XOR   %n,A
        E(3, 0),  // 26: BTJO  %n,A,r8
        E(3, 0),  // 27: BTJZ  %n,A,r8
        E(2, 0),  // 28: ADD   %n,A
        E(2, 0),  // 29: ADC   %n,A
        E(2, 0),  // 2A: SUB   %n,A
        E(2, 0),  // 2B: SBB   %n,A
        E(2, 0),  // 2C: MPY   %n,A
        E(2, 0),  // 2D: CMP   %n,A
        E(2, 0),  // 2E: DAC   %n,A
        E(2, 0),  // 2F: DSB   %n,A
        0,        // 30
        0,        // 31
        E(2, 0),  // 32: MOV   Rs,B
        E(2, 0),  // 33: AND   Rs,B
        E(2, 0),  // 34: OR    Rs,B
        E(2, 0),  // 35: XOR   Rs,B
        E(3, 0),  // 36: BTJO  Rn,B,r8
        E(3, 0),  // 37: BTJZ  Rn,B,r8
        E(2, 0),  // 38: ADD   Rs,B
        E(2, 0),  // 39: ADC   Rs,B
        E(2, 0),  // 3A: SUB   Rs,B
        E(2, 0),  // 3B: SBB   Rs,B
        E(2, 0),  // 3C: MPY   Rs,B
        E(2, 0),  // 3D: CMP   Rs,B
        E(2, 0),  // 3E: DAC   Rs,B
        E(2, 0),  // 3F: DSB   Rs,B
        0,        // 40
        0,        // 41
        E(3, 0),  // 42: MOV   Rs,Rd
        E(3, 0),  // 43: AND   Rs,Rd
        E(3, 0),  // 44: OR    Rs,Rd
        E(3, 0),  // 45: XOR   Rs,Rd
        E(4, 0),  // 46: BTJO  Rn,Rd,r8
        E(4, 0),  // 47: BTJZ  Rn,Rd,r8
        E(3, 0),  // 48: ADD   Rs,Rd
        E(3, 0),  // 49: ADC   Rs,Rd
        E(3, 0),  // 4A: SUB   Rs,Rd
        E(3, 0),  // 4B: SBB   Rs,Rd
        E(3, 0),  // 4C: MPY   Rn,Rn
        E(3, 0),  // 4D: CMP   Rs,Rd
        E(3, 0),  // 4E: DAC   Rs,Rd
        E(3, 0),  // 4F: DSB   Rs,Rd
        0,        // 50
        0,        // 51
        E(2, 0),  // 52: MOV   %n,B
        E(2, 0),  // 53: AND   %n,B
        E(2, 0),  // 54: OR    %n,B
        E(2, 0),  // 55: XOR   %n,B
        E(3, 0),  // 56: BTJO  %n,B,r8
        E(3, 0),  // 57: BTJZ  %n,B,r8
        E(2, 0),  // 58: ADD   %n,B
        E(2, 0),  // 59: ADC   %n,B
        E(2, 0),  // 5A: SUB   %n,B
        E(2, 0),  // 5B: SBB   %n,B
        E(2, 0),  // 5C: MPY   %n,B
        E(2, 0),  // 5D: CMP   %n,B
        E(2, 0),  // 5E: DAC   %n,B
        E(2, 0),  // 5F: DSB   %n,B
        0,        // 60
        0,        // 61
        E(1, 0),  // 62: MOV   B,A
        E(1, 0),  // 63: AND   B,A
        E(1, 0),  // 64: OR    B,A
        E(1, 0),  // 65: XOR   B,A
        E(2, 0),  // 66: BTJO  B,A,r8
        E(2, 0),  // 67: BTJZ  B,A,r8
        E(1, 0),  // 68: ADD   B,A
        E(1, 0),  // 69: ADC   B,A
        E(1, 0),  // 6A: SUB   B,A
        E(1, 0),  // 6B: SBB   B,A
        E(1, 0),  // 6C: MPY   B,A
        E(1, 0),  // 6D: CMP   B,A
        E(1, 0),  // 6E: DAC   B,A
        E(1, 0),  // 6F: DSB   B,A
        0,        // 70
        0,        // 71
        E(3, 0),  // 72: MOV   %n,Rd
        E(3, 0),  // 73: AND   %n,Rd
        E(3, 0),  // 74: OR    %n,Rd
        E(3, 0),  // 75: XOR   %n,Rd
        E(4, 0),  // 76: BTJO  %n,Rn,r8
        E(4, 0),  // 77: BTJZ  %n,Rn,r8
        E(3, 0),  // 78: ADD   %n,Rd
        E(3, 0),  // 79: ADC   %n,Rd
        E(3, 0),  // 7A: SUB   Rs,Rd
        E(3, 0),  // 7B: SBB   Rs,Rd
        E(3, 0),  // 7C: MPY   %n,Rn
        E(3, 0),  // 7D: CMP   %n,Rd
        E(3, 0),  // 7E: DAC   %n,Rn
        E(3, 0),  // 7F: DSB   %n,Rd
        E(2, 1),  // 80: MOVP  Ps,A
        0,        // 81
        E(2, 2),  // 82: MOVP  A,Pd
        E(2, 2),  // 83: ANDP  A,Pd
        E(2, 2),  // 84: ORP   A,Pd
        E(2, 2),  // 85: XORP  A,Pd
        E(3, 1),  // 86: BTJOP A,Pn,r8
        E(3, 1),  // 87: BTJZP A,Pn,r8
        E(4, 0),  // 88: MOVD  %n16,Rp
        0,        // 89
        E(3, 1),  // 8A: LDA   @a16
        E(3, 1),  // 8B: STA   @a16
        E(3, 0),  // 8C: BR    @a16
        E(3, 1),  // 8D: CMPA  @a16
        E(3, 0),  // 8E: CALL  @a16
        0,        // 8F
        0,        // 90
        E(2, 1),  // 91: MOVP  Ps,B
        E(2, 2),  // 92: MOVP  B,Pd
        E(2, 2),  // 93: ANDP  B,Pd
        E(2, 2),  // 94: ORP   B,Pd
        E(2, 2),  // 95: XORP  B,Pd
        E(3, 1),  // 96: BTJOP B,Pn,r8
        E(3, 1),  // 97: BTJZP B,Pn,r8
        E(3, 0),  // 98: MOVD  Rp,Rp
        0,        // 99
        E(2, 1),  // 9A: LDA   *Rp
        E(2, 1),  // 9B: STA   *Rp
        E(2, 0),  // 9C: BR    *Rp
        E(2, 1),  // 9D: CMPA  *Rp
        E(2, 0),  // 9E: CALL  *Rp
        0,        // 9F
        0,        // A0
        0,        // A1
        E(3, 2),  // A2: MOVP  %n,Pd
        E(3, 2),  // A3: ANDP  %n,Pd
        E(3, 2),  // A4: ORP   %n,Pd
        E(3, 2),  // A5: XORP  %n,Pd
        E(4, 1),  // A6: BTJOP %n,Pn,r8
        E(4, 1),  // A7: BTJZP %n,Pn,r8
        E(4, 0),  // A8: MOVD  @a16(B),Rp
        0,        // A9
        E(3, 1),  // AA: LDA   @a16(B)
        E(3, 1),  // AB: STA   @a16(B)
        E(3, 0),  // AC: BR    @a16(B)
        E(3, 1),  // AD: CMPA  @a16(B)
        E(3, 0),  // AE: CALL  @a16(B)
        0,        // AF
        E(1, 0),  // B0: TSTA  -
        0,        // B1
        E(1, 0),  // B2: DEC   A
        E(1, 0),  // B3: INC   A
        E(1, 0),  // B4: INV   A
        E(1, 0),  // B5: CLR   A
        E(1, 0),  // B6: XCHB  A
        E(1, 0),  // B7: SWAP  A
        E(1, 0),  // B8: PUSH  A
        E(1, 0),  // B9: POP   A
        E(2, 0),  // BA: DJNZ  A,r8
        E(1, 0),  // BB: DECD  A
        E(1, 0),  // BC: RR    A
        E(1, 0),  // BD: RRC   A
        E(1, 0),  // BE: RL    A
        E(1, 0),  // BF: RLC   A
        E(1, 0),  // C0: MOV   A,B
        E(1, 0),  // C1: TST   B
        E(1, 0),  // C2: DEC   B
        E(1, 0),  // C3: INC   B
        E(1, 0),  // C4: INV   B
        E(1, 0),  // C5: CLR   B
        E(1, 0),  // C6: XCHB  B
        E(1, 0),  // C7: SWAP  B
        E(1, 0),  // C8: PUSH  B
        E(1, 0),  // C9: POP   B
        E(2, 0),  // CA: DJNZ  B,r8
        E(1, 0),  // CB: DECD  B
        E(1, 0),  // CC: RR    B
        E(1, 0),  // CD: RRC   B
        E(1, 0),  // CE: RL    B
        E(1, 0),  // CF: RLC   B
        E(2, 0),  // D0: MOV   A,Rd
        E(2, 0),  // D1: MOV   B,Rd
        E(2, 0),  // D2: DEC   Rn
        E(2, 0),  // D3: INC   Rn
        E(2, 0),  // D4: INV   Rn
        E(2, 0),  // D5: CLR   Rn
        E(2, 0),  // D6: XCHB  Rn
        E(2, 0),  // D7: SWAP  Rd
        E(2, 0),  // D8: PUSH  Rs
        E(2, 0),  // D9: POP   Rd
        E(3, 0),  // DA: DJNZ  Rn,r8
        E(2, 0),  // DB: DECD  Rp
        E(2, 0),  // DC: RR    Rd
        E(2, 0),  // DD: RRC   Rd
        E(2, 0),  // DE: RL    Rd
        E(2, 0),  // DF: RLC   Rd
        E(2, 0),  // E0: JMP   r8
        E(2, 0),  // E1: JN    r8
        E(2, 0),  // E2: JZ    r8
        E(2, 0),  // E3: JC    r8
        E(2, 0),  // E4: JP    r8
        E(2, 0),  // E5: JPZ   r8
        E(2, 0),  // E6: JNZ   r8
        E(2, 0),  // E7: JNC   r8
        E(1, 0),  // E8: TRAP  23
        E(1, 0),  // E9: TRAP  22
        E(1, 0),  // EA: TRAP  21
        E(1, 0),  // EB: TRAP  20
        E(1, 0),  // EC: TRAP  19
        E(1, 0),  // ED: TRAP  18
        E(1, 0),  // EE: TRAP  17
        E(1, 0),  // EF: TRAP  16
        E(1, 0),  // F0: TRAP  15
        E(1, 0),  // F1: TRAP  14
        E(1, 0),  // F2: TRAP  13
        E(1, 0),  // F3: TRAP  12
        E(1, 0),  // F4: TRAP  11
        E(1, 0),  // F5: TRAP  10
        E(1, 0),  // F6: TRAP  9
        E(1, 0),  // F7: TRAP  8
        E(1, 0),  // F8: TRAP  7
        E(1, 0),  // F9: TRAP  6
        E(1, 0),  // FA: TRAP  5
        E(1, 0),  // FB: TRAP  4
        E(1, 0),  // FC: TRAP  3
        E(1, 0),  // FD: TRAP  2
        E(1, 0),  // FE: TRAP  1
        E(1, 0),  // FF: TRAP  0
};
}  // namespace

uint8_t InstTms7000::busCycles(uint8_t opc) {
    const auto e = INST_TABLE[opc];
    return inst_len(e) + bus_cycles(e);
}

}  // namespace tms7000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
