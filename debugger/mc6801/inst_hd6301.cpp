#include "inst_hd6301.h"
#include "mems_mc6801.h"

namespace debugger {
namespace hd6301 {

struct InstHd6301 Inst {
    &mc6801::Memory,
};

namespace {
/**
# 1: instruction code (next=&1)
# 2: instruction operands (disp, high(addr), ++next)
# 3: instruction operands (low(addr), ++next)
# R: read 1 byte (high(addr))
# r: read 1 byte at address R+1 or r+1 (low(addr))
# W: write 1 byte, equals to R if exists or w-1
# w: write 1 byte at address W+1
# A: read 1 byte at address |addr|
# a: read 1 byte ar A+1
# D: read 1 byte from direct page (00|disp|)
# d: read 1 byte at address (D+1)
# B: write 1 byte at address addr
# b: write 1 byte ar B+1
# E: write 1 byte to direct page (00xx), equals to D if exists
# e: write 1 byte at address E+1
# N: next instruction read from |next|
# J: next instruction read from |addr|
# j: next instruction read from |next| or |next|+disp
# i: next instruction read from unknown
# V: read 1 byte from address FFF8-FFFE (high(addr))
# v: read 1 byte from address V+1 (low(addr))
# X: any valid read
# x: irrelevant read access to $FFFF
# n: non VMA access
#
# Interrupt sequence
# 1:X:x:w:W:W:W:W:W:W:V:v:J
*/

constexpr const char INTERRUPT[] = "1XxwWWWWWWVvJ";

constexpr const char *const SEQUENCES[/*seq*/] = {
        "",               //  0
        "0N",             //  1
        "0Nx",            //  2
        "0NxxN",          //  3
        "02xj",           //  4
        "0NxR",           //  5
        "0NxWN",          //  6
        "0NxRr",          //  7
        "0NxRrJ",         //  8
        "0NxRrrrrrrJ",    //  9
        "0NxwWN",         // 10
        "0Nxxxxxx",       // 11
        "0NxwWWWWWW",     // 12
        "0NxwWWWWWWVvJ",  // 13
        "02xRxWN",        // 14
        "023xRxWN",       // 15
        "023xRN",         // 16
        "02xRN",          // 17
        "02xi",           // 18
        "02xRWN",         // 19
        "023AxBN",        // 20
        "023DxEN",        // 21
        "023DN",          // 22
        "023AN",          // 23
        "023J",           // 24
        "023ABN",         // 25
        "02N",            // 26
        "023N",           // 27
        "02xwWj",         // 28
        "02DN",           // 29
        "02DdN",          // 30
        "02EN",           // 31
        "02xwWi",         // 32
        "02EeN",          // 33
        "02xRrN",         // 34
        "02xWN",          // 35
        "02xWwN",         // 36
        "023AaN",         // 37
        "023BN",          // 38
        "023xwWJ",        // 39
        "023BbN",         // 40
};

constexpr uint8_t INST_TABLE[] = {
        0,   // 00: -    -     0 :0  -
        1,   // 01: NOP  -     1 :1  0:N
        0,   // 02: -    -     0 :0  -
        0,   // 03: -    -     0 :0  -
        1,   // 04: LSRD -     1 :1  0:N
        1,   // 05: ASLD -     1 :1  0:N
        1,   // 06: TAP  -     1 :1  0:N
        1,   // 07: TPA  -     1 :1  0:N
        1,   // 08: INX  -     1 :1  0:N
        1,   // 09: DEX  -     1 :1  0:N
        1,   // 0A: CLV  -     1 :1  0:N
        1,   // 0B: SEV  -     1 :1  0:N
        1,   // 0C: CLC  -     1 :1  0:N
        1,   // 0D: SEC  -     1 :1  0:N
        1,   // 0E: CLI  -     1 :1  0:N
        1,   // 0F: SEI  -     1 :1  0:N
        1,   // 10: SBA  -     1 :1  0:N
        1,   // 11: CBA  -     1 :1  0:N
        0,   // 12: -    -     0 :0  -
        0,   // 13: -    -     0 :0  -
        0,   // 14: -    -     0 :0  -
        0,   // 15: -    -     0 :0  -
        1,   // 16: TAB  -     1 :1  0:N
        1,   // 17: TBA  -     1 :1  0:N
        2,   // 18: XGDX -     2 :1  0:N:x
        2,   // 19: DAA  -     2 :1  0:N:x
        3,   // 1A: SLP  -     4 :1  0:N:x:x:N
        1,   // 1B: ABA  -     1 :1  0:N
        0,   // 1C: -    -     0 :0  -
        0,   // 1D: -    -     0 :0  -
        0,   // 1E: -    -     0 :0  -
        0,   // 1F: -    -     0 :0  -
        4,   // 20: BRA  r8    3 :2  0:2:x:j
        4,   // 21: BRN  r8    3 :2  0:2:x:j
        4,   // 22: BHI  r8    3 :2  0:2:x:j
        4,   // 23: BLS  r8    3 :2  0:2:x:j
        4,   // 24: BCC  r8    3 :2  0:2:x:j
        4,   // 25: BCS  r8    3 :2  0:2:x:j
        4,   // 26: BNE  r8    3 :2  0:2:x:j
        4,   // 27: BEQ  r8    3 :2  0:2:x:j
        4,   // 28: BVC  r8    3 :2  0:2:x:j
        4,   // 29: BVS  r8    3 :2  0:2:x:j
        4,   // 2A: BPL  r8    3 :2  0:2:x:j
        4,   // 2B: BMI  r8    3 :2  0:2:x:j
        4,   // 2C: BGE  r8    3 :2  0:2:x:j
        4,   // 2D: BLT  r8    3 :2  0:2:x:j
        4,   // 2E: BGT  r8    3 :2  0:2:x:j
        4,   // 2F: BLE  r8    3 :2  0:2:x:j
        1,   // 30: TSX  -     1 :1  0:N
        1,   // 31: INS  -     1 :1  0:N
        5,   // 32: PULA -     3 :1  0:N:x:R
        5,   // 33: PULB -     3 :1  0:N:x:R
        1,   // 34: DES  -     1 :1  0:N
        1,   // 35: TXS  -     1 :1  0:N
        6,   // 36: PSHA -     4 :1  0:N:x:W:N
        6,   // 37: PSHB -     4 :1  0:N:x:W:N
        7,   // 38: PULX -     4 :1  0:N:x:R:r
        8,   // 39: RTS  -     5 :1  0:N:x:R:r:J
        1,   // 3A: ABX  -     1 :1  0:N
        9,   // 3B: RTI  -     10:1  0:N:x:R:r:r:r:r:r:r:J
        10,  // 3C: PSHX -     5 :1  0:N:x:w:W:N
        11,  // 3D: MUL  -     7 :1  0:N:x:x:x:x:x:x
        12,  // 3E: WAI  -     9 :1  0:N:x:w:W:W:W:W:W:W
        13,  // 3F: SWI  -     12:1  0:N:x:w:W:W:W:W:W:W:V:v:J
        1,   // 40: NEGA -     1 :1  0:N
        0,   // 41: -    -     0 :0  -
        0,   // 42: -    -     0 :0  -
        1,   // 43: COMA -     1 :1  0:N
        1,   // 44: LSRA -     1 :1  0:N
        0,   // 45: -    -     0 :0  -
        1,   // 46: RORA -     1 :1  0:N
        1,   // 47: ASRA -     1 :1  0:N
        1,   // 48: ASLA -     1 :1  0:N
        1,   // 49: ROLA -     1 :1  0:N
        1,   // 4A: DECA -     1 :1  0:N
        0,   // 4B: -    -     0 :0  -
        1,   // 4C: INCA -     1 :1  0:N
        1,   // 4D: TSTA -     1 :1  0:N
        0,   // 4E: -    -     0 :0  -
        1,   // 4F: CLRA -     1 :1  0:N
        1,   // 50: NEGB -     1 :1  0:N
        0,   // 51: -    -     0 :0  -
        0,   // 52: -    -     0 :0  -
        1,   // 53: COMB -     1 :1  0:N
        1,   // 54: LSRB -     1 :1  0:N
        0,   // 55: -    -     0 :0  -
        1,   // 56: RORB -     1 :1  0:N
        1,   // 57: ASRB -     1 :1  0:N
        1,   // 58: ASLB -     1 :1  0:N
        1,   // 59: ROLB -     1 :1  0:N
        1,   // 5A: DECB -     1 :1  0:N
        0,   // 5B: -    -     0 :0  -
        1,   // 5C: INCB -     1 :1  0:N
        1,   // 5D: TSTB -     1 :1  0:N
        0,   // 5E: -    -     0 :0  -
        1,   // 5F: CLRB -     1 :1  0:N
        14,  // 60: NEG  n8,X  6 :2  0:2:x:R:x:W:N
        15,  // 61: AIM  #,n8,X  7 :3  0:2:3:x:R:x:W:N
        15,  // 62: OIM  #,n8,X  7 :3  0:2:3:x:R:x:W:N
        14,  // 63: COM  n8,X  6 :2  0:2:x:R:x:W:N
        14,  // 64: LSR  n8,X  6 :2  0:2:x:R:x:W:N
        15,  // 65: EIM  #,n8,X  7 :3  0:2:3:x:R:x:W:N
        14,  // 66: ROR  n8,X  6 :2  0:2:x:R:x:W:N
        14,  // 67: ASR  n8,X  6 :2  0:2:x:R:x:W:N
        14,  // 68: ASL  n8,X  6 :2  0:2:x:R:x:W:N
        14,  // 69: ROL  n8,X  6 :2  0:2:x:R:x:W:N
        14,  // 6A: DEC  n8,X  6 :2  0:2:x:R:x:W:N
        16,  // 6B: TIM  #,n8,X  5 :3  0:2:3:x:R:N
        14,  // 6C: INC  n8,X  6 :2  0:2:x:R:x:W:N
        17,  // 6D: TST  n8,X  4 :2  0:2:x:R:N
        18,  // 6E: JMP  n8,X  3 :2  0:2:x:i
        19,  // 6F: CLR  n8,X  5 :2  0:2:x:R:W:N
        20,  // 70: NEG  a16   6 :3  0:2:3:A:x:B:N
        21,  // 71: AIM  #,d8  6 :3  0:2:3:D:x:E:N
        21,  // 72: OIM  #,d8  6 :3  0:2:3:D:x:E:N
        20,  // 73: COM  a16   6 :3  0:2:3:A:x:B:N
        20,  // 74: LSR  a16   6 :3  0:2:3:A:x:B:N
        21,  // 75: EIM  #,d8  6 :3  0:2:3:D:x:E:N
        20,  // 76: ROR  a16   6 :3  0:2:3:A:x:B:N
        20,  // 77: ASR  a16   6 :3  0:2:3:A:x:B:N
        20,  // 78: ASL  a16   6 :3  0:2:3:A:x:B:N
        20,  // 79: ROL  a16   6 :3  0:2:3:A:x:B:N
        20,  // 7A: DEC  a16   6 :3  0:2:3:A:x:B:N
        22,  // 7B: TIM  #,d8  4 :3  0:2:3:D:N
        20,  // 7C: INC  a16   6 :3  0:2:3:A:x:B:N
        23,  // 7D: TST  a16   4 :3  0:2:3:A:N
        24,  // 7E: JMP  a16   3 :3  0:2:3:J
        25,  // 7F: CLR  a16   5 :3  0:2:3:A:B:N
        26,  // 80: SUBA #n8   2 :2  0:2:N
        26,  // 81: CMPA #n8   2 :2  0:2:N
        26,  // 82: SBCA #n8   2 :2  0:2:N
        27,  // 83: SUBD #n16  3 :3  0:2:3:N
        26,  // 84: ANDA #n8   2 :2  0:2:N
        26,  // 85: BITA #n8   2 :2  0:2:N
        26,  // 86: LDAA #n8   2 :2  0:2:N
        0,   // 87: -    -     0 :0  -
        26,  // 88: EORA #n8   2 :2  0:2:N
        26,  // 89: ADCA #n8   2 :2  0:2:N
        26,  // 8A: ORAA #n8   2 :2  0:2:N
        26,  // 8B: ADDA #n8   2 :2  0:2:N
        27,  // 8C: CPX  #n16  3 :3  0:2:3:N
        28,  // 8D: BSR  r8    5 :2  0:2:x:w:W:j
        27,  // 8E: LDS  #n16  3 :3  0:2:3:N
        0,   // 8F: -    -     0 :0  -
        29,  // 90: SUBA d8    3 :2  0:2:D:N
        29,  // 91: CMPA d8    3 :2  0:2:D:N
        29,  // 92: SBCA d8    3 :2  0:2:D:N
        30,  // 93: SUBD d8    4 :2  0:2:D:d:N
        29,  // 94: ANDA d8    3 :2  0:2:D:N
        29,  // 95: BITA d8    3 :2  0:2:D:N
        29,  // 96: LDAA d8    3 :2  0:2:D:N
        31,  // 97: STAA d8    3 :2  0:2:E:N
        29,  // 98: EORA d8    3 :2  0:2:D:N
        29,  // 99: ADCA d8    3 :2  0:2:D:N
        29,  // 9A: ORAA d8    3 :2  0:2:D:N
        29,  // 9B: ADDA d8    3 :2  0:2:D:N
        30,  // 9C: CPX  d8    4 :2  0:2:D:d:N
        32,  // 9D: JSR  d8    5 :2  0:2:x:w:W:i
        30,  // 9E: LDS  d8    4 :2  0:2:D:d:N
        33,  // 9F: STS  d8    4 :2  0:2:E:e:N
        17,  // A0: SUBA n8,X  4 :2  0:2:x:R:N
        17,  // A1: CMPA n8,X  4 :2  0:2:x:R:N
        17,  // A2: SBCA n8,X  4 :2  0:2:x:R:N
        34,  // A3: SUBD n8,X  5 :2  0:2:x:R:r:N
        17,  // A4: ANDA n8,X  4 :2  0:2:x:R:N
        17,  // A5: BITA n8,X  4 :2  0:2:x:R:N
        17,  // A6: LDAA n8,X  4 :2  0:2:x:R:N
        35,  // A7: STAA n8,X  4 :2  0:2:x:W:N
        17,  // A8: EORA n8,X  4 :2  0:2:x:R:N
        17,  // A9: ADCA n8,X  4 :2  0:2:x:R:N
        17,  // AA: ORAA n8,X  4 :2  0:2:x:R:N
        17,  // AB: ADDA n8,X  4 :2  0:2:x:R:N
        34,  // AC: CPX  n8,X  5 :2  0:2:x:R:r:N
        32,  // AD: JSR  n8,X  5 :2  0:2:x:w:W:i
        34,  // AE: LDS  n8,X  5 :2  0:2:x:R:r:N
        36,  // AF: STS  n8,X  5 :2  0:2:x:W:w:N
        23,  // B0: SUBA a16   4 :3  0:2:3:A:N
        23,  // B1: CMPA a16   4 :3  0:2:3:A:N
        23,  // B2: SBCA a16   4 :3  0:2:3:A:N
        37,  // B3: SUBD a16   5 :3  0:2:3:A:a:N
        23,  // B4: ANDA a16   4 :3  0:2:3:A:N
        23,  // B5: BITA a16   4 :3  0:2:3:A:N
        23,  // B6: LDAA a16   4 :3  0:2:3:A:N
        38,  // B7: STAA a16   4 :3  0:2:3:B:N
        23,  // B8: EORA a16   4 :3  0:2:3:A:N
        23,  // B9: ADCA a16   4 :3  0:2:3:A:N
        23,  // BA: ORAA a16   4 :3  0:2:3:A:N
        23,  // BB: ADDA a16   4 :3  0:2:3:A:N
        37,  // BC: CPX  a16   5 :3  0:2:3:A:a:N
        39,  // BD: JSR  a16   6 :3  0:2:3:x:w:W:J
        37,  // BE: LDS  a16   5 :3  0:2:3:A:a:N
        40,  // BF: STS  a16   5 :3  0:2:3:B:b:N
        26,  // C0: SUBB #n8   2 :2  0:2:N
        26,  // C1: CMPB #n8   2 :2  0:2:N
        26,  // C2: SBCB #n8   2 :2  0:2:N
        27,  // C3: ADDD #n16  3 :3  0:2:3:N
        26,  // C4: ANDB #n8   2 :2  0:2:N
        26,  // C5: BITB #n8   2 :2  0:2:N
        26,  // C6: LDAB #n8   2 :2  0:2:N
        0,   // C7: -    -     0 :0  -
        26,  // C8: EORB #n8   2 :2  0:2:N
        26,  // C9: ADCB #n8   2 :2  0:2:N
        26,  // CA: ORAB #n8   2 :2  0:2:N
        26,  // CB: ADDB #n8   2 :2  0:2:N
        27,  // CC: LDD  #n16  3 :3  0:2:3:N
        0,   // CD: -    -     0 :0  -
        27,  // CE: LDX  #n16  3 :3  0:2:3:N
        0,   // CF: -    -     0 :0  -
        29,  // D0: SUBB d8    3 :2  0:2:D:N
        29,  // D1: CMPB d8    3 :2  0:2:D:N
        29,  // D2: SBCB d8    3 :2  0:2:D:N
        30,  // D3: ADDD d8    4 :2  0:2:D:d:N
        29,  // D4: ANDB d8    3 :2  0:2:D:N
        29,  // D5: BITB d8    3 :2  0:2:D:N
        29,  // D6: LDAB d8    3 :2  0:2:D:N
        31,  // D7: STAB d8    3 :2  0:2:E:N
        29,  // D8: EORB d8    3 :2  0:2:D:N
        29,  // D9: ADCB d8    3 :2  0:2:D:N
        29,  // DA: ORAB d8    3 :2  0:2:D:N
        29,  // DB: ADDB d8    3 :2  0:2:D:N
        30,  // DC: LDD  d8    4 :2  0:2:D:d:N
        33,  // DD: STD  d8    4 :2  0:2:E:e:N
        30,  // DE: LDX  d8    4 :2  0:2:D:d:N
        33,  // DF: STX  d8    4 :2  0:2:E:e:N
        17,  // E0: SUBB n8,X  4 :2  0:2:x:R:N
        17,  // E1: CMPB n8,X  4 :2  0:2:x:R:N
        17,  // E2: SBCB n8,X  4 :2  0:2:x:R:N
        34,  // E3: ADDD n8,X  5 :2  0:2:x:R:r:N
        17,  // E4: ANDB n8,X  4 :2  0:2:x:R:N
        17,  // E5: BITB n8,X  4 :2  0:2:x:R:N
        17,  // E6: LDAB n8,X  4 :2  0:2:x:R:N
        35,  // E7: STAB n8,X  4 :2  0:2:x:W:N
        17,  // E8: EORB n8,X  4 :2  0:2:x:R:N
        17,  // E9: ADCB n8,X  4 :2  0:2:x:R:N
        17,  // EA: ORAB n8,X  4 :2  0:2:x:R:N
        17,  // EB: ADDB n8,X  4 :2  0:2:x:R:N
        34,  // EC: LDD  n8,X  5 :2  0:2:x:R:r:N
        36,  // ED: STD  n8,X  5 :2  0:2:x:W:w:N
        34,  // EE: LDX  n8,X  5 :2  0:2:x:R:r:N
        36,  // EF: STX  n8,X  5 :2  0:2:x:W:w:N
        23,  // F0: SUBB a16   4 :3  0:2:3:A:N
        23,  // F1: CMPB a16   4 :3  0:2:3:A:N
        23,  // F2: SBCB a16   4 :3  0:2:3:A:N
        37,  // F3: ADDD a16   5 :3  0:2:3:A:a:N
        23,  // F4: ANDB a16   4 :3  0:2:3:A:N
        23,  // F5: BITB a16   4 :3  0:2:3:A:N
        23,  // F6: LDAB a16   4 :3  0:2:3:A:N
        38,  // F7: STAB a16   4 :3  0:2:3:B:N
        23,  // F8: EORB a16   4 :3  0:2:3:A:N
        23,  // F9: ADCB a16   4 :3  0:2:3:A:N
        23,  // FA: ORAB a16   4 :3  0:2:3:A:N
        23,  // FB: ADDB a16   4 :3  0:2:3:A:N
        37,  // FC: LDD  a16   5 :3  0:2:3:A:a:N
        40,  // FD: STD  a16   5 :3  0:2:3:B:b:N
        37,  // FE: LDX  a16   5 :3  0:2:3:A:a:N
        40,  // FF: STX  a16   5 :3  0:2:3:B:b:N
};
}  // namespace

const char *InstHd6301::instSequence(uint8_t inst) const {
    return SEQUENCES[INST_TABLE[inst]];
}

const char *InstHd6301::intrSequence() const {
    return INTERRUPT;
}

}  // namespace hd6301
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
