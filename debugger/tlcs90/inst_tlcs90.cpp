#include "inst_tlcs90.h"
#include <string.h>
#include "debugger.h"
#include "mems_tlcs90.h"
#include "regs_tlcs90.h"

namespace debugger {
namespace tlcs90 {

namespace {

/**
 * Legend for SEQUENCES
# 0: prefetched instruction
# 2-4: part of instruction or prefix
# 7~9: instruction following prefix
# N: prefetch next instruction
# n: dummy prefetch next instruction
# m: dummy prefetch next instruction for jump true
# i: new instruction address
# j: 8-bit relative branch (next+2)
# k: 16-bit relative branch (next+3:2)
# J: 16-bit absolute jump address (3:2 or r:R)
# V: next instruction fetch from vector (0000_0000_0xxx_x000B)
# d: dummy cycle, no read nor write
# R: read 1 byte
# W: write 1 byte, the same address if R or E is preceeded
# r: read 1 byte at address R+1 or r+1
# w: write 1 byte at address W+1
# E: read 1 byte from direct page (0FFxxH)
# F: write 1 byte to direct page (0FFxxH)
#
# Interrupt sequence
# 0:n:d:d:V:d:w:W:W:W:V
 */

constexpr const char INTERRUPT[] = "0nVwWWWV";

constexpr const char *const SEQUENCES[/*seq*/] = {
        "",             //  0
        "0N",           //  1
        "02ENW@02N",    //  2
        "02N",          //  3
        "023N",         //  4
        "02j@02N",      //  5
        "023J",         //  6
        "023k",         //  7
        "023wWJ",       //  8
        "023wWk",       //  9
        "0nRrJ",        // 10
        "0nRrrrJ",      // 11
        "02EN",         // 12
        "02NF",         // 13
        "023FN",        // 14
        "0234NFw",      // 15
        "02ErN",        // 16
        "02NFw",        // 17
        "0NwW",         // 18
        "0nRrN",        // 19
        "02ENW",        // 20
        "02ErNFw",      // 21
        "02nj@02N",     // 22
        "02nj",         // 23
        "0",            // 24
        "023",          // 25
        "02",           // 26
        "0nVwWWWV",     // 27
        "7nN",          // 28
        "7N",           // 29
        "7nRWN",        // 30
        "7nRWi@7nRWN",  // 31
        "7nRN",         // 32
        "7nRi@7nRN",    // 33
        "78N",          // 34
        "7nRrJ@7N",     // 35
        "7nRrJ",        // 36
        "7RNW",         // 37
        "7RN",          // 38
        "7RrN",         // 39
        "7RrWwN",       // 40
        "7RrNWw",       // 41
        "7NW",          // 42
        "78WN",         // 43
        "789NWw",       // 44
        "7NWw",         // 45
        "78RNW",        // 46
        "78RN",         // 47
        "7mi@7N",       // 48
        "7ni",          // 49
        "7nwWi@7N",     // 50
        "7nwWi",        // 51
};

constexpr uint8_t PAGE0[] = {
        1,   // 00: NOP  -      ; 0N
        1,   // 01: HALT -      ; 0N
        1,   // 02: DI   -      ; 0N
        1,   // 03: EI   -      ; 0N
        0,   // 04: -    -      ; -
        0,   // 05: -    -      ; -
        0,   // 06: -    -      ; -
        2,   // 07: INCX (n)    ; 02ENW@02N
        1,   // 08: EX   DE,HL  ; 0N
        1,   // 09: EX   AF,AF' ; 0N
        1,   // 0A: EXX  -      ; 0N
        1,   // 0B: DAA  A      ; 0N
        1,   // 0C: RCF  -      ; 0N
        1,   // 0D: SCF  -      ; 0N
        1,   // 0E: CCF  -      ; 0N
        2,   // 0F: DECX (n)    ; 02ENW@02N
        1,   // 10: CPL  A      ; 0N
        1,   // 11: NEG  A      ; 0N
        3,   // 12: MUL  HL,n   ; 02N
        3,   // 13: DIV  HL,n   ; 02N
        4,   // 14: ADD  IX,mn  ; 023N
        4,   // 15: ADD  IY,mn  ; 023N
        4,   // 16: ADD  SP,mn  ; 023N
        4,   // 17: LDAR HL,cd  ; 023N
        5,   // 18: DJNZ d      ; 02j@02N
        5,   // 19: DJNZ BC,d   ; 02j@02N
        6,   // 1A: JP   mn     ; 023J
        7,   // 1B: JRL  cd     ; 023k
        8,   // 1C: CALL mn     ; 023wWJ
        9,   // 1D: CALR cd     ; 023wWk
        10,  // 1E: RET  -      ; 0nRrJ
        11,  // 1F: RETI -      ; 0nRrrrJ
        1,   // 20: LD   A,B    ; 0N
        1,   // 21: LD   A,C    ; 0N
        1,   // 22: LD   A,D    ; 0N
        1,   // 23: LD   A,E    ; 0N
        1,   // 24: LD   A,H    ; 0N
        1,   // 25: LD   A,L    ; 0N
        1,   // 26: LD   A,A    ; 0N
        12,  // 27: LD   A,(n)  ; 02EN
        1,   // 28: LD   B,A    ; 0N
        1,   // 29: LD   C,A    ; 0N
        1,   // 2A: LD   D,A    ; 0N
        1,   // 2B: LD   E,A    ; 0N
        1,   // 2C: LD   H,A    ; 0N
        1,   // 2D: LD   L,A    ; 0N
        1,   // 2E: LD   A,A    ; 0N
        13,  // 2F: LD   (n),A  ; 02NF
        3,   // 30: LD   B,n    ; 02N
        3,   // 31: LD   C,n    ; 02N
        3,   // 32: LD   D,n    ; 02N
        3,   // 33: LD   E,n    ; 02N
        3,   // 34: LD   H,n    ; 02N
        3,   // 35: LD   L,n    ; 02N
        3,   // 36: LD   A,n    ; 02N
        14,  // 37: LD   (w),n  ; 023FN
        4,   // 38: LD   BC,mn  ; 023N
        4,   // 39: LD   DE,mn  ; 023N
        4,   // 3A: LD   HL,mn  ; 023N
        0,   // 3B: -    -      ; -
        4,   // 3C: LD   IX,mn  ; 023N
        4,   // 3D: LD   IY,mn  ; 023N
        4,   // 3E: LD   SP,mn  ; 023N
        15,  // 3F: LDW  (w),mn ; 0234NFw
        1,   // 40: LD   HL,BC  ; 0N
        1,   // 41: LD   HL,DE  ; 0N
        1,   // 42: LD   HL,HL  ; 0N
        0,   // 43: -    -      ; -
        1,   // 44: LD   HL,IX  ; 0N
        1,   // 45: LD   HL,IY  ; 0N
        1,   // 46: LD   HL,SP  ; 0N
        16,  // 47: LD   HL,(n) ; 02ErN
        1,   // 48: LD   BC,HL  ; 0N
        1,   // 49: LD   DE,HL  ; 0N
        1,   // 4A: LD   HL,HL  ; 0N
        0,   // 4B: -    -      ; -
        1,   // 4C: LD   IX,HL  ; 0N
        1,   // 4D: LD   IY,HL  ; 0N
        1,   // 4E: LD   SP,HL  ; 0N
        17,  // 4F: LD   (n),HL ; 02NFw
        18,  // 50: PUSH BC     ; 0NwW
        18,  // 51: PUSH DE     ; 0NwW
        18,  // 52: PUSH HL     ; 0NwW
        0,   // 53: -    -      ; -
        18,  // 54: PUSH IX     ; 0NwW
        18,  // 55: PUSH IY     ; 0NwW
        18,  // 56: PUSH AF     ; 0NwW
        0,   // 57: -    -      ; -
        19,  // 58: POP  BC     ; 0nRrN
        19,  // 59: POP  DE     ; 0nRrN
        19,  // 5A: POP  HL     ; 0nRrN
        0,   // 5B: -    -      ; -
        19,  // 5C: POP  IX     ; 0nRrN
        19,  // 5D: POP  IY     ; 0nRrN
        19,  // 5E: POP  AF     ; 0nRrN
        0,   // 5F: -    -      ; -
        12,  // 60: ADD  A,(n)  ; 02EN
        12,  // 61: ADC  A,(n)  ; 02EN
        12,  // 62: SUB  A,(n)  ; 02EN
        12,  // 63: SBC  A,(n)  ; 02EN
        12,  // 64: AND  A,(n)  ; 02EN
        12,  // 65: XOR  A,(n)  ; 02EN
        12,  // 66: OR   A,(n)  ; 02EN
        12,  // 67: CP   A,(n)  ; 02EN
        3,   // 68: ADD  A,n    ; 02N
        3,   // 69: ADC  A,n    ; 02N
        3,   // 6A: SUB  A,n    ; 02N
        3,   // 6B: SBC  A,n    ; 02N
        3,   // 6C: AND  A,n    ; 02N
        3,   // 6D: XOR  A,n    ; 02N
        3,   // 6E: OR   A,n    ; 02N
        3,   // 6F: CP   A,n    ; 02N
        16,  // 70: ADD  HL,(n) ; 02ErN
        16,  // 71: ADC  HL,(n) ; 02ErN
        16,  // 72: SUB  HL,(n) ; 02ErN
        16,  // 73: SBC  HL,(n) ; 02ErN
        16,  // 74: AND  HL,(n) ; 02ErN
        16,  // 75: XOR  HL,(n) ; 02ErN
        16,  // 76: OR   HL,(n) ; 02ErN
        16,  // 77: CP   HL,(n) ; 02ErN
        4,   // 78: ADD  HL,mn  ; 023N
        4,   // 79: ADC  HL,mn  ; 023N
        4,   // 7A: SUB  HL,mn  ; 023N
        4,   // 7B: SBC  HL,mn  ; 023N
        4,   // 7C: AND  HL,mn  ; 023N
        4,   // 7D: XOR  HL,mn  ; 023N
        4,   // 7E: OR   HL,mn  ; 023N
        4,   // 7F: CP   HL,mn  ; 023N
        1,   // 80: INC  B      ; 0N
        1,   // 81: INC  C      ; 0N
        1,   // 82: INC  D      ; 0N
        1,   // 83: INC  E      ; 0N
        1,   // 84: INC  H      ; 0N
        1,   // 85: INC  L      ; 0N
        1,   // 86: INC  A      ; 0N
        20,  // 87: INC  (n)    ; 02ENW
        1,   // 88: DEC  B      ; 0N
        1,   // 89: DEC  C      ; 0N
        1,   // 8A: DEC  D      ; 0N
        1,   // 8B: DEC  E      ; 0N
        1,   // 8C: DEC  H      ; 0N
        1,   // 8D: DEC  L      ; 0N
        1,   // 8E: DEC  A      ; 0N
        20,  // 8F: DEC  (n)    ; 02ENW
        1,   // 90: INC  BC     ; 0N
        1,   // 91: INC  DE     ; 0N
        1,   // 92: INC  HL     ; 0N
        0,   // 93: -    -      ; -
        1,   // 94: INC  IX     ; 0N
        1,   // 95: INC  IY     ; 0N
        1,   // 96: INC  SP     ; 0N
        21,  // 97: INCW (n)    ; 02ErNFw
        1,   // 98: DEC  BC     ; 0N
        1,   // 99: DEC  DE     ; 0N
        1,   // 9A: DEC  HL     ; 0N
        0,   // 9B: -    -      ; -
        1,   // 9C: DEC  IX     ; 0N
        1,   // 9D: DEC  IY     ; 0N
        1,   // 9E: DEC  SP     ; 0N
        21,  // 9F: DECW (n)    ; 02ErNFw
        1,   // A0: RLCA -      ; 0N
        1,   // A1: RRCA -      ; 0N
        1,   // A2: RLA  -      ; 0N
        1,   // A3: RRA  -      ; 0N
        1,   // A4: SLAA -      ; 0N
        1,   // A5: SRAA -      ; 0N
        1,   // A6: SLLA -      ; 0N
        1,   // A7: SRLA -      ; 0N
        12,  // A8: BIT  0,(n)  ; 02EN
        12,  // A9: BIT  1,(n)  ; 02EN
        12,  // AA: BIT  2,(n)  ; 02EN
        12,  // AB: BIT  3,(n)  ; 02EN
        12,  // AC: BIT  4,(n)  ; 02EN
        12,  // AD: BIT  5,(n)  ; 02EN
        12,  // AE: BIT  6,(n)  ; 02EN
        12,  // AF: BIT  7,(n)  ; 02EN
        20,  // B0: RES  0,(n)  ; 02ENW
        20,  // B1: RES  1,(n)  ; 02ENW
        20,  // B2: RES  2,(n)  ; 02ENW
        20,  // B3: RES  3,(n)  ; 02ENW
        20,  // B4: RES  4,(n)  ; 02ENW
        20,  // B5: RES  5,(n)  ; 02ENW
        20,  // B6: RES  6,(n)  ; 02ENW
        20,  // B7: RES  7,(n)  ; 02ENW
        20,  // B8: SET  0,(n)  ; 02ENW
        20,  // B9: SET  1,(n)  ; 02ENW
        20,  // BA: SET  2,(n)  ; 02ENW
        20,  // BB: SET  3,(n)  ; 02ENW
        20,  // BC: SET  4,(n)  ; 02ENW
        20,  // BD: SET  5,(n)  ; 02ENW
        20,  // BE: SET  6,(n)  ; 02ENW
        20,  // BF: SET  7,(n)  ; 02ENW
        22,  // C0: JR   F,d    ; 02nj@02N
        22,  // C1: JR   LT,d   ; 02nj@02N
        22,  // C2: JR   LE,d   ; 02nj@02N
        22,  // C3: JR   ULE,d  ; 02nj@02N
        22,  // C4: JR   OV,d   ; 02nj@02N
        22,  // C5: JR   MI,d   ; 02nj@02N
        22,  // C6: JR   Z,d    ; 02nj@02N
        22,  // C7: JR   C,d    ; 02nj@02N
        23,  // C8: JR   d      ; 02nj
        22,  // C9: JR   GE,d   ; 02nj@02N
        22,  // CA: JR   GT,d   ; 02nj@02N
        22,  // CB: JR   UGT,d  ; 02nj@02N
        22,  // CC: JR   UGE,d  ; 02nj@02N
        22,  // CD: JR   NOV,d  ; 02nj@02N
        22,  // CE: JR   NZ,d   ; 02nj@02N
        22,  // CF: JR   NC,d   ; 02nj@02N
        0,   // D0: -    -      ; -
        0,   // D1: -    -      ; -
        0,   // D2: -    -      ; -
        0,   // D3: -    -      ; -
        0,   // D4: -    -      ; -
        0,   // D5: -    -      ; -
        0,   // D6: -    -      ; -
        0,   // D7: -    -      ; -
        0,   // D8: -    -      ; -
        0,   // D9: -    -      ; -
        0,   // DA: -    -      ; -
        0,   // DB: -    -      ; -
        0,   // DC: -    -      ; -
        0,   // DD: -    -      ; -
        0,   // DE: -    -      ; -
        0,   // DF: -    -      ; -
        24,  // E0: -    (BC)   ; 0
        24,  // E1: -    (DE)   ; 0
        24,  // E2: -    (HL)   ; 0
        25,  // E3: -    (mn)   ; 023
        24,  // E4: -    (IX)   ; 0
        24,  // E5: -    (IY)   ; 0
        24,  // E6: -    (SP)   ; 0
        26,  // E7: -    (d)    ; 02
        24,  // E8: -    (BC)   ; 0
        24,  // E9: -    (DE)   ; 0
        24,  // EA: -    (HL)   ; 0
        25,  // EB: -    (mn)   ; 023
        24,  // EC: -    (IX)   ; 0
        24,  // ED: -    (IY)   ; 0
        24,  // EE: -    (SP)   ; 0
        26,  // EF: -    (d)    ; 02
        26,  // F0: -    (IX+d) ; 02
        26,  // F1: -    (IY+d) ; 02
        26,  // F2: -    (SP+d) ; 02
        24,  // F3: -    (HL+A) ; 0
        26,  // F4: -    (IX+d) ; 02
        26,  // F5: -    (IY+d) ; 02
        26,  // F6: -    (SP+d) ; 02
        24,  // F7: -    (HL+A) ; 0
        24,  // F8: -    B/BC   ; 0
        24,  // F9: -    C/DE   ; 0
        24,  // FA: -    D/HL   ; 0
        24,  // FB: -    E      ; 0
        24,  // FC: -    H/IX   ; 0
        24,  // FD: -    L/IY   ; 0
        24,  // FE: -    A/SP   ; 0
        27,  // FF: SWI  -      ; 0nVwWWWV
};

constexpr uint8_t PAGE1[] = {
        0,   // 00: -    -      ; -
        0,   // 01: -    -      ; -
        0,   // 02: -    -      ; -
        0,   // 03: -    -      ; -
        0,   // 04: -    -      ; -
        0,   // 05: -    -      ; -
        0,   // 06: -    -      ; -
        0,   // 07: -    -      ; -
        0,   // 08: -    -      ; -
        0,   // 09: -    -      ; -
        0,   // 0A: -    -      ; -
        0,   // 0B: -    -      ; -
        0,   // 0C: -    -      ; -
        0,   // 0D: -    -      ; -
        0,   // 0E: -    -      ; -
        0,   // 0F: -    -      ; -
        0,   // 10: -    -      ; -
        0,   // 11: -    -      ; -
        28,  // 12: MUL  HL,g   ; 7nN
        28,  // 13: DIV  HL,g   ; 7nN
        29,  // 14: ADD  IX,gg  ; 7N
        29,  // 15: ADD  IY,gg  ; 7N
        29,  // 16: ADD  SP,gg  ; 7N
        0,   // 17: -    -      ; -
        29,  // 18: TSET 0,g    ; 7N
        29,  // 19: TSET 1,g    ; 7N
        29,  // 1A: TSET 2,g    ; 7N
        29,  // 1B: TSET 3,g    ; 7N
        29,  // 1C: TSET 4,g    ; 7N
        29,  // 1D: TSET 5,g    ; 7N
        29,  // 1E: TSET 6,g    ; 7N
        29,  // 1F: TSET 7,g    ; 7N
        0,   // 20: -    -      ; -
        0,   // 21: -    -      ; -
        0,   // 22: -    -      ; -
        0,   // 23: -    -      ; -
        0,   // 24: -    -      ; -
        0,   // 25: -    -      ; -
        0,   // 26: -    -      ; -
        0,   // 27: -    -      ; -
        0,   // 28: -    -      ; -
        0,   // 29: -    -      ; -
        0,   // 2A: -    -      ; -
        0,   // 2B: -    -      ; -
        0,   // 2C: -    -      ; -
        0,   // 2D: -    -      ; -
        0,   // 2E: -    -      ; -
        0,   // 2F: -    -      ; -
        29,  // 30: LD   B,g    ; 7N
        29,  // 31: LD   C,g    ; 7N
        29,  // 32: LD   D,g    ; 7N
        29,  // 33: LD   E,g    ; 7N
        29,  // 34: LD   H,g    ; 7N
        29,  // 35: LD   L,g    ; 7N
        29,  // 36: LD   A,g    ; 7N
        0,   // 37: -    -      ; -
        29,  // 38: LD   BC,gg  ; 7N
        29,  // 39: LD   DE,gg  ; 7N
        29,  // 3A: LD   HL,gg  ; 7N
        0,   // 3B: -    -      ; -
        29,  // 3C: LD   IX,gg  ; 7N
        29,  // 3D: LD   IY,gg  ; 7N
        29,  // 3E: LD   SP,gg  ; 7N
        0,   // 3F: -    -      ; -
        0,   // 40: -    -      ; -
        0,   // 41: -    -      ; -
        0,   // 42: -    -      ; -
        0,   // 43: -    -      ; -
        0,   // 44: -    -      ; -
        0,   // 45: -    -      ; -
        0,   // 46: -    -      ; -
        0,   // 47: -    -      ; -
        0,   // 48: -    -      ; -
        0,   // 49: -    -      ; -
        0,   // 4A: -    -      ; -
        0,   // 4B: -    -      ; -
        0,   // 4C: -    -      ; -
        0,   // 4D: -    -      ; -
        0,   // 4E: -    -      ; -
        0,   // 4F: -    -      ; -
        0,   // 50: -    -      ; -
        0,   // 51: -    -      ; -
        0,   // 52: -    -      ; -
        0,   // 53: -    -      ; -
        0,   // 54: -    -      ; -
        0,   // 55: -    -      ; -
        0,   // 56: -    -      ; -
        0,   // 57: -    -      ; -
        30,  // 58: LDI  -      ; 7nRWN
        31,  // 59: LDIR -      ; 7nRWi@7nRWN
        30,  // 5A: LDD  -      ; 7nRWN
        31,  // 5B: LDDR -      ; 7nRWi@7nRWN
        32,  // 5C: CPI  -      ; 7nRN
        33,  // 5D: CPIR -      ; 7nRi@7nRN
        32,  // 5E: CPD  -      ; 7nRN
        33,  // 5F: CPDR -      ; 7nRi@7nRN
        29,  // 60: ADD  A,g    ; 7N
        29,  // 61: ADC  A,g    ; 7N
        29,  // 62: SUB  A,g    ; 7N
        29,  // 63: SBC  A,g    ; 7N
        29,  // 64: AND  A,g    ; 7N
        29,  // 65: XOR  A,g    ; 7N
        29,  // 66: OR   A,g    ; 7N
        29,  // 67: CP   A,g    ; 7N
        34,  // 68: ADD  g,n    ; 78N
        34,  // 69: ADC  g,n    ; 78N
        34,  // 6A: SUB  g,n    ; 78N
        34,  // 6B: SBC  g,n    ; 78N
        34,  // 6C: AND  g,n    ; 78N
        34,  // 6D: XOR  g,n    ; 78N
        34,  // 6E: OR   g,n    ; 78N
        34,  // 6F: CP   g,n    ; 78N
        29,  // 70: ADD  HL,gg  ; 7N
        29,  // 71: ADC  HL,gg  ; 7N
        29,  // 72: SUB  HL,gg  ; 7N
        29,  // 73: SBC  HL,gg  ; 7N
        29,  // 74: AND  HL,gg  ; 7N
        29,  // 75: XOR  HL,gg  ; 7N
        29,  // 76: OR   HL,gg  ; 7N
        29,  // 77: CP   HL,gg  ; 7N
        0,   // 78: -    -      ; -
        0,   // 79: -    -      ; -
        0,   // 7A: -    -      ; -
        0,   // 7B: -    -      ; -
        0,   // 7C: -    -      ; -
        0,   // 7D: -    -      ; -
        0,   // 7E: -    -      ; -
        0,   // 7F: -    -      ; -
        0,   // 80: -    -      ; -
        0,   // 81: -    -      ; -
        0,   // 82: -    -      ; -
        0,   // 83: -    -      ; -
        0,   // 84: -    -      ; -
        0,   // 85: -    -      ; -
        0,   // 86: -    -      ; -
        0,   // 87: -    -      ; -
        0,   // 88: -    -      ; -
        0,   // 89: -    -      ; -
        0,   // 8A: -    -      ; -
        0,   // 8B: -    -      ; -
        0,   // 8C: -    -      ; -
        0,   // 8D: -    -      ; -
        0,   // 8E: -    -      ; -
        0,   // 8F: -    -      ; -
        0,   // 90: -    -      ; -
        0,   // 91: -    -      ; -
        0,   // 92: -    -      ; -
        0,   // 93: -    -      ; -
        0,   // 94: -    -      ; -
        0,   // 95: -    -      ; -
        0,   // 96: -    -      ; -
        0,   // 97: -    -      ; -
        0,   // 98: -    -      ; -
        0,   // 99: -    -      ; -
        0,   // 9A: -    -      ; -
        0,   // 9B: -    -      ; -
        0,   // 9C: -    -      ; -
        0,   // 9D: -    -      ; -
        0,   // 9E: -    -      ; -
        0,   // 9F: -    -      ; -
        29,  // A0: RLC  g      ; 7N
        29,  // A1: RRC  g      ; 7N
        29,  // A2: RL   g      ; 7N
        29,  // A3: RR   g      ; 7N
        29,  // A4: SLA  g      ; 7N
        29,  // A5: SRA  g      ; 7N
        29,  // A6: SLL  g      ; 7N
        29,  // A7: SRL  g      ; 7N
        29,  // A8: BIT  0,g    ; 7N
        29,  // A9: BIT  1,g    ; 7N
        29,  // AA: BIT  2,g    ; 7N
        29,  // AB: BIT  3,g    ; 7N
        29,  // AC: BIT  4,g    ; 7N
        29,  // AD: BIT  5,g    ; 7N
        29,  // AE: BIT  6,g    ; 7N
        29,  // AF: BIT  7,g    ; 7N
        29,  // B0: RES  0,g    ; 7N
        29,  // B1: RES  1,g    ; 7N
        29,  // B2: RES  2,g    ; 7N
        29,  // B3: RES  3,g    ; 7N
        29,  // B4: RES  4,g    ; 7N
        29,  // B5: RES  5,g    ; 7N
        29,  // B6: RES  6,g    ; 7N
        29,  // B7: RES  7,g    ; 7N
        29,  // B8: SET  0,g    ; 7N
        29,  // B9: SET  1,g    ; 7N
        29,  // BA: SET  2,g    ; 7N
        29,  // BB: SET  3,g    ; 7N
        29,  // BC: SET  4,g    ; 7N
        29,  // BD: SET  5,g    ; 7N
        29,  // BE: SET  6,g    ; 7N
        29,  // BF: SET  7,g    ; 7N
        0,   // C0: -    -      ; -
        0,   // C1: -    -      ; -
        0,   // C2: -    -      ; -
        0,   // C3: -    -      ; -
        0,   // C4: -    -      ; -
        0,   // C5: -    -      ; -
        0,   // C6: -    -      ; -
        0,   // C7: -    -      ; -
        0,   // C8: -    -      ; -
        0,   // C9: -    -      ; -
        0,   // CA: -    -      ; -
        0,   // CB: -    -      ; -
        0,   // CC: -    -      ; -
        0,   // CD: -    -      ; -
        0,   // CE: -    -      ; -
        0,   // CF: -    -      ; -
        35,  // D0: RET  F      ; 7nRrJ@7N
        35,  // D1: RET  LT     ; 7nRrJ@7N
        35,  // D2: RET  LE     ; 7nRrJ@7N
        35,  // D3: RET  ULE    ; 7nRrJ@7N
        35,  // D4: RET  OV     ; 7nRrJ@7N
        35,  // D5: RET  MI     ; 7nRrJ@7N
        35,  // D6: RET  Z      ; 7nRrJ@7N
        35,  // D7: RET  C      ; 7nRrJ@7N
        36,  // D8: RET  -      ; 7nRrJ
        35,  // D9: RET  GE     ; 7nRrJ@7N
        35,  // DA: RET  GT     ; 7nRrJ@7N
        35,  // DB: RET  UGT    ; 7nRrJ@7N
        35,  // DC: RET  UGE    ; 7nRrJ@7N
        35,  // DD: RET  NOV    ; 7nRrJ@7N
        35,  // DE: RET  NZ     ; 7nRrJ@7N
        35,  // DF: RET  NC     ; 7nRrJ@7N
        0,   // E0: -    -      ; -
        0,   // E1: -    -      ; -
        0,   // E2: -    -      ; -
        0,   // E3: -    -      ; -
        0,   // E4: -    -      ; -
        0,   // E5: -    -      ; -
        0,   // E6: -    -      ; -
        0,   // E7: -    -      ; -
        0,   // E8: -    -      ; -
        0,   // E9: -    -      ; -
        0,   // EA: -    -      ; -
        0,   // EB: -    -      ; -
        0,   // EC: -    -      ; -
        0,   // ED: -    -      ; -
        0,   // EE: -    -      ; -
        0,   // EF: -    -      ; -
        0,   // F0: -    -      ; -
        0,   // F1: -    -      ; -
        0,   // F2: -    -      ; -
        0,   // F3: -    -      ; -
        0,   // F4: -    -      ; -
        0,   // F5: -    -      ; -
        0,   // F6: -    -      ; -
        0,   // F7: -    -      ; -
        0,   // F8: -    -      ; -
        0,   // F9: -    -      ; -
        0,   // FA: -    -      ; -
        0,   // FB: -    -      ; -
        0,   // FC: -    -      ; -
        0,   // FD: -    -      ; -
        0,   // FE: -    -      ; -
        0,   // FF: -    -      ; -
};

constexpr uint8_t PAGE2[] = {
        0,   // 00: -    -      ; -
        0,   // 01: -    -      ; -
        0,   // 02: -    -      ; -
        0,   // 03: -    -      ; -
        0,   // 04: -    -      ; -
        0,   // 05: -    -      ; -
        0,   // 06: -    -      ; -
        0,   // 07: -    -      ; -
        0,   // 08: -    -      ; -
        0,   // 09: -    -      ; -
        0,   // 0A: -    -      ; -
        0,   // 0B: -    -      ; -
        0,   // 0C: -    -      ; -
        0,   // 0D: -    -      ; -
        0,   // 0E: -    -      ; -
        0,   // 0F: -    -      ; -
        37,  // 10: RLD  (gg)   ; 7RNW
        37,  // 11: RRD  (gg)   ; 7RNW
        38,  // 12: MUL  HL,(gg); 7RN
        38,  // 13: DIV  HL,(gg); 7RN
        39,  // 14: ADD  IX,(gg); 7RrN
        39,  // 15: ADD  IY,(gg); 7RrN
        39,  // 16: ADD  SP,(gg); 7RrN
        0,   // 17: -    -      ; -
        37,  // 18: TSET 0,(gg) ; 7RNW
        37,  // 19: TSET 1,(gg) ; 7RNW
        37,  // 1A: TSET 2,(gg) ; 7RNW
        37,  // 1B: TSET 3,(gg) ; 7RNW
        37,  // 1C: TSET 4,(gg) ; 7RNW
        37,  // 1D: TSET 5,(gg) ; 7RNW
        37,  // 1E: TSET 6,(gg) ; 7RNW
        37,  // 1F: TSET 7,(gg) ; 7RNW
        0,   // 20: -    -      ; -
        0,   // 21: -    -      ; -
        0,   // 22: -    -      ; -
        0,   // 23: -    -      ; -
        0,   // 24: -    -      ; -
        0,   // 25: -    -      ; -
        0,   // 26: -    -      ; -
        0,   // 27: -    -      ; -
        38,  // 28: LD   B,(gg) ; 7RN
        38,  // 29: LD   C,(gg) ; 7RN
        38,  // 2A: LD   D,(gg) ; 7RN
        38,  // 2B: LD   E,(gg) ; 7RN
        38,  // 2C: LD   H,(gg) ; 7RN
        38,  // 2D: LD   L,(gg) ; 7RN
        38,  // 2E: LD   A,(gg) ; 7RN
        0,   // 2F: -    -      ; -
        0,   // 30: -    -      ; -
        0,   // 31: -    -      ; -
        0,   // 32: -    -      ; -
        0,   // 33: -    -      ; -
        0,   // 34: -    -      ; -
        0,   // 35: -    -      ; -
        0,   // 36: -    -      ; -
        0,   // 37: -    -      ; -
        0,   // 38: -    -      ; -
        0,   // 39: -    -      ; -
        0,   // 3A: -    -      ; -
        0,   // 3B: -    -      ; -
        0,   // 3C: -    -      ; -
        0,   // 3D: -    -      ; -
        0,   // 3E: -    -      ; -
        0,   // 3F: -    -      ; -
        0,   // 40: -    -      ; -
        0,   // 41: -    -      ; -
        0,   // 42: -    -      ; -
        0,   // 43: -    -      ; -
        0,   // 44: -    -      ; -
        0,   // 45: -    -      ; -
        0,   // 46: -    -      ; -
        0,   // 47: -    -      ; -
        39,  // 48: LD   BC,(gg); 7RrN
        39,  // 49: LD   DE,(gg); 7RrN
        39,  // 4A: LD   HL,(gg); 7RrN
        0,   // 4B: -    -      ; -
        39,  // 4C: LD   IX,(gg); 7RrN
        39,  // 4D: LD   IY,(gg); 7RrN
        39,  // 4E: LD   SP,(gg); 7RrN
        0,   // 4F: -    -      ; -
        40,  // 50: EX   (gg),BC; 7RrWwN
        40,  // 51: EX   (gg),DE; 7RrWwN
        40,  // 52: EX   (gg),HL; 7RrWwN
        0,   // 53: -    -      ; -
        40,  // 54: EX   (gg),IX; 7RrWwN
        40,  // 55: EX   (gg),IY; 7RrWwN
        40,  // 56: EX   (gg),SP; 7RrWwN
        0,   // 57: -    -      ; -
        0,   // 58: -    -      ; -
        0,   // 59: -    -      ; -
        0,   // 5A: -    -      ; -
        0,   // 5B: -    -      ; -
        0,   // 5C: -    -      ; -
        0,   // 5D: -    -      ; -
        0,   // 5E: -    -      ; -
        0,   // 5F: -    -      ; -
        38,  // 60: ADD  A,(gg) ; 7RN
        38,  // 61: ADC  A,(gg) ; 7RN
        38,  // 62: SUB  A,(gg) ; 7RN
        38,  // 63: SBC  A,(gg) ; 7RN
        38,  // 64: AND  A,(gg) ; 7RN
        38,  // 65: XOR  A,(gg) ; 7RN
        38,  // 66: OR   A,(gg) ; 7RN
        38,  // 67: CP   A,(gg) ; 7RN
        0,   // 68: -    -      ; -
        0,   // 69: -    -      ; -
        0,   // 6A: -    -      ; -
        0,   // 6B: -    -      ; -
        0,   // 6C: -    -      ; -
        0,   // 6D: -    -      ; -
        0,   // 6E: -    -      ; -
        0,   // 6F: -    -      ; -
        39,  // 70: ADD  HL,(gg); 7RrN
        39,  // 71: ADC  HL,(gg); 7RrN
        39,  // 72: SUB  HL,(gg); 7RrN
        39,  // 73: SBC  HL,(gg); 7RrN
        39,  // 74: AND  HL,(gg); 7RrN
        39,  // 75: XOR  HL,(gg); 7RrN
        39,  // 76: OR   HL,(gg); 7RrN
        39,  // 77: CP   HL,(gg); 7RrN
        0,   // 78: -    -      ; -
        0,   // 79: -    -      ; -
        0,   // 7A: -    -      ; -
        0,   // 7B: -    -      ; -
        0,   // 7C: -    -      ; -
        0,   // 7D: -    -      ; -
        0,   // 7E: -    -      ; -
        0,   // 7F: -    -      ; -
        0,   // 80: -    -      ; -
        0,   // 81: -    -      ; -
        0,   // 82: -    -      ; -
        0,   // 83: -    -      ; -
        0,   // 84: -    -      ; -
        0,   // 85: -    -      ; -
        0,   // 86: -    -      ; -
        37,  // 87: INC  (gg)   ; 7RNW
        0,   // 88: -    -      ; -
        0,   // 89: -    -      ; -
        0,   // 8A: -    -      ; -
        0,   // 8B: -    -      ; -
        0,   // 8C: -    -      ; -
        0,   // 8D: -    -      ; -
        0,   // 8E: -    -      ; -
        37,  // 8F: DEC  (gg)   ; 7RNW
        0,   // 90: -    -      ; -
        0,   // 91: -    -      ; -
        0,   // 92: -    -      ; -
        0,   // 93: -    -      ; -
        0,   // 94: -    -      ; -
        0,   // 95: -    -      ; -
        0,   // 96: -    -      ; -
        41,  // 97: INCW (gg)   ; 7RrNWw
        0,   // 98: -    -      ; -
        0,   // 99: -    -      ; -
        0,   // 9A: -    -      ; -
        0,   // 9B: -    -      ; -
        0,   // 9C: -    -      ; -
        0,   // 9D: -    -      ; -
        0,   // 9E: -    -      ; -
        41,  // 9F: DECW (gg)   ; 7RrNWw
        37,  // A0: RLC  (gg)   ; 7RNW
        37,  // A1: RRC  (gg)   ; 7RNW
        37,  // A2: RL   (gg)   ; 7RNW
        37,  // A3: RR   (gg)   ; 7RNW
        37,  // A4: SLA  (gg)   ; 7RNW
        37,  // A5: SRA  (gg)   ; 7RNW
        37,  // A6: SLL  (gg)   ; 7RNW
        37,  // A7: SRL  (gg)   ; 7RNW
        38,  // A8: BIT  0,(gg) ; 7RN
        38,  // A9: BIT  1,(gg) ; 7RN
        38,  // AA: BIT  2,(gg) ; 7RN
        38,  // AB: BIT  3,(gg) ; 7RN
        38,  // AC: BIT  4,(gg) ; 7RN
        38,  // AD: BIT  5,(gg) ; 7RN
        38,  // AE: BIT  6,(gg) ; 7RN
        38,  // AF: BIT  7,(gg) ; 7RN
        37,  // B0: RES  0,(gg) ; 7RNW
        37,  // B1: RES  1,(gg) ; 7RNW
        37,  // B2: RES  2,(gg) ; 7RNW
        37,  // B3: RES  3,(gg) ; 7RNW
        37,  // B4: RES  4,(gg) ; 7RNW
        37,  // B5: RES  5,(gg) ; 7RNW
        37,  // B6: RES  6,(gg) ; 7RNW
        37,  // B7: RES  7,(gg) ; 7RNW
        37,  // B8: SET  0,(gg) ; 7RNW
        37,  // B9: SET  1,(gg) ; 7RNW
        37,  // BA: SET  2,(gg) ; 7RNW
        37,  // BB: SET  3,(gg) ; 7RNW
        37,  // BC: SET  4,(gg) ; 7RNW
        37,  // BD: SET  5,(gg) ; 7RNW
        37,  // BE: SET  6,(gg) ; 7RNW
        37,  // BF: SET  7,(gg) ; 7RNW
        0,   // C0: -    -      ; -
        0,   // C1: -    -      ; -
        0,   // C2: -    -      ; -
        0,   // C3: -    -      ; -
        0,   // C4: -    -      ; -
        0,   // C5: -    -      ; -
        0,   // C6: -    -      ; -
        0,   // C7: -    -      ; -
        0,   // C8: -    -      ; -
        0,   // C9: -    -      ; -
        0,   // CA: -    -      ; -
        0,   // CB: -    -      ; -
        0,   // CC: -    -      ; -
        0,   // CD: -    -      ; -
        0,   // CE: -    -      ; -
        0,   // CF: -    -      ; -
        0,   // D0: -    -      ; -
        0,   // D1: -    -      ; -
        0,   // D2: -    -      ; -
        0,   // D3: -    -      ; -
        0,   // D4: -    -      ; -
        0,   // D5: -    -      ; -
        0,   // D6: -    -      ; -
        0,   // D7: -    -      ; -
        0,   // D8: -    -      ; -
        0,   // D9: -    -      ; -
        0,   // DA: -    -      ; -
        0,   // DB: -    -      ; -
        0,   // DC: -    -      ; -
        0,   // DD: -    -      ; -
        0,   // DE: -    -      ; -
        0,   // DF: -    -      ; -
        0,   // E0: -    -      ; -
        0,   // E1: -    -      ; -
        0,   // E2: -    -      ; -
        0,   // E3: -    -      ; -
        0,   // E4: -    -      ; -
        0,   // E5: -    -      ; -
        0,   // E6: -    -      ; -
        0,   // E7: -    -      ; -
        0,   // E8: -    -      ; -
        0,   // E9: -    -      ; -
        0,   // EA: -    -      ; -
        0,   // EB: -    -      ; -
        0,   // EC: -    -      ; -
        0,   // ED: -    -      ; -
        0,   // EE: -    -      ; -
        0,   // EF: -    -      ; -
        0,   // F0: -    -      ; -
        0,   // F1: -    -      ; -
        0,   // F2: -    -      ; -
        0,   // F3: -    -      ; -
        0,   // F4: -    -      ; -
        0,   // F5: -    -      ; -
        0,   // F6: -    -      ; -
        0,   // F7: -    -      ; -
        0,   // F8: -    -      ; -
        0,   // F9: -    -      ; -
        0,   // FA: -    -      ; -
        0,   // FB: -    -      ; -
        0,   // FC: -    -      ; -
        0,   // FD: -    -      ; -
        0,   // FE: -    -      ; -
        0,   // FF: -    -      ; -
};

constexpr uint8_t PAGE3[] = {
        0,   // 00: -    -      ; -
        0,   // 01: -    -      ; -
        0,   // 02: -    -      ; -
        0,   // 03: -    -      ; -
        0,   // 04: -    -      ; -
        0,   // 05: -    -      ; -
        0,   // 06: -    -      ; -
        0,   // 07: -    -      ; -
        0,   // 08: -    -      ; -
        0,   // 09: -    -      ; -
        0,   // 0A: -    -      ; -
        0,   // 0B: -    -      ; -
        0,   // 0C: -    -      ; -
        0,   // 0D: -    -      ; -
        0,   // 0E: -    -      ; -
        0,   // 0F: -    -      ; -
        0,   // 10: -    -      ; -
        0,   // 11: -    -      ; -
        0,   // 12: -    -      ; -
        0,   // 13: -    -      ; -
        0,   // 14: -    -      ; -
        0,   // 15: -    -      ; -
        0,   // 16: -    -      ; -
        0,   // 17: -    -      ; -
        0,   // 18: -    -      ; -
        0,   // 19: -    -      ; -
        0,   // 1A: -    -      ; -
        0,   // 1B: -    -      ; -
        0,   // 1C: -    -      ; -
        0,   // 1D: -    -      ; -
        0,   // 1E: -    -      ; -
        0,   // 1F: -    -      ; -
        42,  // 20: LD   (gg),B ; 7NW
        42,  // 21: LD   (gg),C ; 7NW
        42,  // 22: LD   (gg),D ; 7NW
        42,  // 23: LD   (gg),E ; 7NW
        42,  // 24: LD   (gg),H ; 7NW
        42,  // 25: LD   (gg),L ; 7NW
        42,  // 26: LD   (gg),A ; 7NW
        0,   // 27: -    -      ; -
        0,   // 28: -    -      ; -
        0,   // 29: -    -      ; -
        0,   // 2A: -    -      ; -
        0,   // 2B: -    -      ; -
        0,   // 2C: -    -      ; -
        0,   // 2D: -    -      ; -
        0,   // 2E: -    -      ; -
        0,   // 2F: -    -      ; -
        0,   // 30: -    -      ; -
        0,   // 31: -    -      ; -
        0,   // 32: -    -      ; -
        0,   // 33: -    -      ; -
        0,   // 34: -    -      ; -
        0,   // 35: -    -      ; -
        0,   // 36: -    -      ; -
        43,  // 37: LD   (gg),n ; 78WN
        29,  // 38: LD   BC,gg  ; 7N
        29,  // 39: LD   DE,gg  ; 7N
        29,  // 3A: LD   HL,gg  ; 7N
        0,   // 3B: -    -      ; -
        29,  // 3C: LD   IX,gg  ; 7N
        29,  // 3D: LD   IY,gg  ; 7N
        29,  // 3E: LD   SP,gg  ; 7N
        44,  // 3F: LDW  (gg),mn; 789NWw
        45,  // 40: LD   (gg),BC; 7NWw
        45,  // 41: LD   (gg),DE; 7NWw
        45,  // 42: LD   (gg),HL; 7NWw
        0,   // 43: -    -      ; -
        45,  // 44: LD   (gg),IX; 7NWw
        45,  // 45: LD   (gg),IY; 7NWw
        45,  // 46: LD   (gg),SP; 7NWw
        0,   // 47: -    -      ; -
        0,   // 48: -    -      ; -
        0,   // 49: -    -      ; -
        0,   // 4A: -    -      ; -
        0,   // 4B: -    -      ; -
        0,   // 4C: -    -      ; -
        0,   // 4D: -    -      ; -
        0,   // 4E: -    -      ; -
        0,   // 4F: -    -      ; -
        0,   // 50: -    -      ; -
        0,   // 51: -    -      ; -
        0,   // 52: -    -      ; -
        0,   // 53: -    -      ; -
        0,   // 54: -    -      ; -
        0,   // 55: -    -      ; -
        0,   // 56: -    -      ; -
        0,   // 57: -    -      ; -
        0,   // 58: -    -      ; -
        0,   // 59: -    -      ; -
        0,   // 5A: -    -      ; -
        0,   // 5B: -    -      ; -
        0,   // 5C: -    -      ; -
        0,   // 5D: -    -      ; -
        0,   // 5E: -    -      ; -
        0,   // 5F: -    -      ; -
        0,   // 60: -    -      ; -
        0,   // 61: -    -      ; -
        0,   // 62: -    -      ; -
        0,   // 63: -    -      ; -
        0,   // 64: -    -      ; -
        0,   // 65: -    -      ; -
        0,   // 66: -    -      ; -
        0,   // 67: -    -      ; -
        46,  // 68: ADD  (gg),n ; 78RNW
        46,  // 69: ADC  (gg),n ; 78RNW
        46,  // 6A: SUB  (gg),n ; 78RNW
        46,  // 6B: SBC  (gg),n ; 78RNW
        46,  // 6C: AND  (gg),n ; 78RNW
        46,  // 6D: XOR  (gg),n ; 78RNW
        46,  // 6E: OR   (gg),n ; 78RNW
        47,  // 6F: CP   (gg),n ; 78RN
        0,   // 70: -    -      ; -
        0,   // 71: -    -      ; -
        0,   // 72: -    -      ; -
        0,   // 73: -    -      ; -
        0,   // 74: -    -      ; -
        0,   // 75: -    -      ; -
        0,   // 76: -    -      ; -
        0,   // 77: -    -      ; -
        0,   // 78: -    -      ; -
        0,   // 79: -    -      ; -
        0,   // 7A: -    -      ; -
        0,   // 7B: -    -      ; -
        0,   // 7C: -    -      ; -
        0,   // 7D: -    -      ; -
        0,   // 7E: -    -      ; -
        0,   // 7F: -    -      ; -
        0,   // 80: -    -      ; -
        0,   // 81: -    -      ; -
        0,   // 82: -    -      ; -
        0,   // 83: -    -      ; -
        0,   // 84: -    -      ; -
        0,   // 85: -    -      ; -
        0,   // 86: -    -      ; -
        0,   // 87: -    -      ; -
        0,   // 88: -    -      ; -
        0,   // 89: -    -      ; -
        0,   // 8A: -    -      ; -
        0,   // 8B: -    -      ; -
        0,   // 8C: -    -      ; -
        0,   // 8D: -    -      ; -
        0,   // 8E: -    -      ; -
        0,   // 8F: -    -      ; -
        0,   // 90: -    -      ; -
        0,   // 91: -    -      ; -
        0,   // 92: -    -      ; -
        0,   // 93: -    -      ; -
        0,   // 94: -    -      ; -
        0,   // 95: -    -      ; -
        0,   // 96: -    -      ; -
        0,   // 97: -    -      ; -
        0,   // 98: -    -      ; -
        0,   // 99: -    -      ; -
        0,   // 9A: -    -      ; -
        0,   // 9B: -    -      ; -
        0,   // 9C: -    -      ; -
        0,   // 9D: -    -      ; -
        0,   // 9E: -    -      ; -
        0,   // 9F: -    -      ; -
        0,   // A0: -    -      ; -
        0,   // A1: -    -      ; -
        0,   // A2: -    -      ; -
        0,   // A3: -    -      ; -
        0,   // A4: -    -      ; -
        0,   // A5: -    -      ; -
        0,   // A6: -    -      ; -
        0,   // A7: -    -      ; -
        0,   // A8: -    -      ; -
        0,   // A9: -    -      ; -
        0,   // AA: -    -      ; -
        0,   // AB: -    -      ; -
        0,   // AC: -    -      ; -
        0,   // AD: -    -      ; -
        0,   // AE: -    -      ; -
        0,   // AF: -    -      ; -
        0,   // B0: -    -      ; -
        0,   // B1: -    -      ; -
        0,   // B2: -    -      ; -
        0,   // B3: -    -      ; -
        0,   // B4: -    -      ; -
        0,   // B5: -    -      ; -
        0,   // B6: -    -      ; -
        0,   // B7: -    -      ; -
        0,   // B8: -    -      ; -
        0,   // B9: -    -      ; -
        0,   // BA: -    -      ; -
        0,   // BB: -    -      ; -
        0,   // BC: -    -      ; -
        0,   // BD: -    -      ; -
        0,   // BE: -    -      ; -
        0,   // BF: -    -      ; -
        48,  // C0: JP   F,gg   ; 7mi@7N
        48,  // C1: JP   LT,gg  ; 7mi@7N
        48,  // C2: JP   LE,gg  ; 7mi@7N
        48,  // C3: JP   ULE,gg ; 7mi@7N
        48,  // C4: JP   OV,gg  ; 7mi@7N
        48,  // C5: JP   MI,gg  ; 7mi@7N
        48,  // C6: JP   Z,gg   ; 7mi@7N
        48,  // C7: JP   C,gg   ; 7mi@7N
        49,  // C8: JP   gg     ; 7ni
        48,  // C9: JP   GE,gg  ; 7mi@7N
        48,  // CA: JP   GT,gg  ; 7mi@7N
        48,  // CB: JP   UGT,gg ; 7mi@7N
        48,  // CC: JP   UGE,gg ; 7mi@7N
        48,  // CD: JP   NOV,gg ; 7mi@7N
        48,  // CE: JP   NZ,gg  ; 7mi@7N
        48,  // CF: JP   NC,gg  ; 7mi@7N
        50,  // D0: CALL F,gg   ; 7nwWi@7N
        50,  // D1: CALL LT,gg  ; 7nwWi@7N
        50,  // D2: CALL LE,gg  ; 7nwWi@7N
        50,  // D3: CALL ULE,gg ; 7nwWi@7N
        50,  // D4: CALL OV,gg  ; 7nwWi@7N
        50,  // D5: CALL MI,gg  ; 7nwWi@7N
        50,  // D6: CALL Z,gg   ; 7nwWi@7N
        50,  // D7: CALL C,gg   ; 7nwWi@7N
        51,  // D8: CALL gg     ; 7nwWi
        50,  // D9: CALL GE,gg  ; 7nwWi@7N
        50,  // DA: CALL GT,gg  ; 7nwWi@7N
        50,  // DB: CALL UGT,gg ; 7nwWi@7N
        50,  // DC: CALL UGE,gg ; 7nwWi@7N
        50,  // DD: CALL NOV,gg ; 7nwWi@7N
        50,  // DE: CALL NZ,gg  ; 7nwWi@7N
        50,  // DF: CALL NC,gg  ; 7nwWi@7N
        0,   // E0: -    -      ; -
        0,   // E1: -    -      ; -
        0,   // E2: -    -      ; -
        0,   // E3: -    -      ; -
        0,   // E4: -    -      ; -
        0,   // E5: -    -      ; -
        0,   // E6: -    -      ; -
        0,   // E7: -    -      ; -
        0,   // E8: -    -      ; -
        0,   // E9: -    -      ; -
        0,   // EA: -    -      ; -
        0,   // EB: -    -      ; -
        0,   // EC: -    -      ; -
        0,   // ED: -    -      ; -
        0,   // EE: -    -      ; -
        0,   // EF: -    -      ; -
        0,   // F0: -    -      ; -
        0,   // F1: -    -      ; -
        0,   // F2: -    -      ; -
        0,   // F3: -    -      ; -
        0,   // F4: -    -      ; -
        0,   // F5: -    -      ; -
        0,   // F6: -    -      ; -
        0,   // F7: -    -      ; -
        0,   // F8: -    -      ; -
        0,   // F9: -    -      ; -
        0,   // FA: -    -      ; -
        0,   // FB: -    -      ; -
        0,   // FC: -    -      ; -
        0,   // FD: -    -      ; -
        0,   // FE: -    -      ; -
        0,   // FF: -    -      ; -
};

constexpr uint8_t MV(uint8_t v0, uint8_t v1, uint8_t v2, uint8_t v3) {
    return (v0 << 0) | (v1 << 2) | (v2 << 4) | (v3 << 6);
}

uint8_t M_VALUE(const uint8_t map[], uint8_t opc) {
    const auto index = opc / 4;
    const auto shift = (opc % 4) * 2;
    return (map[index] >> shift) & 3;
}

constexpr const uint8_t *const PAGES[] = {
        PAGE0,
        PAGE1,
        PAGE2,
        PAGE3,
};

constexpr uint8_t PAGE_MAP[] = {
        MV(2, 2, 2, 2),  // E0~E3
        MV(2, 2, 2, 2),  // E4~E7
        MV(3, 3, 3, 3),  // E8~EB
        MV(3, 3, 3, 3),  // EC~EF
        MV(2, 2, 2, 2),  // F0~F3
        MV(3, 3, 3, 3),  // F4~F7
        MV(1, 1, 1, 1),  // F8~FB
        MV(1, 1, 1, 0),  // FC~FF
};

constexpr const uint8_t page_no(uint8_t opc) {
    return (opc < 0xE0) ? 0 : M_VALUE(PAGE_MAP, opc - 0xE0);
}

constexpr uint8_t PREFIX_LEN[] = {
        MV(1, 1, 1, 3),  // E0~E3
        MV(1, 1, 1, 2),  // E4~E7
        MV(1, 1, 1, 3),  // E8~EB
        MV(1, 1, 1, 2),  // EC~EF
        MV(2, 2, 2, 1),  // F0~F3
        MV(2, 2, 2, 1),  // F4~F7
        MV(1, 1, 1, 1),  // F8~FB
        MV(1, 1, 1, 0),  // FC~FF
};

uint8_t prefix_len(uint8_t opc) {
    return M_VALUE(PREFIX_LEN, opc - 0xE0);
}

struct StrBuffer {
    StrBuffer() : _out(0) {}
    operator const char *() const { return _buffer; }
    auto out() { return _out; }
    void reset(uint8_t out) { _out = out; }
    const char *append(const char *p) {
        for (; *p && *p != '@'; ++p) {
            if (_out < sizeof(_buffer) - 1)
                _buffer[_out++] = *p;
        }
        _buffer[_out] = 0;
        return p;
    }

private:
    char _buffer[20];
    uint8_t _out;
};

}  // namespace

bool InstTlcs90::valid(uint16_t addr, Mems *mems) {
    auto opc = mems->read(addr);
    const auto page = page_no(opc);
    if (page != 0)
        opc = mems->read(addr + prefix_len(opc));
    return PAGES[page][opc] != 0;
}

bool InstTlcs90::match(
        const Signals *begin, const Signals *end, const Signals *prefetch) {
    const auto fetch = prefetch ? prefetch : begin;
    if (!fetch->read())
        return false;
    auto opc = fetch->data;
    const auto page = page_no(opc);
    StrBuffer sequence;
    if (page != 0) {
        sequence.append(SEQUENCES[PAGE0[opc]]);
        auto len = prefix_len(opc);
        if (prefetch)
            --len;
        if (begin->diff(end) < len)
            return false;
        const auto next = begin->next(len);
        if (!next->read())
            return false;
        LOG_MATCH(cli.print("@@   match: page="));
        LOG_MATCH(cli.printDec(page));
        LOG_MATCH(cli.print(" prefix="));
        LOG_MATCH(cli.printHex(opc, 2));
        LOG_MATCH(cli.print(' '));
        LOG_MATCH(cli.printDec(prefix_len(opc)));
        opc = next->data;
        LOG_MATCH(cli.print(" opc="));
        LOG_MATCH(cli.printlnHex(opc, 2));
    } else {
        LOG_MATCH(cli.print("@@   match: page="));
        LOG_MATCH(cli.printDec(page));
        LOG_MATCH(cli.print(" opc="));
        LOG_MATCH(cli.printlnHex(opc, 2));
    }
    const auto out = sequence.out();
    auto seq = SEQUENCES[PAGES[page][opc]];
    while (*seq) {
        sequence.reset(out);
        seq = sequence.append(seq);
        if (matchSequence(begin, end, sequence, prefetch))
            return true;
        if (*seq == '@')
            ++seq;
    }
    return matchSequence(begin, end, INTERRUPT, prefetch);
}

bool InstTlcs90::matchSequence(const Signals *begin, const Signals *end,
        const char *seq, const Signals *prefetch) {
    const auto size = begin->diff(end);
    LOG_MATCH(cli.print("@@   match: seq="));
    LOG_MATCH(cli.print(seq));
    LOG_MATCH(cli.print(" size="));
    LOG_MATCH(cli.printlnDec(size));
    LOG_MATCH(cli.print("       begin="));
    LOG_MATCH(begin->print());
    if (prefetch) {
        LOG_MATCH(cli.print("    prefetch="));
        LOG_MATCH(prefetch->print());
    }
    const Signals *r = nullptr;
    const Signals *w = nullptr;
    _nexti = -1;
    uint16_t next = 0;
    uint16_t addr = 0;
    int16_t disp = 0;
    for (auto i = 0; i < size; ++i) {
        const auto s = begin->next(i);
        LOG_MATCH(cli.print("         "));
        LOG_MATCH(cli.print(*seq));
        LOG_MATCH(cli.print(" s="));
        LOG_MATCH(s->print());
        if (s->read()) {
            switch (*seq) {
            case '0':
                if (prefetch)
                    --i;
                next = (prefetch ? prefetch : begin)->addr + 1;
                break;
            case '2':
            case '3':
            case '4':
            case '7':
            case '8':
            case '9':
                if (s->addr == next) {
                    ++next;
                    if (*seq < '7') {
                        addr >>= 8;
                        addr |= s->data << 8;
                    }
                    break;
                }
                goto not_matched;
            case 'm':
                if (s->addr == next) {
                    ++next;
                    break;
                }
                goto not_matched;
            case 'n':
                if (s->addr == next)
                    break;
                goto not_matched;
            case 'N':
                if (s->addr == next) {
                    _nexti = i;
                    break;
                }
                goto not_matched;
            case 'i':
                if (s->addr != next) {
                    _nexti = i;
                    break;
                }
                goto not_matched;
            case 'j':  // 2:j
                disp = static_cast<int8_t>(addr >> 8);
                goto branch;
            case 'k':  // 2:3:k
                disp = static_cast<int16_t>(addr);
            branch:
                if (s->addr == static_cast<uint16_t>(next + disp)) {
                    _nexti = i;
                    break;
                }
                goto not_matched;
            case 'J':  // 2:3:J, R:r:J, r:r:J
                if (s->addr == addr) {
                    _nexti = i;
                    break;
                }
                goto not_matched;
            case 'V':
                if ((s->addr & ~0x78) == 0) {
                    _nexti = i;
                    break;
                }
                goto not_matched;
            case 'R':
                r = s;
                addr = s->data << 8;
                break;
            case 'E':
                if (s->addr >= 0xFF00) {
                    r = s;
                    break;
                }
                goto not_matched;
            case 'r':  // R:r, E:r
                if (s->addr == r->addr + 1U) {
                    r = s;
                    addr >>= 8;
                    addr |= s->data << 8;
                    break;
                }
                goto not_matched;
            default:
                goto not_matched;
            }
        } else if (s->write()) {
            switch (*seq) {
            case 'W':  // R:W, E:W, W:w, w:W
                if (w == nullptr || s->addr == w->addr - 1U ||
                        (r && s->addr == r->addr)) {
                    w = s;
                    break;
                }
                goto not_matched;
            case 'F':
                if (s->addr >= 0xFF00) {
                    w = s;
                    break;
                }
                goto not_matched;
            case 'w':  // W:w, w:W, F:w
                if (w == nullptr || s->addr == w->addr + 1U) {
                    w = s;
                    break;
                }
                goto not_matched;
            default:
                goto not_matched;
            }
        }
        if (*++seq == 0) {
            _matched = (_nexti == i) ? i : i + 1;
            LOG_MATCH(cli.print("@@   MATCHED="));
            LOG_MATCH(cli.printlnDec(i));
            LOG_MATCH(cli.print("       nexti="));
            LOG_MATCH(begin->next(_nexti)->print());
            return true;
        }
    }
not_matched:
    LOG_MATCH(cli.print("@@   NOT MATCHED "));
    LOG_MATCH(cli.println(*seq));
    return false;
}

}  // namespace tlcs90
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
