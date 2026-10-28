#ifndef __SIGNALS_H16_H__
#define __SIGNALS_H16_H__

#include "signals.h"

namespace debugger {
namespace h16 {

// ST2-ST0 (User's Manual Table 3-2).
enum BUS_ST : uint8_t {
    ST_SLEEP = 0,
    ST_HALT = 1,
    ST_LOCKED = 2,  // TAS read-modify-write
    ST_NORMAL = 3,
    ST_DMA_SINGLE = 4,
    ST_DMA_DUAL = 5,
    ST_REFRESH = 6,
    ST_IACK = 7,
};

struct Signals final : SignalsBase<Signals> {
    // Whether #AS is asserted, with the strobes and clock as it opens the
    // cycle. Inline in pins_h16.cpp, polled.
    inline bool getControl();
    // Address, ST and PF, read right as #AS asserts: A1-A16 turn into
    // data in T2.
    void getAddr();
    // Whether #HDS or #LDS is asserted, which picks the byte lane.
    inline bool getStrobes();
    void getData();
    // Read data in two steps: into the port while it is still an input,
    // then the port driving it.
    void stageData() const;
    void driveData() const;
    void inputMode() const;
    void print() const;

    uint8_t st() const { return status() & 7; }
    bool programFetch() const { return (status() & PF) != 0; }
    bool read() const { return (cntl() & RW) != 0; }
    bool write() const { return !read(); }
    // A CPU memory cycle the debugger serves: normal or locked.
    bool memory() const { return st() == ST_NORMAL || st() == ST_LOCKED; }
    bool refresh() const { return st() == ST_REFRESH; }
    bool iack() const { return st() == ST_IACK; }
    // Both strobes: the aligned word (Table 4-38).
    bool wordAccess() const { return (strobes() & (HDS | LDS)) == 0; }

    // No bus cycle for a long time: SLEEP, or halted on a double bus error.
    void markHalt() { cntl() = strobes() = NONE, mark() = HALT; }
    bool halt() const { return mark() == HALT; }
    void clearMark() { mark() = 0; }

private:
    friend struct PinsH16;
    // cntl() bits: P9.04-P9.08 as CNTL reads them
    enum : uint8_t {
        AS = 0x01,
        RW = 0x02,
        HDS = 0x04,
        LDS = 0x08,
        PHI = 0x10,
        NONE = 0x1F,
    };
    enum : uint8_t { PF = 0x08 };
    enum : uint8_t { HALT = 1 };
    // As #AS opens the cycle.
    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
    // Once the data strobes assert.
    uint8_t strobes() const { return _signals[1]; }
    uint8_t &strobes() { return _signals[1]; }
    // ST2-ST0 in bits 0-2, PF in bit 3.
    uint8_t status() const { return _signals[2]; }
    uint8_t &status() { return _signals[2]; }
    uint8_t mark() const { return _signals[3]; }
    uint8_t &mark() { return _signals[3]; }
};

}  // namespace h16
}  // namespace debugger
#endif /* __SIGNALS_H16_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
