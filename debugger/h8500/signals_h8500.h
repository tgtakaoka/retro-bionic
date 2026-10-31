#ifndef __SIGNALS_H8500_H__
#define __SIGNALS_H8500_H__

#include "signals.h"

namespace debugger {
namespace h8500 {

struct ChipH8500;

struct Signals final : SignalsBase<Signals> {
    // Which board the cycles come from: the bus width and the address
    // lines differ. Set once, by the identity.
    static void setChip(const ChipH8500 *chip);

    // Whether #AS is asserted, with the strobes and the clock as it opens
    // the cycle. Inline in pins_h8500.cpp, polled.
    inline bool getControl();
    // The strobes once they settle: #RD, or the write strobes that pick
    // the byte lanes.
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
    bool write() const { return (strobes() & (HWR | LWR)) != (HWR | LWR); }
    // #HWR and #LWR together: the aligned word. A read gives no sign of its
    // width, so it moves the aligned pair on the H8/510's bus.
    bool wordWrite() const { return (strobes() & (HWR | LWR)) == 0; }
    static bool bus16();

    // No bus cycle for a long time: SLEEP, or a program running from
    // on-chip memory.
    void markHalt() { cntl() = strobes() = NONE, mark() = HALT; }
    bool halt() const { return mark() == HALT; }
    void clearMark() { mark() = 0; }

private:
    friend struct PinsH8500;
    // cntl() bits: P9.04-P9.08 as CNTL reads them. #WR is the H8/536's
    // only write strobe, on #HWR's pin; its #LWR pin is pulled up.
    enum : uint8_t {
        AS = 0x01,
        RD = 0x02,
        HWR = 0x04,
        LWR = 0x08,
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

}  // namespace h8500
}  // namespace debugger
#endif /* __SIGNALS_H8500_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
