#include "inst_mc6800.h"
#include "debugger.h"
#include "mems_mc6800.h"

/**
 * from "mc6809/inst_hd6309.awk"
 *
# 1: instruction code (next=&1)
# 2: instruction operands (disp, high(addr), ++next)
# 3: instruction operands (low(addr), ++next)
# 4: instruction operands (low(addr), ++next)
# 5: instruction operands (low(addr), ++next)
# Y: variable instruction operands cycles (1~9) based on post byte
# p: prefix code
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
# P: variable pull (0~12) based on post byte (PSHx)
# Q: variable push (0~12) based on post byte (PULx)
# T: repeated reads and writes (R/R+/R-:x:W/W+/W-, TFM)
# N: next instruction read from |next|
# J: next instruction read from |addr|
# j: next instruction read from |next| or |next|+disp
# k: next instruction read from |next| or |next|+disp16
# i: next instruction read from unknown
# V: read 1 byte from address FFF8-FFFE (high(addr))
# v: read 1 byte from address V+1 (low(addr))
# X: any valid read
# x: irrelevant read access to $FFFF
# Z: repeated irrelevant read access to $FFFF
# n: non VMA access
# @: separator for alternative sequence
*/

namespace debugger {
namespace mc6800 {

const char *InstMc6800::opcSequence(uint16_t addr) const {
    return instSequence(_mems->raw_read(addr));
}

bool InstMc6800::match(
        const Signals *begin, const Signals *end, const Signals *prefetch) {
    const auto fetch = prefetch ? prefetch : begin;
    if (!fetch->read())
        return false;
    return matchSequence(begin, end, opcSequence(fetch->addr), prefetch);
}

bool InstMc6800::matchInterrupt(const Signals *begin, const Signals *end) {
    return matchSequence(begin, end, intrSequence(), nullptr);
}

namespace {
bool adjacent(const Signals *s, const Signals *base) {
    if (base == nullptr)
        return true;
    return s->addr >= base->addr - 1U && s->addr < base->addr + 2U;
}
}  // namespace

bool InstMc6800::matchSequence(const Signals *begin, const Signals *end,
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
    int16_t disp;
    for (auto i = 0; i < size; ++i) {
        const auto s = begin->next(i);
        LOG_MATCH(cli.print("         "));
        LOG_MATCH(cli.print(*seq));
        LOG_MATCH(cli.print(" s="));
        LOG_MATCH(s->print());
        if (!s->valid()) {
            if (*seq != 'n')
                goto not_matched;
        } else if (s->read()) {
            switch (*seq) {
            case '0':
                if (prefetch)
                    --i;
                /* Fall-through */
            case '1':
                next = (prefetch ? prefetch : begin)->addr + 1;
                break;
            case '2':  // 1:2
                if (s->addr == next) {
                    addr = s->data;
                    ++next;
                    break;
                }
                goto not_matched;
            case '3':  // 2:3
            case '4':  // 3:4
            case '5':  // 4:5
                if (s->addr == next) {
                    addr <<= 8;
                    addr |= s->data;
                    ++next;
                    break;
                }
                goto not_matched;
            case 'j':  // N or next+(8bit disp)
                disp = static_cast<int8_t>(addr);
                goto branch;
            case 'k':  // N or next+(16bit disp)
                disp = static_cast<int16_t>(addr);
            branch:
                if (s->addr == static_cast<uint16_t>(next + disp)) {
                    _nexti = i;
                    break;
                }
                /* Fall-through */
            case 'N':  // 1:N or 1:2:N or 1:2:3:N
                if (s->addr == next) {
                    _nexti = i;
                    break;
                }
                goto not_matched;
            case 'A':  // 2:3:A
            case 'D':  // 2:D
                if (s->addr == addr)
                    break;
                goto not_matched;
            case 'J':  // 2:3:J, R:r:J or V:v:J
                if (s->addr == addr) {
                    _nexti = i;
                    break;
                }
                goto not_matched;
            case 'a':  // A:a
            case 'd':  // D:d
                if (s->addr == addr + 1U)
                    break;
                goto not_matched;
            case 'V':  // W:W:V
                if (s->addr >= vectorBase()) {
                    addr = s->data;
                    r = s;
                    break;
                }
                goto not_matched;
            case 'i':
                _nexti = i;
                break;
            case 'R':  // R:r or R:W
                r = s;
                addr = s->data;
                break;
            case 'r':  // R:r
            case 'v':  // V:v
                if (r && s->addr == r->addr + 1U) {
                    addr <<= 8;
                    addr |= s->data;
                    r = s;
                    break;
                }
                goto not_matched;
            case 'X':  // any read
                break;
            case 'x':  // read $FFFF
                if (s->addr == 0xFFFF)
                    break;
                goto not_matched;
            case 'T':  // HD6309 TFM
                if (i + 2 < size && adjacent(s, r)) {
                    const auto x = s->next(1);
                    const auto y = s->next(2);
                    if (x->read() && x->addr == 0xFFFF && y->write() &&
                            adjacent(y, w)) {
                        r = s;
                        w = y;
                        i += 2;  // skip x:y
                        --seq;   // match again with 'T'
                        break;
                    }
                }
                --i;  // match again with next sequence
                break;
            case 'Z':
                if (s->addr == 0xFFFF) {
                    --seq;  // matcha again with 'Z'
                } else {
                    --i;  // match again with next sequence
                }
                break;
            default:
                goto not_matched;
            }
        } else {
            switch (*seq) {
            case 'W':  // W:w or w:W or R:W
                if (w == nullptr || s->addr == w->addr - 1U ||
                        (r && s->addr == r->addr)) {
                    w = s;
                    break;
                }
                goto not_matched;
            case 'w':  // W:w or w:W
                if (w == nullptr || s->addr == w->addr + 1U) {
                    w = s;
                    break;
                }
                goto not_matched;
            case 'B':  // 2:3:B or A:B
            case 'E':  // 2:E or D:E
                if (s->addr == addr)
                    break;
                goto not_matched;
            case 'b':  // B:b
            case 'e':  // E:e
                if (s->addr == addr + 1U)
                    break;
                goto not_matched;
            default:
                goto not_matched;
            }
        }
        if (*++seq == 0) {
            _matched = (_nexti == i) ? i : i + 1;
            LOG_MATCH(cli.print("@@   MATCHED="));
            LOG_MATCH(cli.printlnDec(i));
            if (_nexti >= 0) {
                LOG_MATCH(cli.print("       nexti="));
                LOG_MATCH(begin->next(_nexti)->print());
            }
            return true;
        }
    }
not_matched:
    LOG_MATCH(cli.print("@@   NOT MATCHED "));
    LOG_MATCH(cli.println(*seq));
    return false;
}

}  // namespace mc6800
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
