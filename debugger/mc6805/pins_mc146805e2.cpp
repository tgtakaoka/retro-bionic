#include "pins_mc146805e2.h"
#include "debugger.h"
#include "devs_mc146805e2.h"
#include "digital_bus.h"
#include "inst_mc146805.h"
#include "mems_mc146805e2.h"
#include "regs_mc146805e2.h"

namespace debugger {
namespace mc146805e2 {

using mc146805::Inst;

struct PinsMc146805E2 Pins {
    Regs, Inst, Memory, Devices
};

/**
 * MC146805E bus cycle.
 *      _    __    __    __    __    __    __    __    __    __    __
 * OSC1  |_c|1 |_c|2 |_c|3 |_c|4 |_c|5 |_c|1 |_c|2 |_c|3 |_c|4 |_c|5 |__
 *       \     \ ____\  \  \           \     \ ____\  \  \           \
 *   AS __|_____|     |__|__|___________|_____|     |__|__|___________|_
 *      __            \  |  |___________|           \  |  |___________|
 *   DS   |____________|_|__|           |____________|_|__|           |_
 *        \            |_|______________\____________| |
 *   LI ___|___________| |               |           |_|________________
 *      ___|_____________|_______________|             |
 *  R/W ___|             |               |_____________|________________
 *      ___________ _____|________________________ ____|________________
 * ADRH ___________X_____|________________________X____|________________
 *                   ____|            _____        ____|_______________
 * DATA ------------<_AL_>-----------<__DR_>------<_AL_x________DW_____>-
 */

namespace {

//  fOSC: min 5.0 MHz
// tOLOL: min:  200 ns: osc_cycle
//   tOH: min    75 ns: osc_hi
//   tOL: min    75 ns: osc_lo
//  tCYC: min 1,000 ns; ds_cycle
// tPWEL: min   560 ns: ds_lo
// tPWEH: min   375 ns: ds_hi
//  tASD: min   160 ns: ds_lo to as_hi
// PWASH: min   175 ns: as_hi
// tASED: min   160 ns: as_lo to ds_hi
//   tAD: max   300 ns: ds_lo to r/w
//  tADH: max   100 ns: as_hi to addr
//  tAHL: min    60 ns: as_lo to addr hold
//  tDSR: min   115 ns: ds_hi to data
//  tDHR: min   160 ns: ds_lo to data hold
//  tDDW: max   120 ns: ds_hi to data
//  tDHW: min    55 ns: ds_lo to data hold
//   tRL: min 1,500 ns; reset_lo
constexpr auto osc1_ds_ns = 100;
constexpr auto osc1_hi_ns = 84;     // 100
constexpr auto osc1_lo_ns = 76;     // 100
constexpr auto c1_lo_ns = 68;       // 100
constexpr auto c1_hi_ns = 56;       // 100
constexpr auto c2_lo_ns = 38;       // 100
constexpr auto c2_hi_ns = 78;       // 100
constexpr auto c3_lo_ns = 48;       // 100
constexpr auto c3_hi_ns = 48;       // 100
constexpr auto c4_lo_read = 0;      // 100
constexpr auto c4_lo_inject = 0;    // 100
constexpr auto c4_hi_read = 28;     // 100
constexpr auto c5_lo_read = 80;     // 100
constexpr auto c4_lo_write = 4;     // 100
constexpr auto c4_hi_write = 48;    // 100
constexpr auto c5_lo_write = 0;     // 100
constexpr auto c5_lo_capture = 68;  // 100
constexpr auto c5_hi_ns = 32;       // 100
constexpr auto tpcs_ns = 200;

inline void osc1_hi() {
    digitalWriteFast(PIN_OSC1, HIGH);
}

inline void osc1_lo() {
    digitalWriteFast(PIN_OSC1, LOW);
}

inline void clock_cycle() {
    delayNanoseconds(osc1_lo_ns);
    osc1_hi();
    delayNanoseconds(osc1_hi_ns);
    osc1_lo();
}

inline auto signal_ds() {
    return digitalReadFast(PIN_DS);
}

void assert_irq() {
    digitalWriteFast(PIN_IRQ, LOW);
}

void negate_irq() {
    digitalWriteFast(PIN_IRQ, HIGH);
}

void negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
}

constexpr uint8_t PINS_LOW[] = {
        PIN_OSC1,
        PIN_RESET,
        PIN_TIMER,
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_IRQ,
};

constexpr uint8_t PINS_INPUT[] = {
        PIN_B0,
        PIN_B1,
        PIN_B2,
        PIN_B3,
        PIN_B4,
        PIN_B5,
        PIN_B6,
        PIN_B7,
        PIN_ADDR8,
        PIN_ADDR9,
        PIN_ADDR10,
        PIN_ADDR11,
        PIN_ADDR12,
        PIN_AS,
        PIN_DS,
        PIN_RW,
        PIN_LI,
};

}  // namespace

void PinsMc146805E2::reset() {
    // Assert reset condition
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT_PULLDOWN);

