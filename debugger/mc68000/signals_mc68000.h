#ifndef __SIGNALS_MC68000_H__
#define __SIGNALS_MC68000_H__

#include "signals.h"

namespace debugger {
namespace mc68000 {

struct Signals final : SignalsBase<Signals> {
    // cntl() bits: P40-P43 and P46 as one GPIO9 read gives them. P42 is
    // #UDS on a 16-bit bus and an output, #IPL2/#IPL0, on the MC68008.
    enum : uint8_t {
        AS = 0x01,    // P40
        RW = 0x02,    // P41
        UDS = 0x04,   // P42
        LDS = 0x08,   // P43; #DS on the MC68008
        HALT = 0x10,  // P46
        NONE = 0x1F,
    };

    bool strobe() const { return (cntl() & AS) == 0; }
    bool read() const { return (cntl() & RW) != 0; }
    bool write() const { return !read(); }
    // FC2:FC0: 1/5 user/supervisor data, 2/6 user/supervisor program, 7
    // interrupt acknowledge (M68000UM Table 3-1).
    uint8_t fc() const { return space() & 7; }
    bool program() const { return (fc() & 3) == 2; }
    bool dataSpace() const { return (fc() & 3) == 1; }
    bool iack() const { return fc() == 7; }
    // The level an acknowledge answers, on A3-A1.
    uint8_t level() const { return (addr >> 1) & 7; }
    // Both data strobes: a word on D15-D0. Never on an 8-bit bus.
    bool wordAccess() const { return (space() & WORD) != 0; }
    void setFc(uint8_t fc, bool word) { space() = (fc & 7) | (word ? WORD : 0); }

    // No bus cycle for a long time: STOP, or halted on a double bus fault.
    void markHalt() { cntl() = NONE, space() = 0, mark() = HALTED; }
    bool halt() const { return mark() == HALTED; }
    void clearMark() { mark() = 0; }
    bool fetch() const { return false; }

    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }

    void print() const;

private:
    enum : uint8_t { WORD = 0x80 };
    enum : uint8_t { HALTED = 1 };
    // FC2:FC0 in bits 0-2, WORD in bit 7.
    uint8_t space() const { return _signals[1]; }
    uint8_t &space() { return _signals[1]; }
    uint8_t mark() const { return _signals[2]; }
    uint8_t &mark() { return _signals[2]; }
};

}  // namespace mc68000
}  // namespace debugger
#endif /* __SIGNALS_MC68000_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
