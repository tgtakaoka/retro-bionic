#ifndef __SIGNALS_MC68HC16_H__
#define __SIGNALS_MC68HC16_H__

#include "signals.h"

namespace debugger {
namespace mc68hc16 {

struct Signals final : SignalsBase<Signals> {
    void getAddr();
    // Whether #AS is asserted: the strobes and the IPIPE phase as #AS
    // opens the cycle. Inline in pins_mc68hc16.cpp, read every clock.
    inline bool getControl();
    // The second IPIPE phase, late in the cycle.
    inline void getPhase2();
    void getData();
    void outData() const;
    void inputMode() const;
    void print() const;

    // The four 74HCS153 reads, selects 00, 01, 11 and 10, as GPIO7 holds
    // them: AL in bits 0-3, AH in bits 16-19. AL carries A0-A3, A4-A7,
    // A8-A11 and A12-A15; AH A16-A19, A20-A23 (copies of A19), FC2:FC0
    // and SIZ1:SIZ0.
    struct Address {
        uint32_t addr;
        uint8_t fc;
        uint8_t siz;
    };
    static Address compose(
            uint32_t sel00, uint32_t sel01, uint32_t sel11, uint32_t sel10) {
        const auto lo = [](uint32_t v) { return v & 0xF; };
        const auto hi = [](uint32_t v) { return (v >> 16) & 0xF; };
        Address a;
        a.addr = lo(sel00) | lo(sel01) << 4 | lo(sel11) << 8 | lo(sel10) << 12 |
                 hi(sel00) << 16;
        a.fc = hi(sel11) & 7;
        a.siz = hi(sel10) & 3;
        return a;
    }

    bool strobe() const { return (cntl() & AS) == 0; }
    bool read() const { return (cntl() & RW) != 0; }
    bool write() const { return !read(); }
    uint8_t fc() const { return space() & 7; }
    uint8_t siz() const { return (space() >> 4) & 3; }
    bool program() const { return fc() == 6; }
    bool dataSpace() const { return fc() == 5; }
    bool iack() const { return fc() == 7; }
    // Port size is 16 bits: anything but a byte moves the aligned pair
    // when it starts at an even address (User's Manual Table 5-16).
    bool wordAccess() const { return siz() != 1 && (addr & 1) == 0; }

    // IPIPE1:IPIPE0, active low, two phases (CPU16RM 10.1.2). Phase 1:
    // 00 START and FETCH, 01 FETCH, 10 START, 11 NULL. Phase 2: 00
    // INVALID, 01 ADVANCE, 10 EXCEPTION, 11 NULL.
    uint8_t phase1() const { return (cntl() >> 2) & 3; }
    uint8_t phase2() const { return (cntl2() >> 2) & 3; }
    bool startState() const { return (phase1() & 1) == 0; }
    bool fetchState() const { return (phase1() & 2) == 0; }
    bool advanceState() const { return phase2() == 1; }
    bool exceptionState() const { return phase2() == 2; }
    bool invalidState() const { return phase2() == 0; }

    // No bus cycle for a long time: WAI or LPSTOP.
    void markHalt() { cntl() = cntl2() = NONE, mark() = HALT; }
    bool halt() const { return mark() == HALT; }
    // An instruction started in this cycle; back() cycles earlier is the
    // fetch of its opcode word, 0 when the ring does not hold it.
    void markStart(uint8_t back) { mark() = START, this->back() = back; }
    void clearMark() { mark() = 0, back() = 0; }
    bool fetch() const { return mark() == START; }
    uint8_t back() const { return _signals[4]; }

private:
    friend struct PinsMc68hc16;
    // cntl() bits: P9.04-P9.08 as CNTL reads them
    enum : uint8_t {
        AS = 0x01,
        RW = 0x02,
        IPIPE0 = 0x04,
        IPIPE1 = 0x08,
        DS = 0x10,
        NONE = 0x1F,
    };
    enum : uint8_t { START = 1, HALT = 2 };
    // As #AS opens the cycle, so with phase 1.
    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
    // Late in the cycle, so with phase 2.
    uint8_t cntl2() const { return _signals[1]; }
    uint8_t &cntl2() { return _signals[1]; }
    // FC2:FC0 in bits 0-2, SIZ1:SIZ0 in bits 4-5.
    uint8_t space() const { return _signals[2]; }
    uint8_t &space() { return _signals[2]; }
    uint8_t mark() const { return _signals[3]; }
    uint8_t &mark() { return _signals[3]; }
    uint8_t &back() { return _signals[4]; }
};

}  // namespace mc68hc16
}  // namespace debugger
#endif /* __SIGNALS_MC68HC16_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
