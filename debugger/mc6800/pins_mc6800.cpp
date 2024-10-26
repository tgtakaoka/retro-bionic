#include "debugger.h"
#include "devs_mc6800.h"
#include "digital_bus.h"
#include "inst_mb8861.h"
#include "inst_mc6800.h"
#include "mems_mc6800.h"
#include "pins_mc6800.h"
#include "regs_mc6800.h"
#include "signals_mc6800.h"

namespace debugger {
namespace mc6800 {

/**
 * MC6800 bus cycle.
 *         __________            __________            _____
 *  PHI1 _|          |__________|          |__________|
 *       _     _________________     _________________     _
 *   DBE  |___|                 |___|                 |___|
 *       _            __________            __________
 *  PHI2  |__________|          |__________|          |_____
 *       ________  ____________________  ___________________
 *   VMA ________><____________________><___________________
 *       ________  ____________________
 *   R/W ________>                     |____________________
 *       __       _______________       _______________
 *  Addr __>-----<_______________>-----<_______________>----
 *                           ____             _________
 *  Data -------------------<____>-----------<_________>----
 *
 * - Minimum DBE asserted period is 150ns.
 * - Minimum PHI1/PHI2 high period is 400ns.
 * - Minimum overrapping of PHI1 and PHI2 is 0ns.
 * - Maximum separation of PHI1 and PHI2 is 9100ns.
 * - PHI1 rising-edge to valid VMA, R/W and address is 270ns.
 * - Read data setup to falling PHI2 egde is 100ns.
 * - Read data hold to falling PHI2 edge is 10ns.
 * - Write data gets valid after 225ns of rising DBE edge.
 */

namespace {

// tCYC: min 1000 ns
// td:   max 9100 ns; phi1_lo and phi2_lo
// tPWH: min  400 ns; phi1_hi, phi2_hi
// tUT:  min  900 ns; phi1_hi + phi2_hi
// tAD:  max  270 ns; phi1_hi to addr
// tDSR: min  100 ns; data to phi2_lo
// tDDW: max  225 ns; dbe_hi to data
// tDBE: min  150 ns; dbe_lo
// tPCS: min  200 ns; reset_hi to phi2_lo
constexpr auto dbe_lo_ns = 108;        // 150 ns
constexpr auto phi1_hi_ns = 147;       // 400 ns
constexpr auto phi2_hi_novma = 386;    // 500 ns
constexpr auto phi2_hi_write = 244;    // 500 ns
constexpr auto phi2_hi_read = 238;     // 500 ns
constexpr auto phi2_hi_capture = 336;  // 500 ns
constexpr auto phi2_hi_inject = 60;    // 500 ns
constexpr auto tpcs_ns = 200;

inline void phi1_hi() {
    digitalWriteFast(PIN_PHI1, HIGH);
    digitalWriteFast(PIN_DBE, LOW);
}

inline void phi2_hi() {
    digitalWriteFast(PIN_PHI1, LOW);
    digitalWriteFast(PIN_PHI2, HIGH);
}

inline void phi2_lo() {
    digitalWriteFast(PIN_PHI2, LOW);
}

inline void assert_dbe() {
    digitalWriteFast(PIN_DBE, HIGH);
}

inline void negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
}

constexpr uint8_t PINS_LOW[] = {
        PIN_RESET,
        PIN_PHI1,
        PIN_PHI2,
        PIN_TSC,
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_HALT,
        PIN_DBE,
        PIN_IRQ,
};

constexpr uint8_t PINS_PULLUP[] = {
        PIN_NMI,
};

constexpr uint8_t PINS_INPUT[] = {
        PIN_D0,
        PIN_D1,
        PIN_D2,
        PIN_D3,
        PIN_D4,
        PIN_D5,
        PIN_D6,
        PIN_D7,
        PIN_AL0,
        PIN_AL1,
        PIN_AL2,
        PIN_AL3,
        PIN_AL4,
        PIN_AL5,
        PIN_AL6,
        PIN_AL7,
        PIN_AM8,
        PIN_AM9,
        PIN_AM10,
        PIN_AM11,
        PIN_AH12,
        PIN_AH13,
        PIN_AH14,
        PIN_AH15,
        PIN_RW,
        PIN_VMA,
        PIN_BA,
};

}  // namespace

void PinsMc6800::reset() {
    // Assert reset condition
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_PULLUP, sizeof(PINS_PULLUP), INPUT_PULLUP);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    // Assuming a minimum of 8 clock cycles have occurred.
    for (auto i = 0; i < 8; ++i)
        cycle();
    cycle();
    delayNanoseconds(tpcs_ns);
    negate_reset();
    Signals::resetCycles();
    cycle();
    // Read Reset vector
    cycle();
    cycle();
    // The first instruction will be saving registers, and certainly can be
    // injected.
    _regs.reset();
    _regs.save();
    _regs.setIp(_mems.raw_read16(InstMc6800::VEC_RESET));
    _regs.checkSoftwareType();
}

Signals *PinsMc6800::cycle() {
    return rawCycle();
}

Signals *PinsMc6800::rawCycle() {
    // PHI1=HIGH
    phi1_hi();
    busMode(D, INPUT);
    delayNanoseconds(dbe_lo_ns);
    assert_dbe();
    delayNanoseconds(phi1_hi_ns);
    auto s = Signals::put();
    s->getAddr();
    // PHI2=HIGH
    phi2_hi();
    s->getDirection();

    if (!s->valid()) {
        delayNanoseconds(phi2_hi_novma);
    } else if (s->write()) {
        ++_writes;
        if (s->writeMemory()) {
            delayNanoseconds(phi2_hi_write);
            s->getData();
            _mems.write(s->addr, s->data);
        } else {
            delayNanoseconds(phi2_hi_capture);
            s->getData();
        }
    } else {
        _writes = 0;
        if (s->readMemory()) {
            s->data = _mems.read(s->addr);
        } else {
            // inject data from s->data
            delayNanoseconds(phi2_hi_inject);
        }
        busMode(D, OUTPUT);
        busWrite(D, s->data);
        delayNanoseconds(phi2_hi_read);
    }
    Signals::nextCycle();
    // PHI2=LOW
    phi2_lo();

    return s;
}

}  // namespace mc6800
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
