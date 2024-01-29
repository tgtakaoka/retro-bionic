#include "pins_mc6802.h"
#include "debugger.h"
#include "devs_mc6800.h"
#include "digital_bus.h"
#include "inst_mc6800.h"
#include "mems_mc6802.h"
#include "regs_mc6802.h"
#include "signals_mc6802.h"

namespace debugger {
namespace mc6802 {

/**
 * MC6802 bus cycle.
 *          __    __    __    __    __    __    __    __    __   _
 * EXTAL c4|  |c1|  |c2|  |c3|  |c4|  |c1|  |c2|  |c3|  |c4|  |c1|
 *       __ \           \___________\           \___________\
 *     E    |___________|           |___________|           |____
 *       ______  ______________________  ______________________  ___
 *   VMA ______><______________________><______________________><___
 *       ______  ______________________                         ___
 *   R/W ______><                      |_______________________|
 *       ______  ______________________  ______________________  ___
 *  Addr ______><______________________><______________________><___
 *                              ____                 ________
 *  Data ----------------------<____>---------------<________>---
 *
 * - EXTAL rising-edge to E edges takes 100ns.
 * - EXTAL rising-edge of c4 to VMA and R/W edges take 100ns.
 * - VMA and R/W are valid before falling-edge of c1.
 * - Read data setup to falling E egde is 100ns.
 * - Write data gets valid after 225ns of rising E edge.
 */

namespace {

// XTAL: max 4 MHz, min 0.4 MHz
// tCYC: min 1000 ns, max 10,000 ns
// PWEL: min  450 ns, max  5,000 ns; e_lo
// PWEH: min  450 ns, max  9,700 ns; e_hi
// tAV:  min  160 ns; max 270 ns; addr to e_hi
// tDSR: min   30 ns; data to e_lo
// tDDW: max  225 ns; e_hi to data
// tPCS: min  200 ns; reset_hi to e_lo
constexpr auto extal_hi_ns = 108;   // 125
constexpr auto extal_lo_ns = 108;   // 125
constexpr auto c1_hi_ns = 57;       // 125
constexpr auto c1_lo_ns = 67;       // 125
constexpr auto c2_hi_ns = 48;       // 125
constexpr auto c2_lo_ns = 51;       // 125
constexpr auto c3_hi_novma = 87;    // 125
constexpr auto c3_lo_novma = 95;    // 125
constexpr auto c4_hi_novma = 60;    // 125
constexpr auto c4_lo_novma = 102;   // 125
constexpr auto c3_hi_read = 0;      // 125
constexpr auto c3_hi_inject = 45;   // 125
constexpr auto c3_lo_read = 46;     // 125
constexpr auto c4_hi_read = 46;     // 125
constexpr auto c4_lo_read = 96;     // 125
constexpr auto c3_hi_write = 93;    // 125
constexpr auto c3_lo_write = 88;    // 125
constexpr auto c4_hi_write = 50;    // 125
constexpr auto c4_lo_write = 0;     // 125
constexpr auto c4_lo_capture = 67;  // 125
constexpr auto tpcs_ns = 200;

inline void extal_hi() __attribute__((always_inline));
inline void extal_hi() {
    digitalWriteFast(PIN_EXTAL, HIGH);
}

inline void extal_lo() __attribute__((always_inline));
inline void extal_lo() {
    digitalWriteFast(PIN_EXTAL, LOW);
}

inline void extal_cycle() __attribute__((always_inline));
inline void extal_cycle() {
    extal_lo();
    delayNanoseconds(extal_lo_ns);
    extal_hi();
    delayNanoseconds(extal_hi_ns);
}

uint8_t clock_e() {
    return digitalReadFast(PIN_E);
}

void assert_nmi() {
    digitalWriteFast(PIN_NMI, LOW);
}

void negate_nmi() {
    digitalWriteFast(PIN_NMI, HIGH);
}

void assert_irq() {
    digitalWriteFast(PIN_IRQ, LOW);
}

void negate_irq() {
    digitalWriteFast(PIN_IRQ, HIGH);
}

// TODO: Utilize #HALT to Pins.step()
void assert_halt() __attribute__((unused));
void assert_halt() {
    digitalWriteFast(PIN_HALT, LOW);
}

void negate_halt() {
    digitalWriteFast(PIN_HALT, HIGH);
}

void negate_mr() {
    digitalWriteFast(PIN_MR, HIGH);
}

void assert_reset() {
    // Drive RESET condition
    digitalWriteFast(PIN_RESET, LOW);
    negate_halt();
    negate_nmi();
    negate_irq();
    negate_mr();
}

void negate_reset() {
    // Release RESET conditions
    digitalWriteFast(PIN_RESET, HIGH);
}

const uint8_t PINS_LOW[] = {
        PIN_RESET,
};

const uint8_t PINS_HIGH[] = {
        PIN_EXTAL,
        PIN_HALT,
        PIN_IRQ,
        PIN_NMI,
        PIN_MR,
};

const uint8_t PINS_PULLUP[] = {
        PIN_RE,
};

const uint8_t PINS_INPUT[] = {
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
        PIN_E,
};

}  // namespace

void PinsMc6802::reset() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_PULLUP, sizeof(PINS_PULLUP), INPUT_PULLUP);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    Memory.set_internal_ram(digitalReadFast(PIN_RE) != LOW);
    const auto reset_vec = _mems->raw_read16(InstMc6800::VEC_RESET);
    _mems->raw_write16(InstMc6800::VEC_RESET, 0x8000);

