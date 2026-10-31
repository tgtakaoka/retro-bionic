#ifndef __SIGNALS_H8300_H__
#define __SIGNALS_H8300_H__

#include "signals.h"

namespace debugger {
namespace h8300 {

struct Signals final : SignalsBase<Signals> {
    // Whether #AS is asserted, with the strobes and the clock as it opens
    // the cycle. Inline in pins_h8300.cpp, polled.
    inline bool getControl();
    // The strobes once they settle: #RD asserts with #AS, #WR in T2.
    inline bool getStrobes();
    // The address, valid from T1 through T3: on-chip accesses never show
    // #AS, so every cycle seen is external.
    void getAddr();
    void getData();
    // Read data in two steps: into the port while it is still an input,
    // then the port driving it.
    void stageData() const;
    void driveData() const;
    void inputMode() const;
    void print() const;

    bool read() const { return (strobes() & RD) == 0; }
    bool write() const { return (strobes() & WR) == 0; }

    // No bus cycle for a long time: SLEEP, or a program running from
    // the on-chip RAM.
    void markHalt() { cntl() = strobes() = NONE, mark() = HALT; }
    bool halt() const { return mark() == HALT; }
    void clearMark() { mark() = 0; }

private:
    friend struct PinsH8300;
    // cntl() bits: P9.04-P9.08 as CNTL reads them.
    enum : uint8_t {
        AS = 0x01,
        RD = 0x02,
        WR = 0x04,
        IOS = 0x08,
        PHI = 0x10,
        NONE = 0x1F,
    };
    enum : uint8_t { HALT = 1 };
    // As #AS opens the cycle.
    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
    // Once a data strobe asserts.
    uint8_t strobes() const { return _signals[1]; }
    uint8_t &strobes() { return _signals[1]; }
    uint8_t mark() const { return _signals[3]; }
    uint8_t &mark() { return _signals[3]; }
};

}  // namespace h8300
}  // namespace debugger
#endif /* __SIGNALS_H8300_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
