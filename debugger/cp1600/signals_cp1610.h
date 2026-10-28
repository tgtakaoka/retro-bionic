#ifndef __SIGNALS_CP1610_H__
#define __SIGNALS_CP1610_H__

#include "signals.h"

namespace debugger {
namespace cp1600 {

// Bus control code as BC2:BC1:BDIR, the order they appear on the port.
enum BusControl : uint8_t {
    NACT = 0,   // No action
    BAR = 1,    // Bus to address register
    ADAR = 2,   // Addressed data to address register
    DW = 3,     // Data write
    IAB = 4,    // Interrupt address to bus
    DWS = 5,    // Data write strobe
    DTB = 6,    // Data to bus
    INTAK = 7,  // Interrupt acknowledge
};

struct Signals final : SignalsBase<Signals> {
    void getControl();
    void getAddr();
    void getData();
    void outData() const;
    static void inputMode();

    BusControl bus() const { return static_cast<BusControl>(cntl() & 7); }
    bool read() const;
    bool write() const;
    bool fetch() const;
    void markFetch();
    void clearFetch();

    void print() const;

private:
    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
};

}  // namespace cp1600
}  // namespace debugger
#endif /* __SIGNALS_CP1610_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