    assert_reset();
    for (auto i = 0; i < 3; ++i)
        extal_cycle();
    // Synchronize clock output and E clock input.
    while (clock_e() == LOW) {
        extal_cycle();
        delayNanoseconds(extal_hi_ns);
    }
    while (clock_e() == HIGH) {
        extal_cycle();
        delayNanoseconds(extal_hi_ns);
    }
    // #RESET, when brought low, must be held low at least three clock
    // #cycles.
    for (auto i = 0; i < 3; i++)
        cycle();
    Signals::resetCycles();
    cycle();
    delayNanoseconds(tpcs_ns);
    negate_reset();
    cycle();
    // Read Reset vector
    cycle();
    cycle();
    // The first instruction will be saving registers, and certainly can be
    // injected.
    _regs->reset();
    _regs->save();
    _mems->raw_write16(InstMc6800::VEC_RESET, reset_vec);
    _regs->setIp(reset_vec);
    _regs->checkSoftwareType();
}

mc6800::Signals *PinsMc6802::cycle() {
    delayNanoseconds(c1_hi_ns);
    return rawCycle();
}

mc6800::Signals *PinsMc6802::rawCycle() {
    // MC6802/MC6808 clock E is CLK/4, so we toggle CLK 4 times
    // c1
    extal_lo();
    busMode(D, INPUT);
    auto *signals = Signals::put();
    delayNanoseconds(c1_lo_ns);
    // c2
    extal_hi();
    delayNanoseconds(c2_hi_ns);
    signals->getAddr();
    extal_lo();
    signals->getDirection();
    delayNanoseconds(c2_lo_ns);
    // c3
    extal_hi();

    if (!signals->valid()) {
        delayNanoseconds(c3_hi_novma);
        extal_lo();
        delayNanoseconds(c3_lo_novma);
        // c4
        extal_hi();
        Signals::nextCycle();
        delayNanoseconds(c4_hi_novma);
        extal_lo();
        delayNanoseconds(c4_lo_novma);
    } else if (signals->read() && !Memory.is_internal(signals->addr)) {
        if (signals->readMemory()) {
            signals->data = _mems->read(signals->addr);
            if (c3_hi_read)
                delayNanoseconds(c3_hi_read);
        } else {
            // inject data from signals->data
            delayNanoseconds(c3_hi_inject);
        }
        extal_lo();
        busMode(D, OUTPUT);
        busWrite(D, signals->data);
        delayNanoseconds(c3_lo_read);
        // c4
        extal_hi();
        _writes = 0;
        Signals::nextCycle();
        delayNanoseconds(c4_hi_read);
        extal_lo();
        delayNanoseconds(c4_lo_read);
    } else {
        delayNanoseconds(c3_hi_write);
        extal_lo();
        if (signals->write()) {
            ++_writes;
        } else {
            _writes = 0;  // CPU output internal RAM reading to bus
        }
        delayNanoseconds(c3_lo_write);
        // c4
        extal_hi();
        Signals::nextCycle();
        delayNanoseconds(c4_hi_write);
        extal_lo();
        if (signals->writeMemory()) {
            if (c4_lo_write)
                delayNanoseconds(c4_lo_write);
            signals->getData();
            _mems->write(signals->addr, signals->data);
        } else {
            delayNanoseconds(c4_lo_capture);
            signals->getData();
        }
    }
    // c1
    extal_hi();

    return signals;
}

void PinsMc6802::assertNmi() const {
    assert_nmi();
}

void PinsMc6802::negateNmi() const {
    negate_nmi();
}

void PinsMc6802::assertInt(uint8_t name) {
    (void)name;
    assert_irq();
}

void PinsMc6802::negateInt(uint8_t name) {
    (void)name;
    negate_irq();
}

}  // namespace mc6802
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