    // #RESET must stay low for a minimum of one tRL (1.5usec at VDD=5V).
    for (auto i = 0; i < 15 * 5; i++)
        clock_cycle();

    // Synchronize clock output to DS.
    while (signal_ds() == LOW) {
        clock_cycle();
        delayNanoseconds(osc1_ds_ns);
    }
    while (signal_ds() != LOW) {
        clock_cycle();
        delayNanoseconds(osc1_ds_ns);
    }
    // DS=L

    const auto vec_reset = _mems.vecReset();
    const auto vector = _mems.raw_read16(vec_reset) & _mems.maxAddr();
    // If reset vector pointing internal memory, we can't inject instructions.
    _mems.raw_write16(vec_reset, 0x1000);

    cycle();
    cycle();
    delayNanoseconds(tpcs_ns);
    negate_reset();
    Signals::resetCycles();
    prepareCycle();
    suspend();

    // We should certainly inject SWI by pointing external address here.
    _regs.save();
    // Restore reset vector
    _mems.raw_write16(vec_reset, vector);
    _regs.setIp(vector);
}

mc6805::Signals *PinsMc146805E2::currCycle() const {
    auto s = Signals::put();
    s->getDirection();
    s->getAddr();
    s->getLoadInstruction();
    return s;
}

mc6805::Signals *PinsMc146805E2::rawPrepareCycle() const {
    // MC146805E bus cycle is CLK/5, so we toggle CLK 5 times c1
    // c1
    osc1_hi();
    // To ensure 160ns data hold time after DS-falling edge.
    busMode(B, INPUT);
    delayNanoseconds(c1_hi_ns);
    osc1_lo();  // AS->LOW
    // c2
    delayNanoseconds(c2_lo_ns);
    auto s = Signals::put();
    s->getDirection();
    osc1_hi();
    delayNanoseconds(c2_hi_ns);
    // c3
    osc1_lo();  // AS->LOW
    delayNanoseconds(c3_lo_ns);
    s->getAddr();
    osc1_hi();
    delayNanoseconds(c3_hi_ns);
    s->getLoadInstruction();
    // c4
    osc1_lo();
    // DS=HIGH

    return s;
}

mc6805::Signals *PinsMc146805E2::prepareCycle() const {
    delayNanoseconds(c1_lo_ns);
    return rawPrepareCycle();
}

mc6805::Signals *PinsMc146805E2::completeCycle(mc6805::Signals *signals) const {
    auto s = static_cast<Signals *>(signals);
    if (s->write()) {
        delayNanoseconds(c4_lo_write);
        // c4
        osc1_hi();
        s->getData();
        delayNanoseconds(c4_hi_write);
        // c5
        osc1_lo();  // DS=HIGH
        if (s->writeMemory()) {
            _mems.write(s->addr, s->data);
            if (c5_lo_write)
                delayNanoseconds(c5_lo_write);
        } else {
            delayNanoseconds(c5_lo_capture);
        }
    } else {
        if (s->readMemory()) {
            s->data = _mems.read(s->addr);
            if (c4_lo_read)
                delayNanoseconds(c4_lo_read);
        } else {
            delayNanoseconds(c4_lo_inject);
        }
        // c4
        osc1_hi();
        busMode(B, OUTPUT);
        busWrite(B, s->data);
        delayNanoseconds(c4_hi_read);
        // c5
        osc1_lo();  // DS=HIGH
        delayNanoseconds(c5_lo_read);
    }
    osc1_hi();
    Signals::nextCycle();
    delayNanoseconds(c5_hi_ns);
    // Data hold time will be done in next cycle().
    osc1_lo();  // DS->LOW

    return s;
}

void PinsMc146805E2::assertInt(uint8_t name) {
    (void)name;
    assert_irq();
}

void PinsMc146805E2::negateInt(uint8_t name) {
    (void)name;
    negate_irq();
}

}  // namespace mc146805e2
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
