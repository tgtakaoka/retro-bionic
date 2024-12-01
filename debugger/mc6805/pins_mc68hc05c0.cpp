#include "pins_mc68hc05c0.h"
#include "debugger.h"
#include "digital_bus.h"
#include "signals_mc68hc05c0.h"

namespace debugger {
namespace mc68hc05c0 {

/**
 * MC68HC05C0 bus cycle.
 *       |--c1-|--c2-|--c3-|--c4-|--c1-|--c2-|--c3-|--c4-|--c1-|
 *      _|   __|   __|   __|   __|   __|   __|   __|   __|   __|
 * OSC1  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|
 *          \ ____\  \           \  \ ____\  \           \  \ __
 *   AS _____|     |_________________|     |_________________|
 *      ______________             _____________________________
 *  #RD               |___________|
 *      ______________________________________             _____
 *  #WR                                       |___________|
 *      ______                        __________________________
 * #LIR       |______________________|
 *      ______ _______________________ _______________________ _
 *   AH ______X_______________________X_______________________X_
 *              ______        ____      ______   ___________   _
 *   AD -------<__AL__>------<_in_>----<__AL__>-<___out_____>-<_
 */

namespace {

//   fOSC: min  DC, max 16MHz
// tOH/OL: min  60 ns; OSC1 pulse width
//   tOSC: min  62.5 ns; one-half oscillator period
//   tCYC: min 250 ns; fOSC/4
//   tASH: typ 2*tOSC; AS high width
// tADDSU: typ 2*tOSC; address setup to AS-
//  tADDH: typ   tOSC; address hold from AS-
// tVARWL: typ 3*tOSC; address valid to #RD-/#WR-
//    tRL: typ 4*tOSC; #RD low width
//  tRDSU: typ  10 ns; read data setup to #RD+
//   tRDH: typ   0 ns; read data hold from #RD+
//    tWL: typ 4*tOSC; #WR low width
//  tWDSU: typ 4*tOSC; write data setup to #WR+
//   tWDH: typ   tOSC; write data hold from #WR+

constexpr auto osc1_hi_ns = 40;       // 60
constexpr auto osc1_lo_ns = 50;       // 60
constexpr auto osc1_hi_as = 40;       // 60
constexpr auto osc1_hi_addr = 20;     // 60
constexpr auto osc1_lo_control = 20;  // 60
constexpr auto osc1_hi_inject = 80;   // 60
constexpr auto osc1_hi_write = 80;    // 60
constexpr auto osc1_lo_capture = 80;  // 60

inline void osc1_hi() {
    digitalWriteFast(PIN_OSC1, HIGH);
}

inline void osc1_lo() {
    digitalWriteFast(PIN_OSC1, LOW);
}

void osc1_cycle_hi() {
    osc1_lo();
    delayNanoseconds(osc1_lo_ns);
    osc1_hi();
}

void osc1_cycle() {
    osc1_cycle_hi();
    delayNanoseconds(osc1_hi_ns);
}

inline auto signal_as() {
    return digitalReadFast(PIN_AS);
}

void negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
}

constexpr uint8_t PINS_OPENDRAIN[] = {
        PIN_RESET,  // bi-directional
};

constexpr uint8_t PINS_LOW[] = {
        PIN_OSC1,
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_IRQ,
};

constexpr uint8_t PINS_PULLUP[] = {
        PIN_LIR,  // #LIR/MODE
};

constexpr uint8_t PINS_INPUT[] = {
        PIN_AD0,
        PIN_AD1,
        PIN_AD2,
        PIN_AD3,
        PIN_AD4,
        PIN_AD5,
        PIN_AD6,
        PIN_AD7,
        PIN_ADDR8,
        PIN_ADDR9,
        PIN_ADDR10,
        PIN_ADDR11,
        PIN_ADDR12,
        PIN_ADDR13,
        PIN_ADDR14,
        PIN_ADDR15,
        PIN_AS,
        PIN_RD,
        PIN_WR,
        PIN_PB0,
        PIN_PB1,
        PIN_PB4,
        PIN_PB5,
};

}  // namespace

void PinsMc68HC05C0::resetPins() {
    // Assert reset condition
    pinsMode(PINS_OPENDRAIN, sizeof(PINS_OPENDRAIN), OUTPUT_OPENDRAIN, LOW);
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    // #LIR/MODE=H; select multiplexed bus mode.
    pinsMode(PINS_PULLUP, sizeof(PINS_PULLUP), INPUT_PULLUP);

    const auto vec_reset = _mems.vecReset();
    const auto vector = _mems.raw_read16(vec_reset);
    // If reset vector pointing internal memory, we can't inject instructions.
    _mems.raw_write16(vec_reset, 0x1000);

    // #RESET input for a period of one and one-half machine cycles.
    for (uint8_t i = 0; i < 4 * 10; ++i)
        osc1_cycle();
    negate_reset();
    Signals::resetCycles();

    // Read reset vector
    cycle();  // hi(vector)
    cycle();  // lo(vector)
    cycle();  // dummy cycle
    prepareCycle();

    // Disable COP, enable IRV and ILRV of CNFGR; reset value 0x63
    constexpr auto CNFGR = 0x19;
    _regs.internal_write(CNFGR, 0x5B);

    // We should certainly inject SWI by pointing external address here.
    _regs.save();
    // Restore reset vector
    _mems.raw_write16(vec_reset, vector);
    _regs.setIp(vector);
}

void PinsMc68HC05C0::idle() {
    // MC68HC05C0 is fully static, so we can stop clock safely.
}

mc6805::Signals *PinsMc68HC05C0::currCycle() const {
    auto s = Signals::put();
    s->getControl();
    s->getAddr();
    return s;
}

mc6805::Signals *PinsMc68HC05C0::rawPrepareCycle() const {
    auto s = Signals::put();
    while (signal_as() == LOW) {
        osc1_cycle_hi();
        delayNanoseconds(osc1_hi_as);
    }
    // c2
    while (signal_as() != LOW) {
        osc1_cycle_hi();
        assert_debug();
        s->getAddr();
        negate_debug();
        delayNanoseconds(osc1_hi_addr);
    }
    // c3
    osc1_lo();
    delayNanoseconds(osc1_lo_control);
    assert_debug();
    s->getControl();
    negate_debug();
    return s;
}

mc6805::Signals *PinsMc68HC05C0::prepareCycle() const {
    auto s = rawPrepareCycle();
    delayNanoseconds(osc1_lo_ns);
    return s;
}

mc6805::Signals *PinsMc68HC05C0::completeCycle(mc6805::Signals *signals) const {
    // c3
    osc1_hi();
    auto s = static_cast<Signals *>(signals);
    if (s->write()) {
        delayNanoseconds(osc1_hi_write);
        assert_debug();
        s->getData();
        negate_debug();
        // c4
        osc1_lo();
        if (s->writeMemory()) {
            _mems.write(s->addr, s->data);
        } else {
            delayNanoseconds(osc1_lo_capture);
        }
    } else if (_mems.is_internal(s->addr)) {
        // IRV is enabled and an internal read appears on the external bus
        delayNanoseconds(osc1_hi_write);
        s->getData();
        osc1_lo();
        delayNanoseconds(osc1_lo_ns);
    } else {
        if (s->readMemory()) {
            s->data = _mems.read(s->addr);
        } else {
            delayNanoseconds(osc1_hi_inject);
        }
        // c4
        osc1_lo();
        assert_debug();
        busWrite(AD, s->data);
        busMode(AD, OUTPUT);
        busMode(AD, INPUT);
        negate_debug();
    }
    // c4
    osc1_hi();
    delayNanoseconds(osc1_hi_ns);
    Signals::nextCycle();
    return s;
}

}  // namespace mc68hc05c0
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
