#include "pins_mc68hc05c0.h"
#include "debugger.h"
#include "devs_mc6805.h"
#include "digital_bus.h"
#include "inst_mc68hc05.h"
#include "mems_mc68hc05c0.h"
#include "regs_mc68hc05c0.h"

namespace debugger {
namespace mc68hc05c0 {

struct PinsMc68HC05C0 Pins{Regs, mc68hc05::Inst, Memory, mc6805::Devices};

/**
 * MC68HC05C0 bus cycle.
 *       |--c1-|--c2-|--c3-|--c4-|--c1-|--c2-|--c3-|--c4-|--c1-|
 *      _|   __|   __|   __|   __|   __|   __|   __|   __|   __|
 * OSC1  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|
 *          \ ____\  \           \  \ ____\  \           \  \ _
 *   AS _____|     |_________________|     |_________________|
 *      ______________             ____________________________
 *  #RD               |___________|
 *
 *   LI
 *      ______________________________________             ____
 *  #WR                                       |___________|
 *      ________ _______________________ ____________________ __
 *m  AH ________X_______________________X____________________X__
 *               _____         ____      _____   _________
 *   AD --------<__AL_>-------<_in_>----<__AL_>-<___out___>-----
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

inline auto signal_rd() {
    return digitalReadFast(PIN_RD);
}

inline auto signal_wr() {
    return digitalReadFast(PIN_WR);
}

void negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
}

constexpr uint8_t PINS_LOW[] = {
        PIN_OSC1,
        PIN_RESET,
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
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    // #LIR/MODE=H; select multiplexed bus mode.
    pinsMode(PINS_PULLUP, sizeof(PINS_PULLUP), INPUT_PULLUP);

    // #RESET must stay low for a minimum of one tRL (1.5usec at VDD=5V).
    for (auto i = 0; i < 15 * 5; i++)
        clock_cycle();

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

void PinsMc68HC05C0::idle() {
    // TODO
    ;
}

mc6805::Signals *PinsMc68HC05C0::currCycle() const {
    auto s = Signals::put();
    s->getDirection();
    s->getAddr();
    s->getLoadInstruction();
    return s;
}

mc6805::Signals *PinsMc68HC05C0::rawPrepareCycle() const {
    auto s = Signals::put();
    return s;
}

mc6805::Signals *PinsMc68HC05C0::prepareCycle() const {
    return rawPrepareCycle();
}

mc6805::Signals *PinsMc68HC05C0::completeCycle(mc6805::Signals *signals) const {
    auto s = static_cast<Signals *>(signals);
    if (s->write()) {
        s->getData();
        if (s->writeMemory()) {
            _mems.write(s->addr, s->data);
        } else {
            ;  // capture
        }
    } else {
        if (s->readMemory()) {
            s->data = _mems.read(s->addr);
        } else {
            ;  // inject
        }
        busWrite(AD, s->data);
        busMode(AD, OUTPUT);
        busMode(AD, INPUT);
    }
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
