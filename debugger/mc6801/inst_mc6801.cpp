#include "inst_mc6801.h"
#include "mems_mc6801.h"

namespace debugger {
namespace mc6801 {

struct InstMc6801 Inst {
    Memory
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
# 1:X:w:W:W:W:W:W:W:X:V:v:J
*/

constexpr const char *INTERRUPT = "1XwWWWWWWXVvJ";

constexpr const char *const SEQUENCES[/*seq*/] = {
        "",               //  0
        "1NN",            //  1
        "1NxN",           //  2
        "12xj",           //  3
        "1NXN",           //  4
        "1NXRN",          //  5
        "1NWN",           //  6
        "1NXRrN",         //  7
        "1NXRrJ",         //  8
        "1NXRrrrrrrJ",    //  9
        "1NwWN",          // 10
        "1NxxxxxxxxN",    // 11
        "1NwWWWWWWX",     // 12
        "1NwWWWWWWXVvJ",  // 13
        "12xRxWN",        // 14
        "12xRxxN",        // 15
        "12xi",           // 16
        "123AxBN",        // 17
        "123AxxN",        // 18
        "123J",           // 19
        "12N",            // 20
        "123xN",          // 21
        "123N",           // 22
        "12xXwWj",        // 23
        "12DN",           // 24
        "12DdxN",         // 25
        "12EN",           // 26
        "12DwWi",         // 27
        "12DdN",          // 28
        "12EeN",          // 29
        "12xRN",          // 30
        "12xRrxN",        // 31
        "12xWN",          // 32
        "12xXwWi",        // 33
        "12xRrN",         // 34
        "12xWwN",         // 35
        "123AN",          // 36
        "123AaxN",        // 37
        "123BN",          // 38
        "123AwWJ",        // 39
        "123AaN",         // 40
        "123BbN",         // 41
};

constexpr uint8_t INST_TABLE[] = {
        0,   // 00: -    -     0 :0  -
        1,   // 01: NOP  -     2 :1  1:N:N
        0,   // 02: -    -     0 :0  -
        0,   // 03: -    -     0 :0  -
        2,   // 04: LSRD -     3 :1  1:N:x:N
        2,   // 05: ASLD -     3 :1  1:N:x:N
        1,   // 06: TAP  -     2 :1  1:N:N
        1,   // 07: TPA  -     2 :1  1:N:N
        2,   // 08: INX  -     3 :1  1:N:x:N
        2,   // 09: DEX  -     3 :1  1:N:x:N
        1,   // 0A: CLV  -     2 :1  1:N:N
        1,   // 0B: SEV  -     2 :1  1:N:N
        1,   // 0C: CLC  -     2 :1  1:N:N
        1,   // 0D: SEC  -     2 :1  1:N:N
        1,   // 0E: CLI  -     2 :1  1:N:N
        1,   // 0F: SEI  -     2 :1  1:N:N
        1,   // 10: SBA  -     2 :1  1:N:N
        1,   // 11: CBA  -     2 :1  1:N:N
        0,   // 12: -    -     0 :0  -
        0,   // 13: -    -     0 :0  -
        0,   // 14: -    -     0 :0  -
        0,   // 15: -    -     0 :0  -
        1,   // 16: TAB  -     2 :1  1:N:N
        1,   // 17: TBA  -     2 :1  1:N:N
        0,   // 18: -    -     0 :0  -
        1,   // 19: DAA  -     2 :1  1:N:N
        0,   // 1A: -    -     0 :0  -
        1,   // 1B: ABA  -     2 :1  1:N:N
        0,   // 1C: -    -     0 :0  -
        0,   // 1D: -    -     0 :0  -
        0,   // 1E: -    -     0 :0  -
        0,   // 1F: -    -     0 :0  -
        3,   // 20: BRA  r8    3 :2  1:2:x:j
        3,   // 21: BRN  r8    3 :2  1:2:x:j
        3,   // 22: BHI  r8    3 :2  1:2:x:j
        3,   // 23: BLS  r8    3 :2  1:2:x:j
        3,   // 24: BCC  r8    3 :2  1:2:x:j
        3,   // 25: BCS  r8    3 :2  1:2:x:j
        3,   // 26: BNE  r8    3 :2  1:2:x:j
        3,   // 27: BEQ  r8    3 :2  1:2:x:j
        3,   // 28: BVC  r8    3 :2  1:2:x:j
        3,   // 29: BVS  r8    3 :2  1:2:x:j
        3,   // 2A: BPL  r8    3 :2  1:2:x:j
        3,   // 2B: BMI  r8    3 :2  1:2:x:j
        3,   // 2C: BGE  r8    3 :2  1:2:x:j
        3,   // 2D: BLT  r8    3 :2  1:2:x:j
        3,   // 2E: BGT  r8    3 :2  1:2:x:j
        3,   // 2F: BLE  r8    3 :2  1:2:x:j
        4,   // 30: TSX  -     3 :1  1:N:X:N
        4,   // 31: INS  -     3 :1  1:N:X:N
        5,   // 32: PULA -     4 :1  1:N:X:R:N
        5,   // 33: PULB -     4 :1  1:N:X:R:N
        4,   // 34: DES  -     3 :1  1:N:X:N
        2,   // 35: TXS  -     3 :1  1:N:x:N
        6,   // 36: PSHA -     3 :1  1:N:W:N
        6,   // 37: PSHB -     3 :1  1:N:W:N
        7,   // 38: PULX -     5 :1  1:N:X:R:r:N
        8,   // 39: RTS  -     5 :1  1:N:X:R:r:J
        2,   // 3A: ABX  -     3 :1  1:N:x:N
        9,   // 3B: RTI  -     10:1  1:N:X:R:r:r:r:r:r:r:J
        10,  // 3C: PSHX -     4 :1  1:N:w:W:N
        11,  // 3D: MUL  -     10:1  1:N:x:x:x:x:x:x:x:x:N
        12,  // 3E: WAI  -     9 :1  1:N:w:W:W:W:W:W:W:X
        13,  // 3F: SWI  -     12:1  1:N:w:W:W:W:W:W:W:X:V:v:J
        1,   // 40: NEGA -     2 :1  1:N:N
        0,   // 41: -    -     0 :0  -
        0,   // 42: -    -     0 :0  -
        1,   // 43: COMA -     2 :1  1:N:N
        1,   // 44: LSRA -     2 :1  1:N:N
        0,   // 45: -    -     0 :0  -
        1,   // 46: RORA -     2 :1  1:N:N
        1,   // 47: ASRA -     2 :1  1:N:N
        1,   // 48: ASLA -     2 :1  1:N:N
        1,   // 49: ROLA -     2 :1  1:N:N
        1,   // 4A: DECA -     2 :1  1:N:N
        0,   // 4B: -    -     0 :0  -
        1,   // 4C: INCA -     2 :1  1:N:N
        1,   // 4D: TSTA -     2 :1  1:N:N
        0,   // 4E: -    -     0 :0  -
        1,   // 4F: CLRA -     2 :1  1:N:N
        1,   // 50: NEGB -     2 :1  1:N:N
        0,   // 51: -    -     0 :0  -
        0,   // 52: -    -     0 :0  -
        1,   // 53: COMB -     2 :1  1:N:N
        1,   // 54: LSRB -     2 :1  1:N:N
        0,   // 55: -    -     0 :0  -
        1,   // 56: RORB -     2 :1  1:N:N
        1,   // 57: ASRB -     2 :1  1:N:N
        1,   // 58: ASLB -     2 :1  1:N:N
        1,   // 59: ROLB -     2 :1  1:N:N
        1,   // 5A: DECB -     2 :1  1:N:N
        0,   // 5B: -    -     0 :0  -
        1,   // 5C: INCB -     2 :1  1:N:N
        1,   // 5D: TSTB -     2 :1  1:N:N
        0,   // 5E: -    -     0 :0  -
        1,   // 5F: CLRB -     2 :1  1:N:N
        14,  // 60: NEG  n8,X  6 :2  1:2:x:R:x:W:N
        0,   // 61: -    -     0 :0  -
        0,   // 62: -    -     0 :0  -
        14,  // 63: COM  n8,X  6 :2  1:2:x:R:x:W:N
        14,  // 64: LSR  n8,X  6 :2  1:2:x:R:x:W:N
        0,   // 65: -    -     0 :0  -
        14,  // 66: ROR  n8,X  6 :2  1:2:x:R:x:W:N
        14,  // 67: ASR  n8,X  6 :2  1:2:x:R:x:W:N
        14,  // 68: ASL  n8,X  6 :2  1:2:x:R:x:W:N
        14,  // 69: ROL  n8,X  6 :2  1:2:x:R:x:W:N
        14,  // 6A: DEC  n8,X  6 :2  1:2:x:R:x:W:N
        0,   // 6B: -    -     0 :0  -
        14,  // 6C: INC  n8,X  6 :2  1:2:x:R:x:W:N
        15,  // 6D: TST  n8,X  6 :2  1:2:x:R:x:x:N
        16,  // 6E: JMP  n8,X  3 :2  1:2:x:i
        14,  // 6F: CLR  n8,X  6 :2  1:2:x:R:x:W:N
        17,  // 70: NEG  a16   6 :3  1:2:3:A:x:B:N
        0,   // 71: -    -     0 :0  -
        0,   // 72: -    -     0 :0  -
        17,  // 73: COM  a16   6 :3  1:2:3:A:x:B:N
        17,  // 74: LSR  a16   6 :3  1:2:3:A:x:B:N
        0,   // 75: -    -     0 :0  -
        17,  // 76: ROR  a16   6 :3  1:2:3:A:x:B:N
        17,  // 77: ASR  a16   6 :3  1:2:3:A:x:B:N
        17,  // 78: ASL  a16   6 :3  1:2:3:A:x:B:N
        17,  // 79: ROL  a16   6 :3  1:2:3:A:x:B:N
        17,  // 7A: DEC  a16   6 :3  1:2:3:A:x:B:N
        0,   // 7B: -    -     0 :0  -
        17,  // 7C: INC  a16   6 :3  1:2:3:A:x:B:N
        18,  // 7D: TST  a16   6 :3  1:2:3:A:x:x:N
        19,  // 7E: JMP  a16   3 :3  1:2:3:J
        17,  // 7F: CLR  a16   6 :3  1:2:3:A:x:B:N
        20,  // 80: SUBA #n8   2 :2  1:2:N
        20,  // 81: CMPA #n8   2 :2  1:2:N
        20,  // 82: SBCA #n8   2 :2  1:2:N
        21,  // 83: SUBD #n16  4 :3  1:2:3:x:N
        20,  // 84: ANDA #n8   2 :2  1:2:N
        20,  // 85: BITA #n8   2 :2  1:2:N
        20,  // 86: LDAA #n8   2 :2  1:2:N
        0,   // 87: -    -     0 :0  -
        20,  // 88: EORA #n8   2 :2  1:2:N
        20,  // 89: ADCA #n8   2 :2  1:2:N
        20,  // 8A: ORAA #n8   2 :2  1:2:N
        20,  // 8B: ADDA #n8   2 :2  1:2:N
        22,  // 8C: CPX  #n16  3 :3  1:2:3:N
        23,  // 8D: BSR  r8    6 :2  1:2:x:X:w:W:j
        22,  // 8E: LDS  #n16  3 :3  1:2:3:N
        0,   // 8F: -    -     0 :0  -
        24,  // 90: SUBA d8    3 :2  1:2:D:N
        24,  // 91: CMPA d8    3 :2  1:2:D:N
        24,  // 92: SBCA d8    3 :2  1:2:D:N
        25,  // 93: SUBD d8    5 :2  1:2:D:d:x:N
        24,  // 94: ANDA d8    3 :2  1:2:D:N
        24,  // 95: BITA d8    3 :2  1:2:D:N
        24,  // 96: LDAA d8    3 :2  1:2:D:N
        26,  // 97: STAA d8    3 :2  1:2:E:N
        24,  // 98: EORA d8    3 :2  1:2:D:N
        24,  // 99: ADCA d8    3 :2  1:2:D:N
        24,  // 9A: ORAA d8    3 :2  1:2:D:N
        24,  // 9B: ADDA d8    3 :2  1:2:D:N
        25,  // 9C: CPX  d8    5 :2  1:2:D:d:x:N
        27,  // 9D: JSR  d8    5 :2  1:2:D:w:W:i
        28,  // 9E: LDS  d8    4 :2  1:2:D:d:N
        29,  // 9F: STS  d8    4 :2  1:2:E:e:N
        30,  // A0: SUBA n8,X  4 :2  1:2:x:R:N
        30,  // A1: CMPA n8,X  4 :2  1:2:x:R:N
        30,  // A2: SBCA n8,X  4 :2  1:2:x:R:N
        31,  // A3: SUBD n8,X  6 :2  1:2:x:R:r:x:N
        30,  // A4: ANDA n8,X  4 :2  1:2:x:R:N
        30,  // A5: BITA n8,X  4 :2  1:2:x:R:N
        30,  // A6: LDAA n8,X  4 :2  1:2:x:R:N
        32,  // A7: STAA n8,X  4 :2  1:2:x:W:N
        30,  // A8: EORA n8,X  4 :2  1:2:x:R:N
        30,  // A9: ADCA n8,X  4 :2  1:2:x:R:N
        30,  // AA: ORAA n8,X  4 :2  1:2:x:R:N
        30,  // AB: ADDA n8,X  4 :2  1:2:x:R:N
        31,  // AC: CPX  n8,X  6 :2  1:2:x:R:r:x:N
        33,  // AD: JSR  n8,X  6 :2  1:2:x:X:w:W:i
        34,  // AE: LDS  n8,X  5 :2  1:2:x:R:r:N
        35,  // AF: STS  n8,X  5 :2  1:2:x:W:w:N
        36,  // B0: SUBA a16   4 :3  1:2:3:A:N
        36,  // B1: CMPA a16   4 :3  1:2:3:A:N
        36,  // B2: SBCA a16   4 :3  1:2:3:A:N
        37,  // B3: SUBD a16   6 :3  1:2:3:A:a:x:N
        36,  // B4: ANDA a16   4 :3  1:2:3:A:N
        36,  // B5: BITA a16   4 :3  1:2:3:A:N
        36,  // B6: LDAA a16   4 :3  1:2:3:A:N
        38,  // B7: STAA a16   4 :3  1:2:3:B:N
        36,  // B8: EORA a16   4 :3  1:2:3:A:N
        36,  // B9: ADCA a16   4 :3  1:2:3:A:N
        36,  // BA: ORAA a16   4 :3  1:2:3:A:N
        36,  // BB: ADDA a16   4 :3  1:2:3:A:N
        37,  // BC: CPX  a16   6 :3  1:2:3:A:a:x:N
        39,  // BD: JSR  a16   6 :3  1:2:3:A:w:W:J
        40,  // BE: LDS  a16   5 :3  1:2:3:A:a:N
        41,  // BF: STS  a16   5 :3  1:2:3:B:b:N
        20,  // C0: SUBB #n8   2 :2  1:2:N
        20,  // C1: CMPB #n8   2 :2  1:2:N
        20,  // C2: SBCB #n8   2 :2  1:2:N
        21,  // C3: ADDD #n16  4 :3  1:2:3:x:N
        20,  // C4: ANDB #n8   2 :2  1:2:N
        20,  // C5: BITB #n8   2 :2  1:2:N
        20,  // C6: LDAB #n8   2 :2  1:2:N
        0,   // C7: -    -     0 :0  -
        20,  // C8: EORB #n8   2 :2  1:2:N
        20,  // C9: ADCB #n8   2 :2  1:2:N
        20,  // CA: ORAB #n8   2 :2  1:2:N
        20,  // CB: ADDB #n8   2 :2  1:2:N
        22,  // CC: LDD  #n16  3 :3  1:2:3:N
        0,   // CD: -    -     0 :0  -
        22,  // CE: LDX  #n16  3 :3  1:2:3:N
        0,   // CF: -    -     0 :0  -
        24,  // D0: SUBB d8    3 :2  1:2:D:N
        24,  // D1: CMPB d8    3 :2  1:2:D:N
        24,  // D2: SBCB d8    3 :2  1:2:D:N
        25,  // D3: ADDD d8    5 :2  1:2:D:d:x:N
        24,  // D4: ANDB d8    3 :2  1:2:D:N
        24,  // D5: BITB d8    3 :2  1:2:D:N
        24,  // D6: LDAB d8    3 :2  1:2:D:N
        26,  // D7: STAB d8    3 :2  1:2:E:N
        24,  // D8: EORB d8    3 :2  1:2:D:N
        24,  // D9: ADCB d8    3 :2  1:2:D:N
        24,  // DA: ORAB d8    3 :2  1:2:D:N
        24,  // DB: ADDB d8    3 :2  1:2:D:N
        28,  // DC: LDD  d8    4 :2  1:2:D:d:N
        29,  // DD: STD  d8    4 :2  1:2:E:e:N
        28,  // DE: LDX  d8    4 :2  1:2:D:d:N
        29,  // DF: STX  d8    4 :2  1:2:E:e:N
        30,  // E0: SUBB n8,X  4 :2  1:2:x:R:N
        30,  // E1: CMPB n8,X  4 :2  1:2:x:R:N
        30,  // E2: SBCB n8,X  4 :2  1:2:x:R:N
        31,  // E3: ADDD n8,X  6 :2  1:2:x:R:r:x:N
        30,  // E4: ANDB n8,X  4 :2  1:2:x:R:N
        30,  // E5: BITB n8,X  4 :2  1:2:x:R:N
        30,  // E6: LDAB n8,X  4 :2  1:2:x:R:N
        32,  // E7: STAB n8,X  4 :2  1:2:x:W:N
        30,  // E8: EORB n8,X  4 :2  1:2:x:R:N
        30,  // E9: ADCB n8,X  4 :2  1:2:x:R:N
        30,  // EA: ORAB n8,X  4 :2  1:2:x:R:N
        30,  // EB: ADDB n8,X  4 :2  1:2:x:R:N
        34,  // EC: LDD  n8,X  5 :2  1:2:x:R:r:N
        35,  // ED: STD  n8,X  5 :2  1:2:x:W:w:N
        34,  // EE: LDX  n8,X  5 :2  1:2:x:R:r:N
        35,  // EF: STX  n8,X  5 :2  1:2:x:W:w:N
        36,  // F0: SUBB a16   4 :3  1:2:3:A:N
        36,  // F1: CMPB a16   4 :3  1:2:3:A:N
        36,  // F2: SBCB a16   4 :3  1:2:3:A:N
        37,  // F3: ADDD a16   6 :3  1:2:3:A:a:x:N
        36,  // F4: ANDB a16   4 :3  1:2:3:A:N
        36,  // F5: BITB a16   4 :3  1:2:3:A:N
        36,  // F6: LDAB a16   4 :3  1:2:3:A:N
        38,  // F7: STAB a16   4 :3  1:2:3:B:N
        36,  // F8: EORB a16   4 :3  1:2:3:A:N
        36,  // F9: ADCB a16   4 :3  1:2:3:A:N
        36,  // FA: ORAB a16   4 :3  1:2:3:A:N
        36,  // FB: ADDB a16   4 :3  1:2:3:A:N
        40,  // FC: LDD  a16   5 :3  1:2:3:A:a:N
        41,  // FD: STD  a16   5 :3  1:2:3:B:b:N
        40,  // FE: LDX  a16   5 :3  1:2:3:A:a:N
        41,  // FF: STX  a16   5 :3  1:2:3:B:b:N
};
}  // namespace

const char *InstMc6801::instSequence(uint8_t inst) const {
    return SEQUENCES[INST_TABLE[inst]];
}

const char *InstMc6801::intrSequence() const {
    return INTERRUPT;
}

}  // namespace mc6801
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
