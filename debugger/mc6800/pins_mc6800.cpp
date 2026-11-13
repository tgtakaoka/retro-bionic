#include "pins_mc6800.h"
#include "debugger.h"
#include "devs_mc6800.h"
#include "inst_mb8861.h"
#include "mems_mc6800.h"
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
 * - Minimum overlapping of PHI1 and PHI2 is 0ns.
 * - Maximum separation of PHI1 and PHI2 is 9100ns.
 * - PHI1 rising-edge to valid VMA, R/W and address is 270ns.
 * - Read data setup to falling PHI2 edge is 100ns.
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
enum Delay : uint8_t {
    dbe_lo_ns,        // 150 ns
    phi1_hi_ns,       // 400 ns
    phi1_lo_ns,       // PHI1 low to PHI2 high
    phi2_hi_novma,    // 500 ns
    phi2_hi_write,    // 500 ns
    phi2_hi_read,     // 500 ns
    phi2_hi_capture,  // 500 ns
    phi2_hi_inject,   // 500 ns
    phi_lo_ns,        // tCYC - tUT
    phi_lo_loop,      // +Devs::loop(); tCYC - tUT
    DELAYS,
};
// Long enough that no PHI cycle runs under tCYC or tUT, the 1 MHz part's.
constexpr uint16_t DELAYS_NS[DELAYS] = {
        115, 157, 30, 411, 300, 294, 358, 104, 107, 43};
// MC68B00 (2 MHz): no PHI cycle under 500 ns on the bench, 1.95 MHz.
// constexpr uint16_t DELAYS_NS[DELAYS] = {
//         27, 36, 0, 96, 61, 59, 84, 15, 25, 10};
constexpr auto tpcs_ns = 200;

inline void phi1_hi() {
    digitalWriteFast(PIN_PHI1, HIGH);
    digitalWriteFast(PIN_DBE, LOW);
}

inline void phi1_lo() {
    digitalWriteFast(PIN_PHI1, LOW);
}

inline void phi2_hi() {
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

PinsMc6800::PinsMc6800() {
    auto regs = new RegsMc6800(this);
    auto devs = new DevsMc6800();
    _mems = new MemsMc6800(regs, devs);
    _regs = regs;
    _devs = devs;
    _inst = new mb8861::InstMb8861(_mems);
}

template <uint8_t F>
inline void PinsMc6800::delay() const {
    if constexpr (DELAYS_NS[F] != 0)
        delayNanoseconds(DELAYS_NS[F]);
}

void PinsMc6800::resetPins() {
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
    Cycles::reset();
    cycle();
    // Read Reset vector
    cycle();
    cycle();
    // The first fetch takes the save's injected instructions.
    _regs->reset();
    _regs->save();
    _regs->setIp(_mems->read16(ArchMc6800::VEC_RESET));
    regs<RegsMc6800>()->checkSoftwareType();
}

Signals *PinsMc6800::cycle() {
    delay<phi_lo_ns>();
    return rawCycle();
}

Signals *PinsMc6800::loopCycle() {
    delay<phi_lo_loop>();
    return rawCycle();
}

Signals *PinsMc6800::rawCycle() {
    // PHI1=HIGH
    phi1_hi();
    Signals::inputMode();
    delay<dbe_lo_ns>();
    assert_dbe();
    delay<phi1_hi_ns>();
    auto s = Signals::put();
    s->getAddr();
    // PHI2=HIGH
    phi1_lo();
    delay<phi1_lo_ns>();
    phi2_hi();
    s->getDirection();

    if (!s->valid()) {
        delay<phi2_hi_novma>();
    } else if (s->write()) {
        ++_writes;
        if (s->writeMemory()) {
            delay<phi2_hi_write>();
            s->getData();
            mems<MemsMc6800>()->write(s->addr, s->data);
        } else {
            delay<phi2_hi_capture>();
            s->getData();
        }
    } else {
        _writes = 0;
        if (s->readMemory()) {
            s->data = mems<MemsMc6800>()->read(s->addr);
        } else {
            // inject data from s->data
            delay<phi2_hi_inject>();
        }
        s->outData();
        delay<phi2_hi_read>();
    }
    Cycles::next();
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
