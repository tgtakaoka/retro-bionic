#include "inst_mb8861.h"

namespace debugger {
namespace mb8861 {

namespace {
/**
# 1: instruction code (next=&1)
# 2: instruction operands (disp, high(addr), ++next)
# 3: instruction operands (low(addr), ++next)
# 4: instruction operands
# 5: instruction operands
# 0: prefetched instruction code
# R: read 1 byte (high(addr))
# r: read 1 byte at address R+1 or r+1 (low(addr))
# W: write 1 byte, equals to R if exists or w-1
# w: write 1 byte at address W+1
# A: read 1 byte at address |addr|
# a: read 1 byte ar A+1
# D: read 1 byte from direct page (00|addr|)
# d: read 1 byte at address (D+1)
# B: write 1 byte at address addr
# b: write 1 byte ar B+1
# E: write 1 byte to direct page (00xx), equals to D if exists
# e: write 1 byte at address E+1
# N: next instruction read from |next|
# J: next instruction read from |addr|
# j: next instruction read from |next| or |next|+(8bit disp)
# k: next instruction read from |next| or |next|+(16bit disp)
# i: next instruction read from unknown
# V: read 1 byte from address FFF8-FFFE (high(addr))
# v: read 1 byte from address V+1 (low(addr))
# X: any valid read
# x: irrelevant read access to $FFFF
# Z: repeated irrelevant read accesses to $FFFF
# n: non VMA access
#
# Interrupt sequence
# 1:X:w:W:W:W:W:W:W:n:V:v:J
*/

constexpr const char INTERRUPT[] = "1XwWWWWWWnVvJ";

constexpr const char *const SEQUENCES[/*seq*/] = {
        "",               //  0
        "1NN",            //  1
        "1NnnN",          //  2
        "12nnj",          //  3
        "1NnRN",          //  4
        "1NWnN",          //  5
        "1NnRrJ",         //  6
        "1NnRrrrrrrJ",    //  7
        "1NwWWWWWWX",     //  8
        "1NwWWWWWWnVvJ",  //  9
        "12nnRnWN",       // 10
        "12nnRnnN",       // 11
        "12nni",          // 12
        "123AnBN",        // 13
        "123nnRnWN",      // 14
        "123nnRnN",       // 15
        "123AnnN",        // 16
        "123J",           // 17
        "12N",            // 18
        "123N",           // 19
        "12nwWnnnj",      // 20
        "12DN",           // 21
        "12nEN",          // 22
        "12DdN",          // 23
        "12nEeN",         // 24
        "12nnRN",         // 25
        "12nnnWN",        // 26
        "12nnRrN",        // 27
        "12nwWnnni",      // 28
        "12nnnWwN",       // 29
        "123AN",          // 30
        "123nBN",         // 31
        "123AaN",         // 32
        "123AwWnnXJ",     // 33
        "123nBbN",        // 34
        "12nnN",          // 35
        "123AannN",       // 36
};

constexpr uint8_t INST_TABLE[] = {
        0,   // 00: -    -     0 :0  -
        1,   // 01: NOP  -     2 :1  1:N:N
        0,   // 02: -    -     0 :0  -
        0,   // 03: -    -     0 :0  -
        0,   // 04: -    -     0 :0  -
        0,   // 05: -    -     0 :0  -
        1,   // 06: TAP  -     2 :1  1:N:N
        1,   // 07: TPA  -     2 :1  1:N:N
        2,   // 08: INX  -     4 :1  1:N:n:n:N
        2,   // 09: DEX  -     4 :1  1:N:n:n:N
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
        3,   // 20: BRA  r8    4 :2  1:2:n:n:j
        0,   // 21: -    -     0 :0  -
        3,   // 22: BHI  r8    4 :2  1:2:n:n:j
        3,   // 23: BLS  r8    4 :2  1:2:n:n:j
        3,   // 24: BCC  r8    4 :2  1:2:n:n:j
        3,   // 25: BCS  r8    4 :2  1:2:n:n:j
        3,   // 26: BNE  r8    4 :2  1:2:n:n:j
        3,   // 27: BEQ  r8    4 :2  1:2:n:n:j
        3,   // 28: BVC  r8    4 :2  1:2:n:n:j
        3,   // 29: BVS  r8    4 :2  1:2:n:n:j
        3,   // 2A: BPL  r8    4 :2  1:2:n:n:j
        3,   // 2B: BMI  r8    4 :2  1:2:n:n:j
        3,   // 2C: BGE  r8    4 :2  1:2:n:n:j
        3,   // 2D: BLT  r8    4 :2  1:2:n:n:j
        3,   // 2E: BGT  r8    4 :2  1:2:n:n:j
        3,   // 2F: BLE  r8    4 :2  1:2:n:n:j
        2,   // 30: TSX  -     4 :1  1:N:n:n:N
        2,   // 31: INS  -     4 :1  1:N:n:n:N
        4,   // 32: PULA -     4 :1  1:N:n:R:N
        4,   // 33: PULB -     4 :1  1:N:n:R:N
        2,   // 34: DES  -     4 :1  1:N:n:n:N
        2,   // 35: TXS  -     4 :1  1:N:n:n:N
        5,   // 36: PSHA -     4 :1  1:N:W:n:N
        5,   // 37: PSHB -     4 :1  1:N:W:n:N
        0,   // 38: -    -     0 :0  -
        6,   // 39: RTS  -     5 :1  1:N:n:R:r:J
        0,   // 3A: -    -     0 :0  -
        7,   // 3B: RTI  -     10:1  1:N:n:R:r:r:r:r:r:r:J
        0,   // 3C: -    -     0 :0  -
        0,   // 3D: -    -     0 :0  -
        8,   // 3E: WAI  -     9 :1  1:N:w:W:W:W:W:W:W:X
        9,   // 3F: SWI  -     12:1  1:N:w:W:W:W:W:W:W:n:V:v:J
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
        10,  // 60: NEG  n8,X  7 :2  1:2:n:n:R:n:W:N
        0,   // 61: -    -     0 :0  -
        0,   // 62: -    -     0 :0  -
        10,  // 63: COM  n8,X  7 :2  1:2:n:n:R:n:W:N
        10,  // 64: LSR  n8,X  7 :2  1:2:n:n:R:n:W:N
        0,   // 65: -    -     0 :0  -
        10,  // 66: ROR  n8,X  7 :2  1:2:n:n:R:n:W:N
        10,  // 67: ASR  n8,X  7 :2  1:2:n:n:R:n:W:N
        10,  // 68: ASL  n8,X  7 :2  1:2:n:n:R:n:W:N
        10,  // 69: ROL  n8,X  7 :2  1:2:n:n:R:n:W:N
        10,  // 6A: DEC  n8,X  7 :2  1:2:n:n:R:n:W:N
        0,   // 6B: -    -     0 :0  -
        10,  // 6C: INC  n8,X  7 :2  1:2:n:n:R:n:W:N
        11,  // 6D: TST  n8,X  7 :2  1:2:n:n:R:n:n:N
        12,  // 6E: JMP  n8,X  4 :2  1:2:n:n:i
        10,  // 6F: CLR  n8,X  7 :2  1:2:n:n:R:n:W:N
        13,  // 70: NEG  a16   6 :3  1:2:3:A:n:B:N
        14,  // 71: NIM  #,n8,X  8 :3  1:2:3:n:n:R:n:W:N
        14,  // 72: OIM  #,n8,X  8 :3  1:2:3:n:n:R:n:W:N
        13,  // 73: COM  a16   6 :3  1:2:3:A:n:B:N
        13,  // 74: LSR  a16   6 :3  1:2:3:A:n:B:N
        14,  // 75: XIM  #,n8,X  8 :3  1:2:3:n:n:R:n:W:N
        13,  // 76: ROR  a16   6 :3  1:2:3:A:n:B:N
        13,  // 77: ASR  a16   6 :3  1:2:3:A:n:B:N
        13,  // 78: ASL  a16   6 :3  1:2:3:A:n:B:N
        13,  // 79: ROL  a16   6 :3  1:2:3:A:n:B:N
        13,  // 7A: DEC  a16   6 :3  1:2:3:A:n:B:N
        15,  // 7B: TIM  #,n8,X  7 :3  1:2:3:n:n:R:n:N
        13,  // 7C: INC  a16   6 :3  1:2:3:A:n:B:N
        16,  // 7D: TST  a16   6 :3  1:2:3:A:n:n:N
        17,  // 7E: JMP  a16   3 :3  1:2:3:J
        13,  // 7F: CLR  a16   6 :3  1:2:3:A:n:B:N
        18,  // 80: SUBA #n8   2 :2  1:2:N
        18,  // 81: CMPA #n8   2 :2  1:2:N
        18,  // 82: SBCA #n8   2 :2  1:2:N
        0,   // 83: -    -     0 :0  -
        18,  // 84: ANDA #n8   2 :2  1:2:N
        18,  // 85: BITA #n8   2 :2  1:2:N
        18,  // 86: LDAA #n8   2 :2  1:2:N
        0,   // 87: -    -     0 :0  -
        18,  // 88: EORA #n8   2 :2  1:2:N
        18,  // 89: ADCA #n8   2 :2  1:2:N
        18,  // 8A: ORAA #n8   2 :2  1:2:N
        18,  // 8B: ADDA #n8   2 :2  1:2:N
        19,  // 8C: CPX  #n16  3 :3  1:2:3:N
        20,  // 8D: BSR  r8    8 :2  1:2:n:w:W:n:n:n:j
        19,  // 8E: LDS  #n16  3 :3  1:2:3:N
        0,   // 8F: -    -     0 :0  -
        21,  // 90: SUBA d8    3 :2  1:2:D:N
        21,  // 91: CMPA d8    3 :2  1:2:D:N
        21,  // 92: SBCA d8    3 :2  1:2:D:N
        0,   // 93: -    -     0 :0  -
        21,  // 94: ANDA d8    3 :2  1:2:D:N
        21,  // 95: BITA d8    3 :2  1:2:D:N
        21,  // 96: LDAA d8    3 :2  1:2:D:N
        22,  // 97: STAA d8    4 :2  1:2:n:E:N
        21,  // 98: EORA d8    3 :2  1:2:D:N
        21,  // 99: ADCA d8    3 :2  1:2:D:N
        21,  // 9A: ORAA d8    3 :2  1:2:D:N
        21,  // 9B: ADDA d8    3 :2  1:2:D:N
        23,  // 9C: CPX  d8    4 :2  1:2:D:d:N
        0,   // 9D: -    -     0 :0  -
        23,  // 9E: LDS  d8    4 :2  1:2:D:d:N
        24,  // 9F: STS  d8    5 :2  1:2:n:E:e:N
        25,  // A0: SUBA n8,X  5 :2  1:2:n:n:R:N
        25,  // A1: CMPA n8,X  5 :2  1:2:n:n:R:N
        25,  // A2: SBCA n8,X  5 :2  1:2:n:n:R:N
        0,   // A3: -    -     0 :0  -
        25,  // A4: ANDA n8,X  5 :2  1:2:n:n:R:N
        25,  // A5: BITA n8,X  5 :2  1:2:n:n:R:N
        25,  // A6: LDAA n8,X  5 :2  1:2:n:n:R:N
        26,  // A7: STAA n8,X  6 :2  1:2:n:n:n:W:N
        25,  // A8: EORA n8,X  5 :2  1:2:n:n:R:N
        25,  // A9: ADCA n8,X  5 :2  1:2:n:n:R:N
        25,  // AA: ORAA n8,X  5 :2  1:2:n:n:R:N
        25,  // AB: ADDA n8,X  5 :2  1:2:n:n:R:N
        27,  // AC: CPX  n8,X  6 :2  1:2:n:n:R:r:N
        28,  // AD: JSR  n8,X  8 :2  1:2:n:w:W:n:n:n:i
        27,  // AE: LDS  n8,X  6 :2  1:2:n:n:R:r:N
        29,  // AF: STS  n8,X  7 :2  1:2:n:n:n:W:w:N
        30,  // B0: SUBA a16   4 :3  1:2:3:A:N
        30,  // B1: CMPA a16   4 :3  1:2:3:A:N
        30,  // B2: SBCA a16   4 :3  1:2:3:A:N
        0,   // B3: -    -     0 :0  -
        30,  // B4: ANDA a16   4 :3  1:2:3:A:N
        30,  // B5: BITA a16   4 :3  1:2:3:A:N
        30,  // B6: LDAA a16   4 :3  1:2:3:A:N
        31,  // B7: STAA a16   5 :3  1:2:3:n:B:N
        30,  // B8: EORA a16   4 :3  1:2:3:A:N
        30,  // B9: ADCA a16   4 :3  1:2:3:A:N
        30,  // BA: ORAA a16   4 :3  1:2:3:A:N
        30,  // BB: ADDA a16   4 :3  1:2:3:A:N
        32,  // BC: CPX  a16   5 :3  1:2:3:A:a:N
        33,  // BD: JSR  a16   9 :3  1:2:3:A:w:W:n:n:X:J
        32,  // BE: LDS  a16   5 :3  1:2:3:A:a:N
        34,  // BF: STS  a16   6 :3  1:2:3:n:B:b:N
        18,  // C0: SUBB #n8   2 :2  1:2:N
        18,  // C1: CMPB #n8   2 :2  1:2:N
        18,  // C2: SBCB #n8   2 :2  1:2:N
        0,   // C3: -    -     0 :0  -
        18,  // C4: ANDB #n8   2 :2  1:2:N
        18,  // C5: BITB #n8   2 :2  1:2:N
        18,  // C6: LDAB #n8   2 :2  1:2:N
        0,   // C7: -    -     0 :0  -
        18,  // C8: EORB #n8   2 :2  1:2:N
        18,  // C9: ADCB #n8   2 :2  1:2:N
        18,  // CA: ORAB #n8   2 :2  1:2:N
        18,  // CB: ADDB #n8   2 :2  1:2:N
        0,   // CC: -    -     0 :0  -
        0,   // CD: -    -     0 :0  -
        19,  // CE: LDX  #n16  3 :3  1:2:3:N
        0,   // CF: -    -     0 :0  -
        21,  // D0: SUBB d8    3 :2  1:2:D:N
        21,  // D1: CMPB d8    3 :2  1:2:D:N
        21,  // D2: SBCB d8    3 :2  1:2:D:N
        0,   // D3: -    -     0 :0  -
        21,  // D4: ANDB d8    3 :2  1:2:D:N
        21,  // D5: BITB d8    3 :2  1:2:D:N
        21,  // D6: LDAB d8    3 :2  1:2:D:N
        22,  // D7: STAB d8    4 :2  1:2:n:E:N
        21,  // D8: EORB d8    3 :2  1:2:D:N
        21,  // D9: ADCB d8    3 :2  1:2:D:N
        21,  // DA: ORAB d8    3 :2  1:2:D:N
        21,  // DB: ADDB d8    3 :2  1:2:D:N
        0,   // DC: -    -     0 :0  -
        0,   // DD: -    -     0 :0  -
        23,  // DE: LDX  d8    4 :2  1:2:D:d:N
        24,  // DF: STX  d8    5 :2  1:2:n:E:e:N
        25,  // E0: SUBB n8,X  5 :2  1:2:n:n:R:N
        25,  // E1: CMPB n8,X  5 :2  1:2:n:n:R:N
        25,  // E2: SBCB n8,X  5 :2  1:2:n:n:R:N
        0,   // E3: -    -     0 :0  -
        25,  // E4: ANDB n8,X  5 :2  1:2:n:n:R:N
        25,  // E5: BITB n8,X  5 :2  1:2:n:n:R:N
        25,  // E6: LDAB n8,X  5 :2  1:2:n:n:R:N
        26,  // E7: STAB n8,X  6 :2  1:2:n:n:n:W:N
        25,  // E8: EORB n8,X  5 :2  1:2:n:n:R:N
        25,  // E9: ADCB n8,X  5 :2  1:2:n:n:R:N
        25,  // EA: ORAB n8,X  5 :2  1:2:n:n:R:N
        25,  // EB: ADDB n8,X  5 :2  1:2:n:n:R:N
        35,  // EC: ADX  #n8   4 :2  1:2:n:n:N
        0,   // ED: -    -     0 :0  -
        27,  // EE: LDX  n8,X  6 :2  1:2:n:n:R:r:N
        29,  // EF: STX  n8,X  7 :2  1:2:n:n:n:W:w:N
        30,  // F0: SUBB a16   4 :3  1:2:3:A:N
        30,  // F1: CMPB a16   4 :3  1:2:3:A:N
        30,  // F2: SBCB a16   4 :3  1:2:3:A:N
        0,   // F3: -    -     0 :0  -
        30,  // F4: ANDB a16   4 :3  1:2:3:A:N
        30,  // F5: BITB a16   4 :3  1:2:3:A:N
        30,  // F6: LDAB a16   4 :3  1:2:3:A:N
        31,  // F7: STAB a16   5 :3  1:2:3:n:B:N
        30,  // F8: EORB a16   4 :3  1:2:3:A:N
        30,  // F9: ADCB a16   4 :3  1:2:3:A:N
        30,  // FA: ORAB a16   4 :3  1:2:3:A:N
        30,  // FB: ADDB a16   4 :3  1:2:3:A:N
        36,  // FC: ADX  a16   7 :3  1:2:3:A:a:n:n:N
        0,   // FD: -    -     0 :0  -
        32,  // FE: LDX  a16   5 :3  1:2:3:A:a:N
        34,  // FF: STX  a16   6 :3  1:2:3:n:B:b:N
};
}  // namespace

const char *InstMb8861::instSequence(uint8_t inst) const {
    return SEQUENCES[INST_TABLE[inst]];
}

const char *InstMb8861::intrSequence() const {
    return INTERRUPT;
}

}  // namespace mb8861
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
