#include "inst_ins8070.h"
#include <string.h>
#include "debugger.h"
#include "mems_ins8070.h"
#include "regs_ins8070.h"

namespace debugger {
namespace ins8070 {

namespace {

constexpr uint8_t E(AddrMode mode, uint8_t seq) {
    return (uint16_t(mode) << 5) | seq;
}

constexpr AddrMode addr_mode(uint8_t entry) {
    return AddrMode(entry >> 5);
}

constexpr uint8_t inst_seq(uint8_t entry) {
    return entry & 0x1F;
}

/**
# 1: instruction code (next=&1)
# 2: instruction operands (disp, low(addr), ++next)
# 3: instruction operands (high(addr), ++next)
# R: read 1 byte (low(addr))
# r: read 1 byte at address R+1 (high(addr))
# Q: read 1 byte at PC-relative (next+disp+1)
# q: read 1 byet at PC-relative (Q+1)
# W: write 1 byte, equals to R if exists
# w: write 1 byte at address W+1
# X: write 1 byte at PC-relative (next+disp+1), equals to Q if exists
# x: write 1 byet at PC-relative (X+1)
# E: read 1 byte from direct page (FFxx)
# e: read 1 byte at address (E+1)
# F: write 1 byte to direct page (FFxx), equals to E if exists
# f: write 1 byte at address F+1
# V: read 1 byte from address 0020-003E (low(addr))
# v: read 1 byte from address V+1 (high(addr))
# S: read 1+ byte sequencially next address (S+1)
# @: separator for internal RAM access cycles
# N: next sequential instruction address (next)
# n: next unknown instruction
# s: next instruction address of SSM (next or next+2)
# A: 16-bit absolute address (addr)
# B: 8-bit relative branch (next or next+disp)
# P: poped from stack address (addr)
*/
constexpr const char *const BUS_SEQUENCES[/*seq*/] = {
        "",             //  0
        "1N",           //  1
        "1wWN@1N",      //  2
        "1WN@1N",       //  3
        "1wWVvA@1VvA",  //  4
        "12wW3A@123A",  //  5
        "12wW3N@123N",  //  6
        "123A",         //  7
        "123N",         //  8
        "12B",          //  9
        "1Ss@1s",       // 10
        "1RN@1N",       // 11
        "12N",          // 12
        "1RrN@1N",      // 13
        "1n",           // 14
        "1RrP@1n",      // 15
        "12n",          // 16
        "12QqN@12N",    // 17
        "12RrN@12N",    // 18
        "12EeN@12N",    // 19
        "12XxN@12N",    // 20
        "12WwN@12N",    // 21
        "12FfN@12N",    // 22
        "12QXN@12N",    // 23
        "12RWN@12N",    // 24
        "12EFN@12N",    // 25
        "12QN@12N",     // 26
        "12RN@12N",     // 27
        "12EN@12N",     // 28
        "12XN@12N",     // 29
        "12WN@12N",     // 30
        "12FN@12N",     // 31
};

constexpr uint8_t INST_LENS[] = {
        0,  //  0: -
        1,  //  1: 1N
        1,  //  2: 1wWN@1N
        1,  //  3: 1WN@1N
        1,  //  4: 1wWVvA@1VvA
        3,  //  5: 12wW3A@123A
        3,  //  6: 12wW3N@123N
        3,  //  7: 123A
        3,  //  8: 123N
        2,  //  9: 12B
        1,  // 10: 1Ss@1s
        1,  // 11: 1RN@1N
        2,  // 12: 12N
        1,  // 13: 1RrN@1N
        1,  // 14: 1n
        1,  // 15: 1RrP@1n
        2,  // 16: 12n
        2,  // 17: 12QqN@12N
        2,  // 18: 12RrN@12N
        2,  // 19: 12EeN@12N
        2,  // 20: 12XxN@12N
        2,  // 21: 12WwN@12N
        2,  // 22: 12FfN@12N
        2,  // 23: 12QXN@12N
        2,  // 24: 12RWN@12N
        2,  // 25: 12EFN@12N
        2,  // 26: 12QN@12N
        2,  // 27: 12RN@12N
        2,  // 28: 12EN@12N
        2,  // 29: 12XN@12N
        2,  // 30: 12WN@12N
        2,  // 31: 12FN@12N
};

constexpr uint8_t BUS_CYCLES[] = {
        0,  //  0: -
        1,  //  1: 1N
        3,  //  2: 1wWN@1N
        2,  //  3: 1WN@1N
        5,  //  4: 1wWVvA@1VvA
        5,  //  5: 12wW3A@123A
        5,  //  6: 12wW3N@123N
        3,  //  7: 123A
        3,  //  8: 123N
        2,  //  9: 12B
        2,  // 10: 1Ss@1s
        2,  // 11: 1RN@1N
        2,  // 12: 12N
        3,  // 13: 1RrN@1N
        1,  // 14: 1n
        3,  // 15: 1RrP@1n
        2,  // 16: 12n
        4,  // 17: 12QqN@12N
        4,  // 18: 12RrN@12N
        4,  // 19: 12EeN@12N
        4,  // 20: 12XxN@12N
        4,  // 21: 12WwN@12N
        4,  // 22: 12FfN@12N
        4,  // 23: 12QXN@12N
        4,  // 24: 12RWN@12N
        4,  // 25: 12EFN@12N
        3,  // 26: 12QN@12N
        3,  // 27: 12RN@12N
        3,  // 28: 12EN@12N
        3,  // 29: 12XN@12N
        3,  // 30: 12WN@12N
        3,  // 31: 12FN@12N
};

constexpr uint8_t EXTERNAL_CYCLES[] = {
        0,  //  0: -
        1,  //  1: 1N
        1,  //  2: 1wWN@1N
        1,  //  3: 1WN@1N
        3,  //  4: 1wWVvA@1VvA
        3,  //  5: 12wW3A@123A
        3,  //  6: 12wW3N@123N
        3,  //  7: 123A
        3,  //  8: 123N
        2,  //  9: 12B
        1,  // 10: 1Ss@1s
        1,  // 11: 1RN@1N
        2,  // 12: 12N
        1,  // 13: 1RrN@1N
        1,  // 14: 1n
        1,  // 15: 1RrP@1n
        2,  // 16: 12n
        2,  // 17: 12QqN@12N
        2,  // 18: 12RrN@12N
        2,  // 19: 12EeN@12N
        2,  // 20: 12XxN@12N
        2,  // 21: 12WwN@12N
        2,  // 22: 12FfN@12N
        2,  // 23: 12QXN@12N
        2,  // 24: 12RWN@12N
        2,  // 25: 12EFN@12N
        2,  // 26: 12QN@12N
        2,  // 27: 12RN@12N
        2,  // 28: 12EN@12N
        2,  // 29: 12XN@12N
        2,  // 30: 12WN@12N
        2,  // 31: 12FN@12N
};

constexpr uint8_t INST_TABLE[] = {
        E(M_NO, 1),     // 00: NOP  -        ; 1:N
        E(M_NO, 1),     // 01: XCH  A,E      ; 1:N
        0,              // 02: -    -        ; -
        0,              // 03: -    -        ; -
        0,              // 04: -    -        ; -
        0,              // 05: -    -        ; -
        E(M_NO, 1),     // 06: LD   A,S      ; 1:N
        E(M_NO, 1),     // 07: LD   S,A      ; 1:N
        E(M_PUSH, 2),   // 08: PUSH EA       ; 1:w:W:N
        E(M_NO, 1),     // 09: LD   T,EA     ; 1:N
        E(M_PUSH, 3),   // 0A: PUSH A        ; 1:W:N
        E(M_NO, 1),     // 0B: LD   EA,T     ; 1:N
        E(M_NO, 1),     // 0C: SR   EA       ; 1:N
        E(M_NO, 1),     // 0D: DIV  EA,T     ; 1:N
        E(M_NO, 1),     // 0E: SL   A        ; 1:N
        E(M_NO, 1),     // 0F: SL   EA       ; 1:N
        E(M_PUSH, 4),   // 10: CALL 0        ; 1:w:W:V:v:A
        E(M_PUSH, 4),   // 11: CALL 1        ; 1:w:W:V:v:A
        E(M_PUSH, 4),   // 12: CALL 2        ; 1:w:W:V:v:A
        E(M_PUSH, 4),   // 13: CALL 3        ; 1:w:W:V:v:A
        E(M_PUSH, 4),   // 14: CALL 4        ; 1:w:W:V:v:A
        E(M_PUSH, 4),   // 15: CALL 5        ; 1:w:W:V:v:A
        E(M_PUSH, 4),   // 16: CALL 6        ; 1:w:W:V:v:A
        E(M_PUSH, 4),   // 17: CALL 7        ; 1:w:W:V:v:A
        E(M_PUSH, 4),   // 18: CALL 8        ; 1:w:W:V:v:A
        E(M_PUSH, 4),   // 19: CALL 9        ; 1:w:W:V:v:A
        E(M_PUSH, 4),   // 1A: CALL 10       ; 1:w:W:V:v:A
        E(M_PUSH, 4),   // 1B: CALL 11       ; 1:w:W:V:v:A
        E(M_PUSH, 4),   // 1C: CALL 12       ; 1:w:W:V:v:A
        E(M_PUSH, 4),   // 1D: CALL 13       ; 1:w:W:V:v:A
        E(M_PUSH, 4),   // 1E: CALL 14       ; 1:w:W:V:v:A
        E(M_PUSH, 4),   // 1F: CALL 15       ; 1:w:W:V:v:A
        E(M_PUSH, 5),   // 20: JSR  a        ; 1:2:w:W:3:A
        0,              // 21: -    -        ; -
        E(M_PUSH, 6),   // 22: PLI  P2,=a    ; 1:2:w:W:3:N
        E(M_PUSH, 6),   // 23: PLI  P3,=a    ; 1:2:w:W:3:N
        E(M_NO, 7),     // 24: JMP  a        ; 1:2:3:A
        E(M_NO, 8),     // 25: LD   SP,=a    ; 1:2:3:N
        E(M_NO, 8),     // 26: LD   P2,=a    ; 1:2:3:N
        E(M_NO, 8),     // 27: LD   P3,=a    ; 1:2:3:N
        0,              // 28: -    -        ; -
        0,              // 29: -    -        ; -
        0,              // 2A: -    -        ; -
        0,              // 2B: -    -        ; -
        E(M_NO, 1),     // 2C: MPY  EA,T     ; 1:N
        E(M_NO, 9),     // 2D: BND  a        ; 1:2:B
        E(M_SSM, 10),   // 2E: SSM  P2       ; 1:S:s
        E(M_SSM, 10),   // 2F: SSM  P3       ; 1:S:s
        E(M_NO, 1),     // 30: LD   EA,PC    ; 1:N
        E(M_NO, 1),     // 31: LD   EA,SP    ; 1:N
        E(M_NO, 1),     // 32: LD   EA,P2    ; 1:N
        E(M_NO, 1),     // 33: LD   EA,P3    ; 1:N
        0,              // 34: -    -        ; -
        0,              // 35: -    -        ; -
        0,              // 36: -    -        ; -
        0,              // 37: -    -        ; -
        E(M_POP, 11),   // 38: POP  A        ; 1:R:N
        E(M_NO, 12),    // 39: AND  S,=n     ; 1:2:N
        E(M_POP, 13),   // 3A: POP  EA       ; 1:R:r:N
        E(M_NO, 12),    // 3B: OR   S,=n     ; 1:2:N
        E(M_NO, 1),     // 3C: SR   A        ; 1:N
        E(M_NO, 1),     // 3D: SRL  A        ; 1:N
        E(M_NO, 1),     // 3E: RR   A        ; 1:N
        E(M_NO, 1),     // 3F: RRL  A        ; 1:N
        E(M_NO, 1),     // 40: LD   A,E      ; 1:N
        0,              // 41: -    -        ; -
        0,              // 42: -    -        ; -
        0,              // 43: -    -        ; -
        E(M_NO, 14),    // 44: LD   PC,EA    ; 1:n
        E(M_NO, 1),     // 45: LD   SP,EA    ; 1:N
        E(M_NO, 1),     // 46: LD   P2,EA    ; 1:N
        E(M_NO, 1),     // 47: LD   P3,EA    ; 1:N
        E(M_NO, 1),     // 48: LD   E,A      ; 1:N
        0,              // 49: -    -        ; -
        0,              // 4A: -    -        ; -
        0,              // 4B: -    -        ; -
        E(M_NO, 14),    // 4C: XCH  EA,PC    ; 1:n
        E(M_NO, 1),     // 4D: XCH  EA,SP    ; 1:N
        E(M_NO, 1),     // 4E: XCH  EA,P2    ; 1:N
        E(M_NO, 1),     // 4F: XCH  EA,P3    ; 1:N
        E(M_NO, 1),     // 50: AND  A,E      ; 1:N
        0,              // 51: -    -        ; -
        0,              // 52: -    -        ; -
        0,              // 53: -    -        ; -
        E(M_PUSH, 2),   // 54: PUSH PC       ; 1:w:W:N
        0,              // 55: -    -        ; -
        E(M_PUSH, 2),   // 56: PUSH P2       ; 1:w:W:N
        E(M_PUSH, 2),   // 57: PUSH P3       ; 1:w:W:N
        E(M_NO, 1),     // 58: OR   A,E      ; 1:N
        0,              // 59: -    -        ; -
        0,              // 5A: -    -        ; -
        0,              // 5B: -    -        ; -
        E(M_POP, 15),   // 5C: RET  -        ; 1:R:r:P
        0,              // 5D: -    -        ; -
        E(M_POP, 13),   // 5E: POP  P2       ; 1:R:r:N
        E(M_POP, 13),   // 5F: POP  P3       ; 1:R:r:N
        E(M_NO, 1),     // 60: XOR  A,E      ; 1:N
        0,              // 61: -    -        ; -
        0,              // 62: -    -        ; -
        0,              // 63: -    -        ; -
        E(M_NO, 9),     // 64: BP   a        ; 1:2:B
        0,              // 65: -    -        ; -
        E(M_NO, 16),    // 66: BP   d,P2     ; 1:2:n
        E(M_NO, 16),    // 67: BP   d,P3     ; 1:2:n
        0,              // 68: -    -        ; -
        0,              // 69: -    -        ; -
        0,              // 6A: -    -        ; -
        0,              // 6B: -    -        ; -
        E(M_NO, 9),     // 6C: BZ   a        ; 1:2:B
        0,              // 6D: -    -        ; -
        E(M_NO, 16),    // 6E: BZ   d,P2     ; 1:2:n
        E(M_NO, 16),    // 6F: BZ   d,P3     ; 1:2:n
        E(M_NO, 1),     // 70: ADD  A,E      ; 1:N
        0,              // 71: -    -        ; -
        0,              // 72: -    -        ; -
        0,              // 73: -    -        ; -
        E(M_NO, 9),     // 74: BRA  a        ; 1:2:B
        0,              // 75: -    -        ; -
        E(M_NO, 16),    // 76: BRA  d,P2     ; 1:2:n
        E(M_NO, 16),    // 77: BRA  d,P3     ; 1:2:n
        E(M_NO, 1),     // 78: SUB  A,E      ; 1:N
        0,              // 79: -    -        ; -
        0,              // 7A: -    -        ; -
        0,              // 7B: -    -        ; -
        E(M_NO, 9),     // 7C: BNZ  a        ; 1:2:B
        0,              // 7D: -    -        ; -
        E(M_NO, 16),    // 7E: BNZ  d,P2     ; 1:2:n
        E(M_NO, 16),    // 7F: BNZ  d,P3     ; 1:2:n
        E(M_DISP, 17),  // 80: LD   EA,d,PC  ; 1:2:Q:q:N
        E(M_DISP, 18),  // 81: LD   EA,d,SP  ; 1:2:R:r:N
        E(M_DISP, 18),  // 82: LD   EA,d,P2  ; 1:2:R:r:N
        E(M_DISP, 18),  // 83: LD   EA,d,P3  ; 1:2:R:r:N
        E(M_NO, 8),     // 84: LD   EA,=nn   ; 1:2:3:N
        E(M_DIR, 19),   // 85: LD   EA,n     ; 1:2:E:e:N
        E(M_AUTO, 18),  // 86: LD   EA,@d,P2 ; 1:2:R:r:N
        E(M_AUTO, 18),  // 87: LD   EA,@d,P3 ; 1:2:R:r:N
        E(M_DISP, 20),  // 88: ST   EA,d,PC  ; 1:2:X:x:N
        E(M_DISP, 21),  // 89: ST   EA,d,SP  ; 1:2:W:w:N
        E(M_DISP, 21),  // 8A: ST   EA,d,P2  ; 1:2:W:w:N
        E(M_DISP, 21),  // 8B: ST   EA,d,P3  ; 1:2:W:w:N
        0,              // 8C: -    -        ; -
        E(M_DIR, 22),   // 8D: ST   EA,n     ; 1:2:F:f:N
        E(M_AUTO, 21),  // 8E: ST   EA,@d,P2 ; 1:2:W:w:N
        E(M_AUTO, 21),  // 8F: ST   EA,@d,P3 ; 1:2:W:w:N
        E(M_DISP, 23),  // 90: ILD  A,d,PC   ; 1:2:Q:X:N
        E(M_DISP, 24),  // 91: ILD  A,d,SP   ; 1:2:R:W:N
        E(M_DISP, 24),  // 92: ILD  A,d,P2   ; 1:2:R:W:N
        E(M_DISP, 24),  // 93: ILD  A,d,P3   ; 1:2:R:W:N
        0,              // 94: -    -        ; -
        E(M_DIR, 25),   // 95: ILD  A,n      ; 1:2:E:F:N
        E(M_AUTO, 24),  // 96: ILD  A,@d,P2  ; 1:2:R:W:N
        E(M_AUTO, 24),  // 97: ILD  A,@d,P3  ; 1:2:R:W:N
        E(M_DISP, 23),  // 98: DLD  A,d,PC   ; 1:2:Q:X:N
        E(M_DISP, 24),  // 99: DLD  A,d,SP   ; 1:2:R:W:N
        E(M_DISP, 24),  // 9A: DLD  A,d,P2   ; 1:2:R:W:N
        E(M_DISP, 24),  // 9B: DLD  A,d,P3   ; 1:2:R:W:N
        0,              // 9C: -    -        ; -
        E(M_DIR, 25),   // 9D: DLD  A,n      ; 1:2:E:F:N
        E(M_AUTO, 24),  // 9E: DLD  A,@d,P2  ; 1:2:R:W:N
        E(M_AUTO, 24),  // 9F: DLD  A,@d,P3  ; 1:2:R:W:N
        E(M_DISP, 17),  // A0: LD   T,d,PC   ; 1:2:Q:q:N
        E(M_DISP, 18),  // A1: LD   T,d,SP   ; 1:2:R:r:N
        E(M_DISP, 18),  // A2: LD   T,d,P2   ; 1:2:R:r:N
        E(M_DISP, 18),  // A3: LD   T,d,P3   ; 1:2:R:r:N
        E(M_NO, 8),     // A4: LD   T,=nn    ; 1:2:3:N
        E(M_DIR, 19),   // A5: LD   T,n      ; 1:2:E:e:N
        E(M_AUTO, 18),  // A6: LD   T,@d,P2  ; 1:2:R:r:N
        E(M_AUTO, 18),  // A7: LD   T,@d,P3  ; 1:2:R:r:N
        0,              // A8: -    -        ; -
        0,              // A9: -    -        ; -
        0,              // AA: -    -        ; -
        0,              // AB: -    -        ; -
        0,              // AC: -    -        ; -
        0,              // AD: -    -        ; -
        0,              // AE: -    -        ; -
        0,              // AF: -    -        ; -
        E(M_DISP, 17),  // B0: ADD  EA,d,PC  ; 1:2:Q:q:N
        E(M_DISP, 18),  // B1: ADD  EA,d,SP  ; 1:2:R:r:N
        E(M_DISP, 18),  // B2: ADD  EA,d,P2  ; 1:2:R:r:N
        E(M_DISP, 18),  // B3: ADD  EA,d,P3  ; 1:2:R:r:N
        E(M_NO, 8),     // B4: ADD  EA,=nn   ; 1:2:3:N
        E(M_DIR, 19),   // B5: ADD  EA,n     ; 1:2:E:e:N
        E(M_AUTO, 18),  // B6: ADD  EA,@d,P2 ; 1:2:R:r:N
        E(M_AUTO, 18),  // B7: ADD  EA,@d,P3 ; 1:2:R:r:N
        E(M_DISP, 17),  // B8: SUB  EA,d,PC  ; 1:2:Q:q:N
        E(M_DISP, 18),  // B9: SUB  EA,d,SP  ; 1:2:R:r:N
        E(M_DISP, 18),  // BA: SUB  EA,d,P2  ; 1:2:R:r:N
        E(M_DISP, 18),  // BB: SUB  EA,d,P3  ; 1:2:R:r:N
        E(M_NO, 8),     // BC: SUB  EA,=nn   ; 1:2:3:N
        E(M_DIR, 19),   // BD: SUB  EA,n     ; 1:2:E:e:N
        E(M_AUTO, 18),  // BE: SUB  EA,@d,P2 ; 1:2:R:r:N
        E(M_AUTO, 18),  // BF: SUB  EA,@d,P3 ; 1:2:R:r:N
        E(M_DISP, 26),  // C0: LD   A,d,PC   ; 1:2:Q:N
        E(M_DISP, 27),  // C1: LD   A,d,SP   ; 1:2:R:N
        E(M_DISP, 27),  // C2: LD   A,d,P2   ; 1:2:R:N
        E(M_DISP, 27),  // C3: LD   A,d,P3   ; 1:2:R:N
        E(M_NO, 12),    // C4: LD   A,=n     ; 1:2:N
        E(M_DIR, 28),   // C5: LD   A,n      ; 1:2:E:N
        E(M_AUTO, 27),  // C6: LD   A,@d,P2  ; 1:2:R:N
        E(M_AUTO, 27),  // C7: LD   A,@d,P3  ; 1:2:R:N
        E(M_DISP, 29),  // C8: ST   A,d,PC   ; 1:2:X:N
        E(M_DISP, 30),  // C9: ST   A,d,SP   ; 1:2:W:N
        E(M_DISP, 30),  // CA: ST   A,d,P2   ; 1:2:W:N
        E(M_DISP, 30),  // CB: ST   A,d,P3   ; 1:2:W:N
        0,              // CC: -    -        ; -
        E(M_DIR, 31),   // CD: ST   A,n      ; 1:2:F:N
        E(M_AUTO, 30),  // CE: ST   A,@d,P2  ; 1:2:W:N
        E(M_AUTO, 30),  // CF: ST   A,@d,P3  ; 1:2:W:N
        E(M_DISP, 26),  // D0: AND  A,d,PC   ; 1:2:Q:N
        E(M_DISP, 27),  // D1: AND  A,d,SP   ; 1:2:R:N
        E(M_DISP, 27),  // D2: AND  A,d,P2   ; 1:2:R:N
        E(M_DISP, 27),  // D3: AND  A,d,P3   ; 1:2:R:N
        E(M_NO, 12),    // D4: AND  A,=n     ; 1:2:N
        E(M_DIR, 28),   // D5: AND  A,n      ; 1:2:E:N
        E(M_AUTO, 27),  // D6: AND  A,@d,P2  ; 1:2:R:N
        E(M_AUTO, 27),  // D7: AND  A,@d,P3  ; 1:2:R:N
        E(M_DISP, 26),  // D8: OR   A,d,PC   ; 1:2:Q:N
        E(M_DISP, 27),  // D9: OR   A,d,SP   ; 1:2:R:N
        E(M_DISP, 27),  // DA: OR   A,d,P2   ; 1:2:R:N
        E(M_DISP, 27),  // DB: OR   A,d,P3   ; 1:2:R:N
        E(M_NO, 12),    // DC: OR   A,=n     ; 1:2:N
        E(M_DIR, 28),   // DD: OR   A,n      ; 1:2:E:N
        E(M_AUTO, 27),  // DE: OR   A,@d,P2  ; 1:2:R:N
        E(M_AUTO, 27),  // DF: OR   A,@d,P3  ; 1:2:R:N
        0,              // E0: XOR  A,d,PC   ; 1:2:Q:N
        E(M_DISP, 27),  // E1: XOR  A,d,SP   ; 1:2:R:N
        E(M_DISP, 27),  // E2: XOR  A,d,P2   ; 1:2:R:N
        E(M_DISP, 27),  // E3: XOR  A,d,P3   ; 1:2:R:N
        E(M_NO, 12),    // E4: XOR  A,=n     ; 1:2:N
        E(M_DIR, 28),   // E5: XOR  A,n      ; 1:2:E:N
        E(M_AUTO, 27),  // E6: XOR  A,@d,P2  ; 1:2:R:N
        E(M_AUTO, 27),  // E7: XOR  A,@d,P3  ; 1:2:R:N
        0,              // E8: -    -        ;
        0,              // E9: -    -        ;
        0,              // EA: -    -        ;
        0,              // EB: -    -        ;
        0,              // EC: -    -        ;
        0,              // ED: -    -        ;
        0,              // EE: -    -        ;
        0,              // EF: -    -        ;
        E(M_DISP, 26),  // F0: ADD  A,d,PC   ; 1:2:Q:N
        E(M_DISP, 27),  // F1: ADD  A,d,SP   ; 1:2:R:N
        E(M_DISP, 27),  // F2: ADD  A,d,P2   ; 1:2:R:N
        E(M_DISP, 27),  // F3: ADD  A,d,P3   ; 1:2:R:N
        E(M_NO, 12),    // F4: ADD  A,=n     ; 1:2:N
        E(M_DIR, 28),   // F5: ADD  A,n      ; 1:2:E:N
        E(M_AUTO, 27),  // F6: ADD  A,@d,P2  ; 1:2:R:N
        E(M_AUTO, 27),  // F7: ADD  A,@d,P3  ; 1:2:R:N
        E(M_DISP, 27),  // F8: SUB  A,Q,PC   ; 1:2:Q:N
        E(M_DISP, 27),  // F9: SUB  A,d,SP   ; 1:2:R:N
        E(M_DISP, 27),  // FA: SUB  A,d,P2   ; 1:2:R:N
        E(M_DISP, 27),  // FB: SUB  A,d,P3   ; 1:2:R:N
        E(M_NO, 12),    // FC: SUB  A,=n     ; 1:2:N
        E(M_DIR, 28),   // FD: SUB  A,n      ; 1:2:E:N
        E(M_AUTO, 27),  // FE: SUB  A,@d,P2  ; 1:2:R:N
        E(M_AUTO, 27),  // FF: SUB  A,@d,P3  ; 1:2:R:N
};
}  // namespace

bool InstIns8070::get(uint16_t addr) {
    opc = Memory.raw_read(addr);
    seq = inst_seq(INST_TABLE[opc]);
    return len() != 0;
}

bool InstIns8070::get(const Signals *signals) {
    opc = signals->data;
    seq = inst_seq(INST_TABLE[opc]);
    return len() != 0;
}

uint8_t InstIns8070::len() const {
    return INST_LENS[seq];
}

uint8_t InstIns8070::busCycles() const {
    return BUS_CYCLES[seq];
}

uint8_t InstIns8070::externalCycles() const {
    return EXTERNAL_CYCLES[seq];
}

AddrMode InstIns8070::addrMode() const {
    return addr_mode(INST_TABLE[opc]);
}

bool InstIns8070::match(const Signals *begin, const Signals *end) {
    if (!begin->read() || !get(begin))
        return false;
    const auto size = begin->diff(end);
    const auto *sequence = BUS_SEQUENCES[seq];
    matched = matchSequence(begin, size, sequence);
    if (matched)
        return true;
    const auto internal = strchr(sequence, '@');
    if (internal) {
        matched = matchSequence(begin, size, internal + 1);
        return matched != 0;
    }
    return false;
}

uint8_t InstIns8070::matchSequence(
        const Signals *begin, uint8_t size, const char *seq) const {
    LOG_MATCH(cli.print("@@   match: seq="));
    LOG_MATCH(cli.print(seq));
    LOG_MATCH(cli.print(" size="));
    LOG_MATCH(cli.printlnDec(size));
    LOG_MATCH(cli.print("       begin="));
    LOG_MATCH(begin->print());
    const Signals *r = nullptr;
    const Signals *w = nullptr;
    uint8_t cycles = 0;
    uint16_t next = 0;
    uint16_t addr = 0;
    int8_t disp = 0;
    for (auto i = 0; i < size; ++i, ++cycles, ++seq) {
        const auto s = begin->next(i);
        LOG_MATCH(cli.print("         "));
        LOG_MATCH(cli.print(*seq));
        LOG_MATCH(cli.print(" s="));
        LOG_MATCH(s->print());
        if (s->read()) {
            switch (*seq) {
            case '1':
                next = begin->addr + 1;
                continue;
            case '2':  // 1:2
                if (s->addr == next) {
                    disp = addr = s->data;
                    ++next;
                    continue;
                }
                break;
            case '3':  // 2:3
                if (s->addr == next) {
                    addr |= s->data << 8;
                    ++next;
                    continue;
                }
                break;
            case 'N':  // 1:N or 1:2:N or 1:2:3:N
                if (s->addr == next)
                    goto return_matched;
                break;
            case 'n':
            return_matched:
                LOG_MATCH(cli.print("@@   MATCHED="));
                LOG_MATCH(cli.printlnDec(cycles));
                return cycles;
            case 'A':  // 2:3:A or V:v:A
            case 'P':  // R:r:P
                if (s->addr == addr + 1U)
                    goto return_matched;
                break;
            case 'B':  // 2:B
                if (s->addr == next || s->addr == uint16_t(next + disp))
                    goto return_matched;
                break;
            case 'Q':  // 2:Q
                if (s->addr == uint16_t(next + disp - 1))
                    goto read_low;
                break;
            case 'E':  // 2:E
                if (s->addr == 0xFF00U + (uint8_t)disp)
                    goto read_low;
                break;
            case 'V':
                if ((s->addr & ~0x001E) == 0x0020)
                    goto read_low;
                break;
            case 'R':  // R:r or R:W
            read_low:
                r = s;
                addr = s->data;
                continue;
            case 'r':  // R:r
            case 'v':  // V:v
            case 'e':  // E:e
            case 'q':  // Q:q
                if (r && s->addr == r->addr + 1U) {
                    addr |= s->data << 8;
                    continue;
                }
                break;
            case 'S':  // 1:S:s
                if (r == nullptr || s->addr == r->addr + 1U) {
                    --seq;  // repeat until 's'
                    goto read_low;
                }
                if (s->addr == next || s->addr == next + 2U)
                    goto return_matched;
                break;
            default:
                break;
            }
        } else if (s->write()) {
            switch (*seq) {
            case 'W':  // W:w or w:W or R:W
                if (w == nullptr || s->addr == w->addr - 1U ||
                        (r && s->addr == r->addr)) {
                write_low:
                    w = s;
                    continue;
                }
                break;
            case 'X':  // 2:X
                if (s->addr == uint16_t(next + disp - 1))
                    goto write_low;
                break;
            case 'F':  // 2:F
                if (s->addr == 0xFF00U + (uint8_t)disp)
                    goto write_low;
                break;
            case 'w':  // w:W or W:w
            case 'x':  // X:x
            case 'f':  // F:f
                if (w == nullptr || s->addr == w->addr + 1U) {
                    w = s;
                    continue;
                }
                break;
            default:
                break;
            }
        }
        break;
    }
    LOG_MATCH(cli.print("@@   NOT MATCHED "));
    LOG_MATCH(cli.println(*seq));
    return 0;
}

}  // namespace ins8070
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
