#include "inst_mc68hc08.h"
#include <string.h>
#include "debugger.h"
#include "mems_mc6805.h"

/**
 * from "mc6805/inst_mc68hc08.awk"
 *
# 1: instruction code (next=&1)
# 2: instruction operands (disp, high(addr), ++next)
# 3: instruction operands (low(addr), ++next)
# R: read 1 byte (high(addr))
# r: read 1 byte at address R+1 or r+1 (low(addr))
# W: write 1 byte, equals to R if exists or w-1
# w: write 1 byte at address W+1
# A: read 1 byte at address |addr|
# D: read 1 byte from direct page (00|disp|)
# B: write 1 byte at address addr
# E: write 1 byte to direct page (00xx), equals to D if exists
# N: next instruction read from |next|
# J: next instruction read from |addr|
# j: next instruction read from |next| or |next|+disp
# i: next instruction read from unknown
# V: read 1 byte from address FFF8-FFFE (high(addr))
# v: read 1 byte from address V+1 (low(addr))
# d: read 1 byte from previousy address
#
# Interrupt sequence
# 0:d:w:W:W:W:W:V:v:J
*/

namespace debugger {
namespace mc68hc08 {

constexpr const char *const SEQUENCES[/*seq*/] = {
        "",            //  0
        "rrrrrN",      //  1
        "rrrwN",       //  2
        "rrrN",        //  3
        "rrwwN",       //  4
        "rrrrwN",      //  5
        "rrwN",        //  6
        "rN",          //  7
        "rrrrN",       //  8
        "rNrrrr",      //  9
        "rNNNrrrr",    // 10
        "rrNrw",       // 11
        "rNrr",        // 12
        "rrNr",        // 13
        "rrNw",        // 14
        "rNrw",        // 15
        "rNr",         // 16
        "rNw",         // 17
        "rrrrrrrN",    // 18
        "rrwwwwwVrN",  // 19
        "rNN",         // 20
        "rrN",         // 21
        "rrrwwN",      // 22
        "rrrNr",       // 23
        "rrrNw",       // 24
        "rrrwwrN",     // 25
        "rrwwrN",      // 26
        "rrrNrw",      // 27
        "rrrrrrN",     // 28
        "rrrrrwN",     // 29
        "rrrrNr",      // 30
        "rrrrNw",      // 31
};

constexpr uint8_t P00_TABLE[] = {
        1,   // 00: BRSET 0,a8,r8  3 5 0:2:D:3:d:j
        1,   // 01: BRCLR 0,a8,r8  3 5 0:2:D:3:d:j
        1,   // 02: BRSET 1,a8,r8  3 5 0:2:D:3:d:j
        1,   // 03: BRCLR 1,a8,r8  3 5 0:2:D:3:d:j
        1,   // 04: BRSET 2,a8,r8  3 5 0:2:D:3:d:j
        1,   // 05: BRCLR 2,a8,r8  3 5 0:2:D:3:d:j
        1,   // 06: BRSET 3,a8,r8  3 5 0:2:D:3:d:j
        1,   // 07: BRCLR 3,a8,r8  3 5 0:2:D:3:d:j
        1,   // 08: BRSET 4,a8,r8  3 5 0:2:D:3:d:j
        1,   // 09: BRCLR 4,a8,r8  3 5 0:2:D:3:d:j
        1,   // 0A: BRSET 5,a8,r8  3 5 0:2:D:3:d:j
        1,   // 0B: BRCLR 5,a8,r8  3 5 0:2:D:3:d:j
        1,   // 0C: BRSET 6,a8,r8  3 5 0:2:D:3:d:j
        1,   // 0D: BRCLR 6,a8,r8  3 5 0:2:D:3:d:j
        1,   // 0E: BRSET 7,a8,r8  3 5 0:2:D:3:d:j
        1,   // 0F: BRCLR 7,a8,r8  3 5 0:2:D:3:d:j
        2,   // 10: BSET  0,a8     2 4 0:2:D:E:N
        2,   // 11: BCLR  0,a8     2 4 0:2:D:E:N
        2,   // 12: BSET  1,a8     2 4 0:2:D:E:N
        2,   // 13: BCLR  1,a8     2 4 0:2:D:E:N
        2,   // 14: BSET  2,a8     2 4 0:2:D:E:N
        2,   // 15: BCLR  2,a8     2 4 0:2:D:E:N
        2,   // 16: BSET  3,a8     2 4 0:2:D:E:N
        2,   // 17: BCLR  3,a8     2 4 0:2:D:E:N
        2,   // 18: BSET  4,a8     2 4 0:2:D:E:N
        2,   // 19: BCLR  4,a8     2 4 0:2:D:E:N
        2,   // 1A: BSET  5,a8     2 4 0:2:D:E:N
        2,   // 1B: BCLR  5,a8     2 4 0:2:D:E:N
        2,   // 1C: BSET  6,a8     2 4 0:2:D:E:N
        2,   // 1D: BCLR  6,a8     2 4 0:2:D:E:N
        2,   // 1E: BSET  7,a8     2 4 0:2:D:E:N
        2,   // 1F: BCLR  7,a8     2 4 0:2:D:E:N
        3,   // 20: BRA   r8       2 3 0:2:d:j
        3,   // 21: BRN   r8       2 3 0:2:d:N
        3,   // 22: BHI   r8       2 3 0:2:d:j
        3,   // 23: BLS   r8       2 3 0:2:d:j
        3,   // 24: BCC   r8       2 3 0:2:d:j
        3,   // 25: BCS   r8       2 3 0:2:d:j
        3,   // 26: BNE   r8       2 3 0:2:d:j
        3,   // 27: BEQ   r8       2 3 0:2:d:j
        3,   // 28: BHCC  r8       2 3 0:2:d:j
        3,   // 29: BHCS  r8       2 3 0:2:d:j
        3,   // 2A: BPL   r8       2 3 0:2:d:j
        3,   // 2B: BMI   r8       2 3 0:2:d:j
        3,   // 2C: BMC   r8       2 3 0:2:d:j
        3,   // 2D: BMS   r8       2 3 0:2:d:j
        3,   // 2E: BIL   r8       2 3 0:2:d:j
        3,   // 2F: BIH   r8       2 3 0:2:d:j
        2,   // 30: NEG   a8       2 4 0:2:D:E:N
        1,   // 31: CBEQ  a8,r8    3 5 0:2:3:D:d:N
        0,   // 32: -     -        - - -
        2,   // 33: COM   a8       2 4 0:2:D:E:N
        2,   // 34: LSR   a8       2 4 0:2:D:E:N
        4,   // 35: STHX  a8       2 4 0:2:E:w:N
        2,   // 36: ROR   a8       2 4 0:2:D:E:N
        2,   // 37: ASR   a8       2 4 0:2:D:E:N
        2,   // 38: ASL   a8       2 4 0:2:D:E:N
        2,   // 39: ROL   a8       2 4 0:2:D:E:N
        2,   // 3A: DEC   a8       2 4 0:2:D:E:N
        5,   // 3B: DBNZ  a8,r8    3 5 0:2:3:D:E:j
        2,   // 3C: INC   a8       2 4 0:2:D:E:N
        3,   // 3D: TST   a8       2 3 0:2:D:N
        0,   // 3E: -     -        - - -
        6,   // 3F: CLR   a8       2 3 0:2:E:N
        7,   // 40: NEGA  -        1 1 0:N
        8,   // 41: CBEQA #n8,r8   3 4 0:2:3:d:N
        9,   // 42: MUL   -        1 5 0:N:d:d:d:d
        7,   // 43: COMA  -        1 1 0:N
        7,   // 44: LSRA  -        1 1 0:N
        3,   // 45: LDHX  #n16     3 3 0:2:3:N
        7,   // 46: RORA  -        1 1 0:N
        7,   // 47: ASRA  -        1 1 0:N
        7,   // 48: ASLA  -        1 1 0:N
        7,   // 49: ROLA  -        1 1 0:N
        7,   // 4A: DECA  -        1 1 0:N
        3,   // 4B: DBNZA r8       2 3 0:2:d:j
        7,   // 4C: INCA  -        1 1 0:N
        7,   // 4D: TSTA  -        1 1 0:N
        5,   // 4E: MOV   a8,a8    3 5 0:2:D:3:E:N
        7,   // 4F: CLRA  -        1 1 0:N
        7,   // 50: NEGX  -        1 1 0:N
        8,   // 51: CBEQX #n8,r8   3 4 0:2:3:d:N
        10,  // 52: DIV   -        1 7 0:N:N:N:d:d:d:d
        7,   // 53: COMX  -        1 1 0:N
        7,   // 54: LSRX  -        1 1 0:N
        8,   // 55: LDHX  a8       2 4 0:2:D:r:N
        7,   // 56: RORX  -        1 1 0:N
        7,   // 57: ASRX  -        1 1 0:N
        7,   // 58: ASLX  -        1 1 0:N
        7,   // 59: ROLX  -        1 1 0:N
        7,   // 5A: DECX  -        1 1 0:N
        3,   // 5B: DBNZX r8       2 3 0:2:d:j
        7,   // 5C: INCX  -        1 1 0:N
        7,   // 5D: TSTX  -        1 1 0:N
        2,   // 5E: MOV   a8,X+    2 4 0:2:D:W:N
        7,   // 5F: CLRX  -        1 1 0:N
        11,  // 60: NEG   d8,X     2 4 0:2:N:R:W
        1,   // 61: CBEQ  d8,X+,r8 3 5 0:2:3:R:d:N
        12,  // 62: NSA   -        1 3 0:N:d:d
        11,  // 63: COM   d8,X     2 4 0:2:N:R:W
        11,  // 64: LSR   d8,X     2 4 0:2:N:R:W
        3,   // 65: CPHX  #n16     3 3 0:2:3:N
        11,  // 66: ROR   d8,X     2 4 0:2:N:R:W
        11,  // 67: ASR   d8,X     2 4 0:2:N:R:W
        11,  // 68: ASL   d8,X     2 4 0:2:N:R:W
        11,  // 69: ROL   d8,X     2 4 0:2:N:R:W
        11,  // 6A: DEC   d8,X     2 4 0:2:N:R:W
        5,   // 6B: DBNZ  d8,X,r8  3 5 0:2:3:R:W:j
        11,  // 6C: INC   d8,X     2 4 0:2:N:R:W
        13,  // 6D: TST   d8,X     2 3 0:2:N:R
        2,   // 6E: MOV   #n8,a8   3 4 0:2:3:E:N
        14,  // 6F: CLR   d8,X     2 3 0:2:N:W
        15,  // 70: NEG   ,X       1 3 0:N:R:W
        8,   // 71: CBEQ  X+,r8    2 4 0:2:R:d:N
        16,  // 72: DAA   -        1 2 0:N:d
        15,  // 73: COM   ,X       1 3 0:N:R:W
        15,  // 74: LSR   ,X       1 3 0:N:R:W
        8,   // 75: CPHX  a8       2 4 0:2:D:r:N
        15,  // 76: ROR   ,X       1 3 0:N:R:W
        15,  // 77: ASR   ,X       1 3 0:N:R:W
        15,  // 78: ASL   ,X       1 3 0:N:R:W
        15,  // 79: ROL   ,X       1 3 0:N:R:W
        15,  // 7A: DEC   ,X       1 3 0:N:R:W
        2,   // 7B: DBNZ  X,r8     2 4 0:2:R:W:j
        15,  // 7C: INC   ,X       1 3 0:N:R:W
        16,  // 7D: TST   ,X       1 2 0:N:R
        2,   // 7E: MOV   X+,d8    2 4 0:2:R:E:N
        17,  // 7F: CLR   ,X       1 2 0:N:W
        18,  // 80: RTI   -        1 7 0:d:R:r:r:r:r:J
        8,   // 81: RTS   -        1 4 0:d:R:r:J
        0,   // 82: -     -        - - -
        19,  // 83: SWI   -        1 9 0:d:w:W:W:W:W:V:v:J
        20,  // 84: TAP   -        1 2 0:N:N
        7,   // 85: TPA   -        1 1 0:N
        16,  // 86: PULA  -        1 2 0:N:R
        17,  // 87: PSHA  -        1 2 0:N:W
        16,  // 88: PULX  -        1 2 0:N:R
        17,  // 89: PSHX  -        1 2 0:N:W
        16,  // 8A: PULH  -        1 2 0:N:R
        17,  // 8B: PSHH  -        1 2 0:N:W
        7,   // 8C: CLRH  -        1 1 0:N
        0,   // 8D: -     -        - - -
        7,   // 8E: STOP  -        1 1 0:N
        7,   // 8F: WAIT  -        1 1 0:N
        3,   // 90: BGE   r8       2 3 0:2:d:j
        3,   // 91: BLT   r8       2 3 0:2:d:j
        3,   // 92: BGT   r8       2 3 0:2:d:j
        3,   // 93: BLE   r8       2 3 0:2:d:j
        16,  // 94: TXS   -        1 2 0:N:d
        16,  // 95: TSX   -        1 2 0:N:d
        0,   // 96: -     -        - - -
        7,   // 97: TAX   -        1 1 0:N
        7,   // 98: CLC   -        1 1 0:N
        7,   // 99: SEC   -        1 1 0:N
        16,  // 9A: CLI   -        1 2 0:N:d
        16,  // 9B: SEI   -        1 2 0:N:d
        7,   // 9C: RSP   -        1 1 0:N
        7,   // 9D: NOP   -        1 1 0:N
        0,   // 9E: P9E   -        - - -
        7,   // 9F: TXA   -        1 1 0:N
        21,  // A0: SUB   #n8      2 2 0:2:N
        21,  // A1: CMP   #n8      2 2 0:2:N
        21,  // A2: SBC   #n8      2 2 0:2:N
        21,  // A3: CPX   #n8      2 2 0:2:N
        21,  // A4: AND   #n8      2 2 0:2:N
        21,  // A5: BIT   #n8      2 2 0:2:N
        21,  // A6: LDA   #n8      2 2 0:2:N
        21,  // A7: AIS   #n8      2 2 0:2:N
        21,  // A8: EOR   #n8      2 2 0:2:N
        21,  // A9: ADC   #n8      2 2 0:2:N
        21,  // AA: ORA   #n8      2 2 0:2:N
        21,  // AB: ADD   #n8      2 2 0:2:N
        0,   // AC: -     -        - - -
        4,   // AD: BSR   r8       2 4 0:2:w:W:N
        21,  // AE: LDX   #n8      2 2 0:2:N
        21,  // AF: AIX   #n8      2 2 0:2:N
        3,   // B0: SUB   a8       2 3 0:2:D:N
        3,   // B1: CMP   a8       2 3 0:2:D:N
        3,   // B2: SBC   a8       2 3 0:2:D:N
        3,   // B3: CPX   a8       2 3 0:2:D:N
        3,   // B4: AND   a8       2 3 0:2:D:N
        3,   // B5: BIT   a8       2 3 0:2:D:N
        3,   // B6: LDA   a8       2 3 0:2:D:N
        6,   // B7: STA   a8       2 3 0:2:E:N
        3,   // B8: EOR   a8       2 3 0:2:D:N
        3,   // B9: ADC   a8       2 3 0:2:D:N
        3,   // BA: ORA   a8       2 3 0:2:D:N
        3,   // BB: ADD   a8       2 3 0:2:D:N
        21,  // BC: JMP   a8       2 2 0:2:J
        4,   // BD: JSR   a8       2 4 0:2:w:W:J
        3,   // BE: LDX   a8       2 3 0:2:D:N
        3,   // BF: STX   a8       2 3 0:2:D:N
        8,   // C0: SUB   a16      3 4 0:2:3:A:N
        8,   // C1: CMP   a16      3 4 0:2:3:A:N
        8,   // C2: SBC   a16      3 4 0:2:3:A:N
        8,   // C3: CPX   a16      3 4 0:2:3:A:N
        8,   // C4: AND   a16      3 4 0:2:3:A:N
        8,   // C5: BIT   a16      3 4 0:2:3:A:N
        8,   // C6: LDA   a16      3 4 0:2:3:A:N
        2,   // C7: STA   a16      3 4 0:2:3:B:N
        8,   // C8: EOR   a16      3 4 0:2:3:A:N
        8,   // C9: ADC   a16      3 4 0:2:3:A:N
        8,   // CA: ORA   a16      3 4 0:2:3:A:N
        8,   // CB: ADD   a16      3 4 0:2:3:A:N
        3,   // CC: JMP   a16      3 3 0:2:3:J
        22,  // CD: JSR   a16      3 5 0:2:3:w:W:J
        8,   // CE: LDX   a16      3 4 0:2:3:A:N
        2,   // CF: STX   a16      3 4 0:2:3:B:N
        23,  // D0: SUB   d16,X    3 4 0:2:3:N:R
        23,  // D1: CMP   d16,X    3 4 0:2:3:N:R
        23,  // D2: SBC   d16,X    3 4 0:2:3:N:R
        23,  // D3: CPX   d16,X    3 4 0:2:3:N:R
        23,  // D4: AND   d16,X    3 4 0:2:3:N:R
        23,  // D5: BIT   d16,X    3 4 0:2:3:N:R
        23,  // D6: LDA   d16,X    3 4 0:2:3:N:R
        24,  // D7: STA   d16,X    3 4 0:2:3:N:W
        23,  // D8: EOR   d16,X    3 4 0:2:3:N:R
        23,  // D9: ADC   d16,X    3 4 0:2:3:N:R
        23,  // DA: ORA   d16,X    3 4 0:2:3:N:R
        23,  // DB: ADD   d16,X    3 4 0:2:3:N:R
        8,   // DC: JMP   d16,X    3 4 0:2:3:d:i
        25,  // DD: JSR   d16,X    3 6 0:2:3:w:W:d:i
        23,  // DE: LDX   d16,X    3 4 0:2:3:N:R
        24,  // DF: STX   d16,X    3 4 0:2:3:N:W
        13,  // E0: SUB   d8,X     2 3 0:2:N:R
        13,  // E1: CMP   d8,X     2 3 0:2:N:R
        13,  // E2: SBC   d8,X     2 3 0:2:N:R
        13,  // E3: CPX   d8,X     2 3 0:2:N:R
        13,  // E4: AND   d8,X     2 3 0:2:N:R
        13,  // E5: BIT   d8,X     2 3 0:2:N:R
        13,  // E6: LDA   d8,X     2 3 0:2:N:R
        14,  // E7: STA   d8,X     2 3 0:2:N:W
        13,  // E8: EOR   d8,X     2 3 0:2:N:R
        13,  // E9: ADC   d8,X     2 3 0:2:N:R
        13,  // EA: ORA   d8,X     2 3 0:2:N:R
        13,  // EB: ADD   d8,X     2 3 0:2:N:R
        3,   // EC: JMP   d8,X     2 3 0:2:d:i
        26,  // ED: JSR   d8,X     2 5 0:2:w:W:d:i
        13,  // EE: LDX   d8,X     2 3 0:2:N:R
        14,  // EF: STX   d8,X     2 3 0:2:N:W
        16,  // F0: SUB   ,X       1 2 0:N:R
        16,  // F1: CMP   ,X       1 2 0:N:R
        16,  // F2: SBC   ,X       1 2 0:N:R
        16,  // F3: CPX   ,X       1 2 0:N:R
        16,  // F4: AND   ,X       1 2 0:N:R
        16,  // F5: BIT   ,X       1 2 0:N:R
        16,  // F6: LDA   ,X       1 2 0:N:R
        17,  // F7: STA   ,X       1 2 0:N:W
        16,  // F8: EOR   ,X       1 2 0:N:R
        16,  // F9: ADC   ,X       1 2 0:N:R
        16,  // FA: ORA   ,X       1 2 0:N:R
        16,  // FB: ADD   ,X       1 2 0:N:R
        21,  // FC: JMP   ,X       1 2 0:d:i
        4,   // FD: JSR   ,X       1 4 0:d:w:W:i
        16,  // FE: LDX   ,X       1 2 0:N:R
        17,  // FF: STX   ,X       1 2 0:N:W
};

constexpr uint8_t P9E_TABLE[] = {
        0,   // 00: -     -        - - -
        0,   // 01: -     -        - - -
        0,   // 02: -     -        - - -
        0,   // 03: -     -        - - -
        0,   // 04: -     -        - - -
        0,   // 05: -     -        - - -
        0,   // 06: -     -        - - -
        0,   // 07: -     -        - - -
        0,   // 08: -     -        - - -
        0,   // 09: -     -        - - -
        0,   // 0A: -     -        - - -
        0,   // 0B: -     -        - - -
        0,   // 0C: -     -        - - -
        0,   // 0D: -     -        - - -
        0,   // 0E: -     -        - - -
        0,   // 0F: -     -        - - -
        0,   // 10: -     -        - - -
        0,   // 11: -     -        - - -
        0,   // 12: -     -        - - -
        0,   // 13: -     -        - - -
        0,   // 14: -     -        - - -
        0,   // 15: -     -        - - -
        0,   // 16: -     -        - - -
        0,   // 17: -     -        - - -
        0,   // 18: -     -        - - -
        0,   // 19: -     -        - - -
        0,   // 1A: -     -        - - -
        0,   // 1B: -     -        - - -
        0,   // 1C: -     -        - - -
        0,   // 1D: -     -        - - -
        0,   // 1E: -     -        - - -
        0,   // 1F: -     -        - - -
        0,   // 20: -     -        - - -
        0,   // 21: -     -        - - -
        0,   // 22: -     -        - - -
        0,   // 23: -     -        - - -
        0,   // 24: -     -        - - -
        0,   // 25: -     -        - - -
        0,   // 26: -     -        - - -
        0,   // 27: -     -        - - -
        0,   // 28: -     -        - - -
        0,   // 29: -     -        - - -
        0,   // 2A: -     -        - - -
        0,   // 2B: -     -        - - -
        0,   // 2C: -     -        - - -
        0,   // 2D: -     -        - - -
        0,   // 2E: -     -        - - -
        0,   // 2F: -     -        - - -
        0,   // 30: -     -        - - -
        0,   // 31: -     -        - - -
        0,   // 32: -     -        - - -
        0,   // 33: -     -        - - -
        0,   // 34: -     -        - - -
        0,   // 35: -     -        - - -
        0,   // 36: -     -        - - -
        0,   // 37: -     -        - - -
        0,   // 38: -     -        - - -
        0,   // 39: -     -        - - -
        0,   // 3A: -     -        - - -
        0,   // 3B: -     -        - - -
        0,   // 3C: -     -        - - -
        0,   // 3D: -     -        - - -
        0,   // 3E: -     -        - - -
        0,   // 3F: -     -        - - -
        0,   // 40: -     -        - - -
        0,   // 41: -     -        - - -
        0,   // 42: -     -        - - -
        0,   // 43: -     -        - - -
        0,   // 44: -     -        - - -
        0,   // 45: -     -        - - -
        0,   // 46: -     -        - - -
        0,   // 47: -     -        - - -
        0,   // 48: -     -        - - -
        0,   // 49: -     -        - - -
        0,   // 4A: -     -        - - -
        0,   // 4B: -     -        - - -
        0,   // 4C: -     -        - - -
        0,   // 4D: -     -        - - -
        0,   // 4E: -     -        - - -
        0,   // 4F: -     -        - - -
        0,   // 50: -     -        - - -
        0,   // 51: -     -        - - -
        0,   // 52: -     -        - - -
        0,   // 53: -     -        - - -
        0,   // 54: -     -        - - -
        0,   // 55: -     -        - - -
        0,   // 56: -     -        - - -
        0,   // 57: -     -        - - -
        0,   // 58: -     -        - - -
        0,   // 59: -     -        - - -
        0,   // 5A: -     -        - - -
        0,   // 5B: -     -        - - -
        0,   // 5C: -     -        - - -
        0,   // 5D: -     -        - - -
        0,   // 5E: -     -        - - -
        0,   // 5F: -     -        - - -
        27,  // 60: NEG   d8,SP    3 5 0:2:3:N:R:W
        28,  // 61: CBEQ  d8,SP,r8 4 6 0:2:3:4:R:d:j
        0,   // 62: -     -        - - -
        27,  // 63: COM   d8,SP    3 5 0:2:3:N:R:W
        27,  // 64: LSR   d8,SP    4 5 0:2:3:N:R:W
        0,   // 65: -     -        - - -
        27,  // 66: ROR   d8,SP    3 5 0:2:3:N:R:W
        27,  // 67: ASR   d8,SP    3 5 0:2:3:N:R:W
        27,  // 68: ASL   d8,SP    3 5 0:2:3:N:R:W
        27,  // 69: ROL   d8,SP    3 5 0:2:3:N:R:W
        27,  // 6A: DEC   d8,SP    3 5 0:2:3:N:R:W
        29,  // 6B: DBNZ  d8,SP,r8 4 6 0:2:3:4:R:W:j
        27,  // 6C: INC   d8,SP    3 5 0:2:3:N:R:W
        23,  // 6D: TST   d8,SP    3 4 0:2:3:N:R
        0,   // 6E: -     -        - - -
        24,  // 6F: CLR   d8,SP    3 4 0:2:3:N:W
        0,   // 70: -     -        - - -
        0,   // 71: -     -        - - -
        0,   // 72: -     -        - - -
        0,   // 73: -     -        - - -
        0,   // 74: -     -        - - -
        0,   // 75: -     -        - - -
        0,   // 76: -     -        - - -
        0,   // 77: -     -        - - -
        0,   // 78: -     -        - - -
        0,   // 79: -     -        - - -
        0,   // 7A: -     -        - - -
        0,   // 7B: -     -        - - -
        0,   // 7C: -     -        - - -
        0,   // 7D: -     -        - - -
        0,   // 7E: -     -        - - -
        0,   // 7F: -     -        - - -
        0,   // 80: -     -        - - -
        0,   // 81: -     -        - - -
        0,   // 82: -     -        - - -
        0,   // 83: -     -        - - -
        0,   // 84: -     -        - - -
        0,   // 85: -     -        - - -
        0,   // 86: -     -        - - -
        0,   // 87: -     -        - - -
        0,   // 88: -     -        - - -
        0,   // 89: -     -        - - -
        0,   // 8A: -     -        - - -
        0,   // 8B: -     -        - - -
        0,   // 8C: -     -        - - -
        0,   // 8D: -     -        - - -
        0,   // 8E: -     -        - - -
        0,   // 8F: -     -        - - -
        0,   // 90: -     -        - - -
        0,   // 91: -     -        - - -
        0,   // 92: -     -        - - -
        0,   // 93: -     -        - - -
        0,   // 94: -     -        - - -
        0,   // 95: -     -        - - -
        0,   // 96: -     -        - - -
        0,   // 97: -     -        - - -
        0,   // 98: -     -        - - -
        0,   // 99: -     -        - - -
        0,   // 9A: -     -        - - -
        0,   // 9B: -     -        - - -
        0,   // 9C: -     -        - - -
        0,   // 9D: -     -        - - -
        0,   // 9E: -     -        - - -
        0,   // 9F: -     -        - - -
        0,   // A0: -     -        - - -
        0,   // A1: -     -        - - -
        0,   // A2: -     -        - - -
        0,   // A3: -     -        - - -
        0,   // A4: -     -        - - -
        0,   // A5: -     -        - - -
        0,   // A6: -     -        - - -
        0,   // A7: -     -        - - -
        0,   // A8: -     -        - - -
        0,   // A9: -     -        - - -
        0,   // AA: -     -        - - -
        0,   // AB: -     -        - - -
        0,   // AC: -     -        - - -
        0,   // AD: -     -        - - -
        0,   // AE: -     -        - - -
        0,   // AF: -     -        - - -
        0,   // B0: -     -        - - -
        0,   // B1: -     -        - - -
        0,   // B2: -     -        - - -
        0,   // B3: -     -        - - -
        0,   // B4: -     -        - - -
        0,   // B5: -     -        - - -
        0,   // B6: -     -        - - -
        0,   // B7: -     -        - - -
        0,   // B8: -     -        - - -
        0,   // B9: -     -        - - -
        0,   // BA: -     -        - - -
        0,   // BB: -     -        - - -
        0,   // BC: -     -        - - -
        0,   // BD: -     -        - - -
        0,   // BE: -     -        - - -
        0,   // BF: -     -        - - -
        0,   // C0: -     -        - - -
        0,   // C1: -     -        - - -
        0,   // C2: -     -        - - -
        0,   // C3: -     -        - - -
        0,   // C4: -     -        - - -
        0,   // C5: -     -        - - -
        0,   // C6: -     -        - - -
        0,   // C7: -     -        - - -
        0,   // C8: -     -        - - -
        0,   // C9: -     -        - - -
        0,   // CA: -     -        - - -
        0,   // CB: -     -        - - -
        0,   // CC: -     -        - - -
        0,   // CD: -     -        - - -
        0,   // CE: -     -        - - -
        0,   // CF: -     -        - - -
        30,  // D0: SUB   d16,SP   4 5 0:2:3:4:N:R
        30,  // D1: CMP   d16,SP   4 5 0:2:3:4:N:R
        30,  // D2: SBC   d16,SP   4 5 0:2:3:4:N:R
        30,  // D3: CPX   d16,SP   4 5 0:2:3:4:N:R
        30,  // D4: AND   d16,SP   4 5 0:2:3:4:N:R
        30,  // D5: BIT   d16,SP   4 5 0:2:3:4:N:R
        30,  // D6: LDA   d16,SP   4 5 0:2:3:4:N:R
        31,  // D7: STA   d16,SP   4 5 0:2:3:4:N:W
        30,  // D8: EOR   d16,SP   4 5 0:2:3:4:N:R
        30,  // D9: ADC   d16,SP   4 5 0:2:3:4:N:R
        30,  // DA: ORA   d16,SP   4 5 0:2:3:4:N:R
        30,  // DB: ADD   d16,SP   4 5 0:2:3:4:N:R
        0,   // DC: -     -        - - -
        0,   // DD: -     -        - - -
        30,  // DE: LDX   d16,SP   4 5 0:2:3:4:N:R
        31,  // DF: STX   d16,SP   4 5 0:2:3:4:N:W
        23,  // E0: SUB   d8,SP    3 4 0:2:3:N:R
        23,  // E1: CMP   d8,SP    3 4 0:2:3:N:R
        23,  // E2: SBC   d8,SP    3 4 0:2:3:N:R
        23,  // E3: CPX   d8,SP    3 4 0:2:3:N:R
        23,  // E4: AND   d8,SP    3 4 0:2:3:N:R
        23,  // E5: BIT   d8,SP    3 4 0:2:3:N:R
        23,  // E6: LDA   d8,SP    3 4 0:2:3:N:R
        24,  // E7: STA   d8,SP    3 4 0:2:3:N:W
        23,  // E8: EOR   d8,SP    3 4 0:2:3:N:R
        23,  // E9: ADC   d8,SP    3 4 0:2:3:N:R
        23,  // EA: ORA   d8,SP    3 4 0:2:3:N:R
        23,  // EB: ADD   d8,SP    3 4 0:2:3:N:R
        0,   // EC: -     -        - - -
        0,   // ED: -     -        - - -
        23,  // EE: LDX   d8,SP    3 4 0:2:3:N:R
        24,  // EF: STX   d8,SP    3 4 0:2:3:N:W
        0,   // F0: -     -        - - -
        0,   // F1: -     -        - - -
        0,   // F2: -     -        - - -
        0,   // F3: -     -        - - -
        0,   // F4: -     -        - - -
        0,   // F5: -     -        - - -
        0,   // F6: -     -        - - -
        0,   // F7: -     -        - - -
        0,   // F8: -     -        - - -
        0,   // F9: -     -        - - -
        0,   // FA: -     -        - - -
        0,   // FB: -     -        - - -
        0,   // FC: -     -        - - -
        0,   // FD: -     -        - - -
        0,   // FE: -     -        - - -
        0,   // FF: -     -        - - -
};

const char *InstMc68HC08::sequence(Signals *s) const {
    auto opc = _mems->read(s->addr);
    // avoid unnecessary duplicated read
    s->inject(opc);
    if (opc == 0x9E) {
        opc = _mems->read(s->addr + 1);
        return SEQUENCES[P9E_TABLE[opc]];
    }
    return SEQUENCES[P00_TABLE[opc]];
}

}  // namespace mc68hc08
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
