#include "inst_i8096.h"
#include <ctype.h>
#include <string.h>
#include "debugger.h"
#include "mems_i8096.h"

namespace debugger {
namespace i8096 {

namespace {
/**
 * Legends for SEQUENCES.
# 1: instruction code (next=&1+1)
# 2: instruction operands (++next)
# 3: instruction operands (++next)
# 4: instruction operands (++next)
# 5: instruction operands (++next)
# 6: instruction operands (++next)
# 7: instruction operands (++next)
# ~: prefetch bytes (++next)
# B: read byte
# R: read low byte of word
# r: read high byte of word (=&R+1)
# Y: write byte
# W: write low byte of word
# w: write high byte of word (=&W+1)
# V: read low byte of vector
# v: read high byte of vector (=&V+1)
#--- following code invalidate prefetch bytes
# j: fetch instruction from next+8-bit disp
# k: fetch instruction from next+8-bit disp (JBC/JBS/DJNZ)
# K: fetch instruction from next+11-bit disp
# J: fetch instruction from next+16-bit disp
# i: fetch instruction from unknown address
# A: fetch instruction from previous read word
# C: fetch from reset origin (2080H)
#---- separator
# @: separator for conditional
# /: separator for 8/16 bit index
#---- interrupt
# V:v:W:w:A
 */

constexpr const char INTERRUPT[] = "VvWwA";

constexpr const char *const SEQUENCES[/*seq*/] = {
        "",                     //  0
        "12",                   //  1
        "123",                  //  2
        "12~K",                 //  3
        "12~WwK",               //  4
        "123~k@123",            //  5
        "1234",                 //  6
        "12345",                //  7
        "1234~Rr",              //  8
        "12345Rr/123456Rr",     //  9
        "1234~Rr/1234~Rr",      // 10
        "12345~Rr/123456Rr",    // 11
        "1234~B",               // 12
        "12345B/123456B",       // 13
        "12345~B/123456B",      // 14
        "123~Rr",               // 15
        "1234~Rr/12345~Rr",     // 16
        "123~B",                // 17
        "1234~B/12345~B",       // 18
        "123~Ww",               // 19
        "1234~Ww/12345~Ww",     // 20
        "123~Y",                // 21
        "1234~Y/12345~Y",       // 22
        "12~Ww",                // 23
        "12~RrWw",              // 24
        "123~RrWw/1234~RrWw",   // 25
        "12~Rr",                // 26
        "12~j@12",              // 27
        "12~i",                 // 28
        "123~J",                // 29
        "123~WwJ",              // 30
        "1~RrA",                // 31
        "1~Ww",                 // 32
        "1~Rr",                 // 33
        "1~VvWv",               // 34
        "1",                    // 35
        "1~C",                  // 36
        "123456",               // 37
        "12345~Rr",             // 38
        "123456~Rr/1234567Rr",  // 39
        "12345~B",              // 40
        "123456~B/1234567B",    // 41
        "12345~Rr/123456~Rr",   // 42
        "12345~B/123456~B",     // 43
        "1234Rr",               // 44
};

constexpr InstI8096::Table PAGE00_TABLE[] = {
        {2, 1},   // 00: SKIP  b        1:2
        {2, 1},   // 01: CLR   w        1:2
        {2, 1},   // 02: NOT   w        1:2
        {2, 1},   // 03: NEG   w        1:2
        {0, 0},   // 04
        {2, 1},   // 05: DEC   w        1:2
        {2, 1},   // 06: EXT   l        1:2
        {2, 1},   // 07: INC   w        1:2
        {3, 2},   // 08: SHR   w,c      1:2:3
        {3, 2},   // 09: SHL   w,c      1:2:3
        {3, 2},   // 0A: SHRA  w,c      1:2:3
        {0, 0},   // 0B
        {3, 2},   // 0C: SHRL  l,c      1:2:3
        {3, 2},   // 0D: SHLL  l,c      1:2:3
        {3, 2},   // 0E: SHRAL l,c      1:2:3
        {3, 2},   // 0F: NORM  l,b      1:2:3
        {0, 0},   // 10
        {2, 1},   // 11: CLRB  b        1:2
        {2, 1},   // 12: NOTB  b        1:2
        {2, 1},   // 13: NEGB  b        1:2
        {0, 0},   // 14
        {2, 1},   // 15: DECB  b        1:2
        {2, 1},   // 16: EXTB  w        1:2
        {2, 1},   // 17: INCB  b        1:2
        {3, 2},   // 18: SHRB  b,c      1:2:3
        {3, 2},   // 19: SHLB  b,c      1:2:3
        {3, 2},   // 1A: SHRAB b,c      1:2:3
        {0, 0},   // 1B
        {0, 0},   // 1C
        {0, 0},   // 1D
        {0, 0},   // 1E
        {0, 0},   // 1F
        {2, 3},   // 20: SJMP  r        1:2:~:K
        {2, 3},   // 21: SJMP  r        1:2:~:K
        {2, 3},   // 22: SJMP  r        1:2:~:K
        {2, 3},   // 23: SJMP  r        1:2:~:K
        {2, 3},   // 24: SJMP  r        1:2:~:K
        {2, 3},   // 25: SJMP  r        1:2:~:K
        {2, 3},   // 26: SJMP  r        1:2:~:K
        {2, 3},   // 27: SJMP  r        1:2:~:K
        {2, 4},   // 28: SCALL r        1:2:~:W:w:K
        {2, 4},   // 29: SCALL r        1:2:~:W:w:K
        {2, 4},   // 2A: SCALL r        1:2:~:W:w:K
        {2, 4},   // 2B: SCALL r        1:2:~:W:w:K
        {2, 4},   // 2C: SCALL r        1:2:~:W:w:K
        {2, 4},   // 2D: SCALL r        1:2:~:W:w:K
        {2, 4},   // 2E: SCALL r        1:2:~:W:w:K
        {2, 4},   // 2F: SCALL r        1:2:~:W:w:K
        {3, 5},   // 30: JBC   0,r      1:2:3:~:k@1:2:3
        {3, 5},   // 31: JBC   1,r      1:2:3:~:k@1:2:3
        {3, 5},   // 32: JBC   2,r      1:2:3:~:k@1:2:3
        {3, 5},   // 33: JBC   3,r      1:2:3:~:k@1:2:3
        {3, 5},   // 34: JBC   4,r      1:2:3:~:k@1:2:3
        {3, 5},   // 35: JBC   5,r      1:2:3:~:k@1:2:3
        {3, 5},   // 36: JBC   6,r      1:2:3:~:k@1:2:3
        {3, 5},   // 37: JBC   7,r      1:2:3:~:k@1:2:3
        {3, 5},   // 38: JBS   0,r      1:2:3:~:k@1:2:3
        {3, 5},   // 39: JBS   1,r      1:2:3:~:k@1:2:3
        {3, 5},   // 3A: JBS   2,r      1:2:3:~:k@1:2:3
        {3, 5},   // 3B: JBS   3,r      1:2:3:~:k@1:2:3
        {3, 5},   // 3C: JBS   4,r      1:2:3:~:k@1:2:3
        {3, 5},   // 3D: JBS   5,r      1:2:3:~:k@1:2:3
        {3, 5},   // 3E: JBS   6,r      1:2:3:~:k@1:2:3
        {3, 5},   // 3F: JBS   7,r      1:2:3:~:k@1:2:3
        {4, 6},   // 40: AND   w,w,w    1:2:3:4
        {5, 7},   // 41: AND   w,w,#    1:2:3:4:5
        {4, 8},   // 42: AND   w,w,[w]  1:2:3:4:~:R:r
        {5, 9},   // 43: AND   w,w,n[w] 1:2:3:4:5:R:r/1:2:3:4:5:6:R:r
        {4, 6},   // 44: ADD   w,w,w    1:2:3:4
        {5, 7},   // 45: ADD   w,w,#    1:2:3:4:5
        {4, 8},   // 46: ADD   w,w,[w]  1:2:3:4:~:R:r
        {5, 9},   // 47: ADD   w,w,n[w] 1:2:3:4:5:R:r/1:2:3:4:5:6:R:r
        {4, 6},   // 48: SUB   w,w,w    1:2:3:4
        {5, 7},   // 49: SUB   w,w,#    1:2:3:4:5
        {4, 8},   // 4A: SUB   w,w,[w]  1:2:3:4:~:R:r
        {5, 9},   // 4B: SUB   w,w,n[w] 1:2:3:4:5:R:r/1:2:3:4:5:6:R:r
        {4, 6},   // 4C: MULU  w,w,w    1:2:3:4
        {5, 7},   // 4D: MULU  w,w,#    1:2:3:4:5
        {4, 10},  // 4E: MULU  w,w,[w]  1:2:3:4:~:R:r/1:2:3:4:~:R:r
        {5, 11},  // 4F: MULU  w,w,n[w] 1:2:3:4:5:~:R:r/1:2:3:4:5:6:R:r
        {4, 6},   // 50: ANDB  b,b,b    1:2:3:4
        {4, 6},   // 51: ANDB  b,b,#    1:2:3:4
        {4, 12},  // 52: ANDB  b,b,[w]  1:2:3:4:~:B
        {5, 13},  // 53: ANDB  b,b,n[w] 1:2:3:4:5:B/1:2:3:4:5:6:B
        {4, 6},   // 54: ADDB  b,b,b    1:2:3:4
        {4, 6},   // 55: ADDB  b,b,#    1:2:3:4
        {4, 12},  // 56: ADDB  b,b,[w]  1:2:3:4:~:B
        {5, 13},  // 57: ADDB  b,b,n[w] 1:2:3:4:5:B/1:2:3:4:5:6:B
        {4, 6},   // 58: SUBB  b,b,b    1:2:3:4
        {4, 6},   // 59: SUBB  b,b,#    1:2:3:4
        {4, 12},  // 5A: SUBB  b,b,[w]  1:2:3:4:~:B
        {5, 13},  // 5B: SUBB  b,b,n[w] 1:2:3:4:5:B/1:2:3:4:5:6:B
        {4, 6},   // 5C: MULUB w,b,b    1:2:3:4
        {4, 6},   // 5D: MULUB w,b,#    1:2:3:4
        {4, 12},  // 5E: MULUB w,b,[w]  1:2:3:4:~:B
        {5, 14},  // 5F: MULUB w,b,n[w] 1:2:3:4:5:~:B/1:2:3:4:5:6:B
        {3, 2},   // 60: AND   w,w      1:2:3
        {4, 6},   // 61: AND   w,#      1:2:3:4
        {3, 15},  // 62: AND   w,[w]    1:2:3:~:R:r
        {4, 16},  // 63: AND   w,n[w]   1:2:3:4:~:R:r/1:2:3:4:5:~:R:r
        {3, 2},   // 64: ADD   w,w      1:2:3
        {4, 6},   // 65: ADD   w,#      1:2:3:4
        {3, 15},  // 66: ADD   w,[w]    1:2:3:~:R:r
        {4, 16},  // 67: ADD   w,n[w]   1:2:3:4:~:R:r/1:2:3:4:5:~:R:r
        {3, 2},   // 68: SUB   w,w      1:2:3
        {4, 6},   // 69: SUB   w,#      1:2:3:4
        {3, 15},  // 6A: SUB   w,[w]    1:2:3:~:R:r
        {4, 16},  // 6B: SUB   w,n[w]   1:2:3:4:~:R:r/1:2:3:4:5:~:R:r
        {3, 2},   // 6C: MULU  l,w      1:2:3
        {4, 6},   // 6D: MULU  l,#      1:2:3:4
        {3, 15},  // 6E: MULU  l,[w]    1:2:3:~:R:r
        {4, 16},  // 6F: MULU  l,n[w]   1:2:3:4:~:R:r/1:2:3:4:5:~:R:r
        {3, 2},   // 70: ANDB  b,b      1:2:3
        {3, 2},   // 71: ANDB  b,#      1:2:3
        {3, 17},  // 72: ANDB  b,[w]    1:2:3:~:B
        {4, 18},  // 73: ANDB  b,n[w]   1:2:3:4:~:B/1:2:3:4:5:~:B
        {3, 2},   // 74: ADDB  b,b      1:2:3
        {3, 2},   // 75: ADDB  b,#      1:2:3
        {3, 17},  // 76: ADDB  b,[w]    1:2:3:~:B
        {4, 18},  // 77: ADDB  b,n[w]   1:2:3:4:~:B/1:2:3:4:5:~:B
        {3, 2},   // 78: SUBB  b,b      1:2:3
        {3, 2},   // 79: SUBB  b,#      1:2:3
        {3, 17},  // 7A: SUBB  b,[w]    1:2:3:~:B
        {4, 18},  // 7B: SUBB  b,n[w]   1:2:3:4:~:B/1:2:3:4:5:~:B
        {3, 2},   // 7C: MULUB w,b      1:2:3
        {3, 2},   // 7D: MULUB w,#      1:2:3
        {3, 17},  // 7E: MULUB w,[w]    1:2:3:~:B
        {4, 18},  // 7F: MULUB w,n[w]   1:2:3:4:~:B/1:2:3:4:5:~:B
        {3, 2},   // 80: OR    w,w      1:2:3
        {4, 6},   // 81: OR    w,#      1:2:3:4
        {3, 15},  // 82: OR    w,[w]    1:2:3:~:R:r
        {4, 16},  // 83: OR    w,n[w]   1:2:3:4:~:R:r/1:2:3:4:5:~:R:r
        {3, 2},   // 84: XOR   w,w      1:2:3
        {4, 6},   // 85: XOR   w,#      1:2:3:4
        {3, 15},  // 86: XOR   w,[w]    1:2:3:~:R:r
        {4, 16},  // 87: XOR   w,n[w]   1:2:3:4:~:R:r/1:2:3:4:5:~:R:r
        {3, 2},   // 88: CMP   w,w      1:2:3
        {4, 6},   // 89: CMP   w,#      1:2:3:4
        {3, 15},  // 8A: CMP   w,[w]    1:2:3:~:R:r
        {4, 16},  // 8B: CMP   w,n[w]   1:2:3:4:~:R:r/1:2:3:4:5:~:R:r
        {3, 2},   // 8C: DIV   l,w      1:2:3
        {4, 6},   // 8D: DIV   l,#      1:2:3:4
        {3, 15},  // 8E: DIV   l,[w]    1:2:3:~:R:r
        {4, 16},  // 8F: DIV   l,n[w]   1:2:3:4:~:R:r/1:2:3:4:5:~:R:r
        {3, 2},   // 90: ORB   b,b      1:2:3
        {3, 2},   // 91: ORB   b,#      1:2:3
        {3, 17},  // 92: ORB   b,[w]    1:2:3:~:B
        {4, 18},  // 93: ORB   b,n[w]   1:2:3:4:~:B/1:2:3:4:5:~:B
        {3, 2},   // 94: XORB  b,b      1:2:3
        {3, 2},   // 95: XORB  b,#      1:2:3
        {3, 17},  // 96: XORB  b,[w]    1:2:3:~:B
        {4, 18},  // 97: XORB  b,n[w]   1:2:3:4:~:B/1:2:3:4:5:~:B
        {3, 2},   // 98: CMPB  b,b      1:2:3
        {3, 2},   // 99: CMPB  b,#      1:2:3
        {3, 17},  // 9A: CMPB  b,[w]    1:2:3:~:B
        {4, 18},  // 9B: CMPB  b,n[w]   1:2:3:4:~:B/1:2:3:4:5:~:B
        {3, 2},   // 9C: DIVB  w,b      1:2:3
        {3, 2},   // 9D: DIVB  w,#      1:2:3
        {3, 17},  // 9E: DIVB  w,[w]    1:2:3:~:B
        {4, 18},  // 9F: DIVB  w,n[w]   1:2:3:4:~:B/1:2:3:4:5:~:B
        {3, 2},   // A0: LD    w,w      1:2:3
        {4, 6},   // A1: LD    w,#      1:2:3:4
        {3, 15},  // A2: LD    w,[w]    1:2:3:~:R:r
        {4, 16},  // A3: LD    w,n[w]   1:2:3:4:~:R:r/1:2:3:4:5:~:R:r
        {3, 2},   // A4: ADDC  w,w      1:2:3
        {4, 6},   // A5: ADDC  w,#      1:2:3:4
        {3, 15},  // A6: ADDC  w,[w]    1:2:3:~:R:r
        {4, 16},  // A7: ADDC  w,n[w]   1:2:3:4:~:R:r/1:2:3:4:5:~:R:r
        {3, 2},   // A8: SUBC  w,w      1:2:3
        {4, 6},   // A9: SUBC  w,#      1:2:3:4
        {3, 15},  // AA: SUBC  w,[w]    1:2:3:~:R:r
        {4, 16},  // AB: SUBC  w,n[w]   1:2:3:4:~:R:r/1:2:3:4:5:~:R:r
        {3, 2},   // AC: LDBZE w,b      1:2:3
        {3, 6},   // AD: LDBZE w,#      1:2:3:4
        {3, 17},  // AE: LDBZE w,[w]    1:2:3:~:B
        {4, 18},  // AF: LDBZE w,n[w]   1:2:3:4:~:B/1:2:3:4:5:~:B
        {3, 2},   // B0: LDB   b,b      1:2:3
        {3, 2},   // B1: LDB   b,#      1:2:3
        {3, 17},  // B2: LDB   b,[w]    1:2:3:~:B
        {4, 18},  // B3: LDB   b,n[w]   1:2:3:4:~:B/1:2:3:4:5:~:B
        {3, 2},   // B4: ADDCB b,b      1:2:3
        {3, 2},   // B5: ADDCB b,#      1:2:3
        {3, 17},  // B6: ADDCB b,[w]    1:2:3:~:B
        {4, 18},  // B7: ADDCB b,n[w]   1:2:3:4:~:B/1:2:3:4:5:~:B
        {3, 2},   // B8: SUBCB b,b      1:2:3
        {3, 2},   // B9: SUBCB b,#      1:2:3
        {3, 17},  // BA: SUBCB b,[w]    1:2:3:~:B
        {4, 18},  // BB: SUBCB b,n[w]   1:2:3:4:~:B/1:2:3:4:5:~:B
        {3, 2},   // BC: LDBSE w,b      1:2:3
        {3, 6},   // BD: LDBSE w,#      1:2:3:4
        {3, 17},  // BE: LDBSE w,[w]    1:2:3:~:B
        {4, 18},  // BF: LDBSE w,n[w]   1:2:3:4:~:B/1:2:3:4:5:~:B
        {3, 2},   // C0: ST    w,w      1:2:3
        {0, 0},   // C1
        {3, 19},  // C2: ST    w,[w]    1:2:3:~:W:w
        {4, 20},  // C3: ST    w,n[w]   1:2:3:4:~:W:w/1:2:3:4:5:~:W:w
        {3, 2},   // C4: STB   b,b      1:2:3
        {0, 0},   // C5
        {3, 21},  // C6: STB   b,[w]    1:2:3:~:Y
        {4, 22},  // C7: STB   b,n[w]   1:2:3:4:~:Y/1:2:3:4:5:~:Y
        {2, 23},  // C8: PUSH  w        1:2:~:W:w
        {3, 19},  // C9: PUSH  #        1:2:3:~:W:w
        {2, 24},  // CA: PUSH  [w]      1:2:~:R:r:W:w
        {3, 25},  // CB: PUSH  n[w]     1:2:3:~:R:r:W:w/1:2:3:4:~:R:r:W:w
        {2, 26},  // CC: POP   w        1:2:~:R:r
        {0, 0},   // CD
        {2, 24},  // CE: POP   [w]      1:2:~:R:r:W:w
        {3, 25},  // CF: POP   n[w]     1:2:3:~:R:r:W:w/1:2:3:4:~:R:r:W:w
        {2, 27},  // D0: JNST  r        1:2:~:j@1:2
        {2, 27},  // D1: JNH   r        1:2:~:j@1:2
        {2, 27},  // D2: JGT   r        1:2:~:j@1:2
        {2, 27},  // D3: JNC   r        1:2:~:j@1:2
        {2, 27},  // D4: JNVT  r        1:2:~:j@1:2
        {2, 27},  // D5: JNV   r        1:2:~:j@1:2
        {2, 27},  // D6: JGE   r        1:2:~:j@1:2
        {2, 27},  // D7: JNE   r        1:2:~:j@1:2
        {2, 27},  // D8: JST   r        1:2:~:j@1:2
        {2, 27},  // D9: JH    r        1:2:~:j@1:2
        {2, 27},  // DA: JLE   r        1:2:~:j@1:2
        {2, 27},  // DB: JC    r        1:2:~:j@1:2
        {2, 27},  // DC: JVT   r        1:2:~:j@1:2
        {2, 27},  // DD: JV    r        1:2:~:j@1:2
        {2, 27},  // DE: JLT   r        1:2:~:j@1:2
        {2, 27},  // DF: JE    r        1:2:~:j@1:2
        {3, 5},   // E0: DJNZ  w,r      1:2:3:~:k@1:2:3
        {0, 0},   // E1
        {0, 0},   // E2
        {2, 28},  // E3: BR    [w]      1:2:~:i
        {0, 0},   // E4
        {0, 0},   // E5
        {0, 0},   // E6
        {3, 29},  // E7: LJMP  rr       1:2:3:~:J
        {0, 0},   // E8
        {0, 0},   // E9
        {0, 0},   // EA
        {0, 0},   // EB
        {0, 0},   // EC
        {0, 0},   // ED
        {0, 0},   // EE
        {3, 30},  // EF: LCALL rr       1:2:3:~:W:w:J
        {1, 31},  // F0: RET   -        1:~:R:r:A
        {0, 0},   // F1
        {1, 32},  // F2: PUSHF -        1:~:W:w
        {1, 33},  // F3: POPF  -        1:~:R:r
        {0, 0},   // F4
        {0, 0},   // F5
        {0, 0},   // F6
        {1, 34},  // F7: TRAP  -        1:~:V:v:W:v
        {1, 35},  // F8: CLRC  -        1
        {1, 35},  // F9: SETC  -        1
        {1, 35},  // FA: DI    -        1
        {1, 35},  // FB: EI    -        1
        {1, 35},  // FC: CLRVT -        1
        {1, 35},  // FD: NOP   -        1
        {0, 0},   // FE
        {1, 36},  // FF: RST   -        1:~:C
};

constexpr InstI8096::Table PAGEFE_TABLE[] = {
        {5, 7},   // 4C: MUL   l,w,w    1:2:3:4:5
        {6, 37},  // 4D: MUL   l,w,#    1:2:3:4:5:6
        {5, 38},  // 4E: MUL   l,w,[w]  1:2:3:4:5:~:R:r
        {6, 39},  // 4F: MUL   l,w,n[w] 1:2:3:4:5:6:~:R:r/1:2:3:4:5:6:7:R:r
        {5, 7},   // 5C: MULB  w,b,b    1:2:3:4:5
        {5, 7},   // 5D: MULB  w,b,#    1:2:3:4:5
        {5, 40},  // 5E: MULB  w,b,[w]  1:2:3:4:5:~:B
        {6, 41},  // 5F: MULB  w,b,n[w] 1:2:3:4:5:6:~:B/1:2:3:4:5:6:7:B
        {4, 6},   // 6C: MUL   l,w      1:2:3:4
        {5, 7},   // 6D: MUL   l,#      1:2:3:4:5
        {4, 8},   // 6E: MUL   l,[w]    1:2:3:4:~:R:r
        {5, 42},  // 6F: MUL   l,n[w]   1:2:3:4:5:~:R:r/1:2:3:4:5:6:~:R:r
        {4, 6},   // 7C: MULB  w,b      1:2:3:4
        {4, 6},   // 7D: MULB  w,#      1:2:3:4
        {4, 12},  // 7E: MULB  w,[w]    1:2:3:4:~:B
        {5, 43},  // 7F: MULB  w,n[w]   1:2:3:4:5:~:B/1:2:3:4:5:6:~:B
        {4, 6},   // 8C: DIV   l,w      1:2:3:4
        {5, 7},   // 8D: DIV   l,#      1:2:3:4:5
        {4, 44},  // 8E: DIV   l,[w]    1:2:3:4:R:r
        {5, 42},  // 8F: DIV   l,n[w]   1:2:3:4:5:~:R:r/1:2:3:4:5:6:~:R:r
        {4, 6},   // 9C: DIVB  w,b      1:2:3:4
        {4, 6},   // 9D: DIVB  w,#      1:2:3:4
        {4, 12},  // 9E: DIVB  w,[w]    1:2:3:4:~:B
        {5, 43},  // 9F: DIVB  w,n[w]   1:2:3:4:5:~:B/1:2:3:4:5:6:~:B
};

bool isVector(uint16_t addr) {
    return addr >= 0x2000 && addr < 0x2012;
}

int16_t signExtend(uint16_t u16, uint_fast8_t width) {
    const auto sign = UINT16_C(1) << (width - 1);
    const auto mask = (sign << 1) - 1;
    return static_cast<int16_t>(((u16 + sign) & mask) - sign);
}

const InstI8096::Table *page_FE(uint_fast8_t opc) {
    const auto hi = opc >> 4;
    const auto lo = (opc >> 2) & 3;
    if (hi < 0x4 || hi >= 0xA || lo != 3)
        return nullptr;
    const auto index = ((hi - 0x4) << 2) + (opc & 3);
    return &PAGEFE_TABLE[index];
}

struct StrBuffer {
    StrBuffer() : _out(0) {}
    operator const char *() const { return _buffer; }
    auto out() { return _out; }
    void reset(uint8_t out) { _out = out; }
    const char *append(const char *p) {
        for (; *p && *p != '@' && *p != '/'; ++p) {
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

bool InstI8096::indexAddressing(uint_fast8_t opc) {
    return opc >= 0x40 && opc < 0xD0 && (opc & 3) == 3;
}

const InstI8096::Table *InstI8096::get(uint16_t pc, MemsI8096 *mems) {
    _opc = mems->read(pc++);
    auto table = &PAGE00_TABLE[_opc];
    if (_opc == 0xFE) {
        _opc = mems->read(pc++);
        table = page_FE(_opc);
    }
    _len = table ? table->len : 0;
    _indexed = indexAddressing(_opc) && (mems->read(pc) & 1);
    return table;
}

bool InstI8096::set(uint16_t pc, MemsI8096 *mems) {
    get(pc, mems);
    return _len != 0;
}

uint_fast8_t InstI8096::instLength() const {
    return _indexed ? _len + 1 : _len;
}

bool InstI8096::match(
        SignalsI8096 *begin, const SignalsI8096 *end, MemsI8096 *mems) {
    _insufficient = begin->diff(end) < 2;
    if (!begin->read() || _insufficient)
        return false;
    const auto table = get(begin->addr, mems);
    if (_len == 0)
        return false;

    auto seq = SEQUENCES[table->seq];
    if (_indexed)
        seq = strchr(seq, '/') + 1;

    StrBuffer sequence;
    const auto out = sequence.out();
    while (*seq) {
        sequence.reset(out);
        seq = sequence.append(seq);
        if (matchSequence(begin, end, sequence))
            return true;
        if (*seq == '@')
            ++seq;
    }
    return false;
}

bool InstI8096::matchInterrupt(
        SignalsI8096 *begin, const SignalsI8096 *end) const {
    const auto size = begin->diff(end);
    const char *seq = INTERRUPT;
    LOG_MATCH(cli.print("@@ match interrupt: "));
    LOG_MATCH(cli.print(" size="));
    LOG_MATCH(cli.printlnDec(size));
    uint_fast8_t i = 0;
    uint16_t addr = 0;
    const SignalsI8096 *r = nullptr;
    const SignalsI8096 *w = nullptr;
    while (*seq && i < size) {
        const auto s = begin->next(i);
        LOG_MATCH(cli.print("         "));
        LOG_MATCH(cli.print(*seq));
        LOG_MATCH(cli.print(" s="));
        LOG_MATCH(s->print());
        if (s->isOperand()) {
            i++;
            continue;
        }
        if (s->write()) {
            switch (*seq) {
            case 'W':
                w = s;
                break;
            case 'w':
                if (s->addr != w->addr + 1)
                    goto not_matched;
                break;
            default:
                goto not_matched;
            }
            s->markOperand();
        } else {
            switch (*seq) {
            case 'V':
                if (!isVector(s->addr))
                    goto not_matched;
                addr = s->data;
                r = s;
                s->markOperand();
                break;
            case 'v':
                if (s->addr != r->addr + 1)
                    goto not_matched;
                addr |= (s->data << 8);
                s->markOperand();
                break;
            case 'A':
                if (s->addr != addr)
                    goto not_matched;
                break;
            default:
                goto not_matched;
            }
        }
        seq++;
        i++;
    }
    if (*seq == 0) {
        LOG_MATCH(cli.println("@@   MATCHED INTERRUPT"));
        return true;
    } else {
    not_matched:
        for (uint_fast8_t j = 0; j < i; j++)
            begin->next(i)->clearMark();
        LOG_MATCH(cli.println("@@   NOT MATCHED INTERRUPT"));
        return false;
    }
}

bool InstI8096::matchSequence(
        SignalsI8096 *begin, const SignalsI8096 *end, const char *seq) {
    const auto size = begin->diff(end);
    const auto max = strlen(seq) + 4;
    LOG_MATCH(cli.print("@@   match: seq="));
    LOG_MATCH(cli.print(seq));
    LOG_MATCH(cli.print(" size="));
    LOG_MATCH(cli.printlnDec(size));

    uint_fast8_t opc = 0;
    uint_fast8_t disp2 = 0;
    uint_fast8_t disp3 = 0;
    uint16_t next = 0;
    uint_fast8_t i = 0;
    _nexti = 0;
    while (isdigit(*seq) && i < size && i < max) {
        const auto s = begin->next(i++);
        LOG_MATCH(cli.print("         "));
        LOG_MATCH(cli.print(*seq));
        LOG_MATCH(cli.print(" s="));
        LOG_MATCH(s->print());
        if (s->isOperand())
            continue;
        if (isVector(s->addr) && matchInterrupt(s, end)) {
            _nexti = i + 4;
            LOG_MATCH(cli.print("@@   MATCHED: nexti="));
            LOG_MATCH(cli.printlnDec(_nexti, 2));
            return true;
        }
        if (s->read()) {
            if (*seq == '1') {
                s->markFetch();
                // may be part of 11-bit branch displacement
                opc = s->data;
                next = s->addr + 1;
                seq++;
                _nexti = i;
            } else if (s->addr == next) {
                if (*seq == '2') {
                    disp2 = s->data;
                } else if (*seq == '3') {
                    disp3 = s->data;
                }
                next++;
                seq++;
                _nexti = i;
            }
        }
    }
    if (isdigit(*seq)) {
    not_matched:
        _insufficient = i >= size;
        for (uint_fast8_t j = 0; j < i; j++)
            begin->next(i)->clearMark();
        LOG_MATCH(cli.print("@@   NOT MATCHED "));
        LOG_MATCH(cli.println(*seq));
        return false;
    }

    uint16_t prefetch = next;
    uint16_t addr = 0;
    int16_t disp = 0;
    const SignalsI8096 *r = nullptr;
    const SignalsI8096 *w = nullptr;
    while (*seq) {
        if (i >= size)
            goto not_matched;
        const auto s = begin->next(i);
        if (*seq == '~') {
            if (s->isOperand()) {
                i++;
                continue;
            }
            if (s->read() && s->addr == prefetch) {
                LOG_MATCH(cli.print("         "));
                LOG_MATCH(cli.print(*seq));
                LOG_MATCH(cli.print(" s="));
                LOG_MATCH(s->print());
                if (prefetch - next >= 4)
                    goto not_matched;
                prefetch++;
                i++;
                continue;
            }
            seq++;
        }
        LOG_MATCH(cli.print("         "));
        LOG_MATCH(cli.print(*seq));
        LOG_MATCH(cli.print(" s="));
        LOG_MATCH(s->print());
        if (s->isOperand()) {
            i++;
            continue;
        }
        if (isVector(s->addr) && matchInterrupt(s, end)) {
            _nexti += i;
            for (uint_fast8_t j = 0; j < i; j++)
                begin->next(j)->clearMark();
            return true;
        }
        if (s->write()) {
            switch (*seq) {
            case 'Y':
            case 'W':
                w = s;
                break;
            case 'w':
                if (s->addr != w->addr + 1)
                    goto not_matched;
                break;
            default:
                goto not_matched;
            }
            s->markOperand();
        } else {
            switch (*seq) {
            case 'V':
                if (!isVector(s->addr))
                    goto not_matched;
                // Fall-through
            case 'B':
            case 'R':
                r = s;
                addr = s->data;  // low 8-bit
                s->markOperand();
                break;
            case 'v':
            case 'r':
                if (s->addr != r->addr + 1)
                    goto not_matched;
                addr |= (s->data << 8);  // high 8-bit
                s->markOperand();
                break;
            case 'j':
                disp = static_cast<int8_t>(disp2);
                goto branch;
            case 'k':
                disp = static_cast<int8_t>(disp3);
                goto branch;
            case 'K':
                disp = signExtend(((opc & 7) << 8) + disp2, 11);
                goto branch;
            case 'J':
                disp = static_cast<int16_t>((disp3 << 8) | disp2);
            branch:
                LOG_MATCH(cli.print("     disp="));
                LOG_MATCH(cli.printHex(disp, 4));
                LOG_MATCH(cli.print(" next="));
                LOG_MATCH(cli.printHex(next, 4));
                LOG_MATCH(cli.print(" target="));
                LOG_MATCH(cli.printlnHex((next + disp) & UINT16_MAX, 4));
                if (s->addr == static_cast<uint16_t>(next + disp)) {
                match_branch:
                    _nexti = i;
                    goto match;
                }
                goto not_matched;
            case 'A':
                if (s->addr == addr)
                    goto match_branch;
                goto not_matched;
            case 'C':
                if (s->addr == 0x2080)
                    goto match_branch;
                goto not_matched;
            case 'i':
                if (s->addr != prefetch)
                    goto match_branch;
                goto not_matched;
            default:
                goto not_matched;
            }
        }
        seq++;
        i++;
    }
match:
    LOG_MATCH(cli.print("@@   MATCHED: nexti="));
    LOG_MATCH(cli.printlnDec(_nexti, 2));
    return true;
}

}  // namespace i8096
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
