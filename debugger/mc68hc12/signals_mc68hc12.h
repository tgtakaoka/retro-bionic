#ifndef __SIGNALS_MC68HC12_H__
#define __SIGNALS_MC68HC12_H__

#include "signals.h"

namespace debugger {
namespace mc68hc12 {

struct Signals final : SignalsBase<Signals> {
    // Before the E rise: address, R/W, #LSTRB and the queue movement.
    void getAddr();
    // After the E rise: #DBE, low for an external read only.
    void getDbe();
    // Before the E fall: the execution start.
    void getStart();
    void getData();
    void outData() const;
    static void inputMode();
    void print() const;

    bool read() const { return (cntl() & S_READ) != 0; }
    bool write() const { return !read() && !none(); }
    // An external read: the one the debugger answers. A read without #DBE
    // is a free cycle or an internal one the CPU shows.
    bool external() const { return read() && (cntl() & S_DBE) == 0; }
    // A bus cycle that didn't happen.
    void noCycle() { cntl() = S_NONE; }
    bool none() const { return cntl() == S_NONE; }

    // The bytes the cycle moves, from its address, and the one at |addr|:
    // the even byte rides AD8-AD15, the odd one AD0-AD7, whatever the
    // address; #LSTRB equal to A0 is a word.
    uint_fast8_t bytes() const;
    uint8_t byteAt(uint16_t addr) const;

    // IPIPE1:0, the queue movement sampled at the E rise (it refers to
    // the previous cycle's data), and the execution start sampled at the
    // E fall (it refers to the next cycle).
    static constexpr uint8_t MOVE_LAT = 1;
    static constexpr uint8_t MOVE_ALD = 2;
    static constexpr uint8_t MOVE_ALL = 3;
    static constexpr uint8_t START_INT = 1;
    static constexpr uint8_t START_EVEN = 2;
    static constexpr uint8_t START_ODD = 3;
    uint8_t move() const { return ipipe() & 3; }
    uint8_t start() const { return (ipipe() >> 2) & 3; }

    // An instruction starts at this cycle, at |inst()|; set by replaying
    // the queue.
    bool fetch() const { return (mark() & MARK_FETCH) != 0; }
    bool interrupt() const { return (mark() & MARK_INT) != 0; }
    bool program() const { return (mark() & MARK_PROGRAM) != 0; }
    uint16_t inst() const { return _signals[3] | (_signals[4] << 8); }
    void clearMark() { mark() = 0; }
    void markFetch(uint16_t inst);
    void markInterrupt() { mark() |= MARK_INT; }
    void markProgram() { mark() |= MARK_PROGRAM; }

#if !defined(ARDUINO)
    // For host tests: a recorded cycle.
    void set(uint16_t a, uint16_t d, bool rd, uint8_t move, uint8_t start,
            bool dbe = true, bool lstrb = false);
#endif

private:
    static constexpr uint8_t S_READ = 0x01;
    static constexpr uint8_t S_LSTRB = 0x02;  // the level, active low
    static constexpr uint8_t S_DBE = 0x04;    // the level, active low
    static constexpr uint8_t S_NONE = 0xFF;
    static constexpr uint8_t MARK_FETCH = 0x01;
    static constexpr uint8_t MARK_INT = 0x02;
    static constexpr uint8_t MARK_PROGRAM = 0x04;

    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
    uint8_t ipipe() const { return _signals[1]; }
    uint8_t &ipipe() { return _signals[1]; }
    uint8_t mark() const { return _signals[2]; }
    uint8_t &mark() { return _signals[2]; }
};

}  // namespace mc68hc12
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
