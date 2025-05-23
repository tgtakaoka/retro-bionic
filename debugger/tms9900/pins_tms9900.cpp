#include "pins_tms9900.h"
#include "debugger.h"
#include "devs_tms9900.h"
#include "digital_bus.h"
#include "mems_tms9900.h"
#include "regs_tms9900.h"
#include "signals_tms9900.h"

#define DEBUG(e)
// #define DEBUG(e) e

namespace debugger {
namespace tms9900 {

// clang-format off
/**
 * TMS9990 bus cycle
 *         __          __          __          __          __          __
 *  PHI1 _|  |________|  |________|  |________|  |________|  |________|  |____
 *            __      v   __          __          __      |   __      |   __
 *  PHI2 ____|  |________|  |________|  |________|  |_____|__|  |_____|__|  |_
 *           |   __          __      |   __      |   __   |      __   |  |   _
 *  PHI3 ____|__|  |________|  |_____|__|  |_____|__|  |__|_____|  |__|__|__|
 *       _   |      __          __   |      __   |      __|         __|  |
 *  PHI4  |__|_____|  |________|  |__|_ ___|  |__|_____|  |________|  |__|____
 *       ____|                       |___________|        |           |  |____
 * MEMEN ____\_______________________/           \________|___________|__/
 *       ____________________________                     |           |
 *  DBIN ____/                       \____________________|___________|_______
 *       _________________________________________________|           |_______
 *    WE                                                   \__________/
 *       ____ __________________ ____ ______________ ____________________ ____
 *  DATA ____X__________________X_in_X______________X_______valid________X____
 *            _______________________
 *   IAQ ____/                       \________________________________________
 *       _____________v___________________________________v___________________
 * READY ____________/     \_____________________________/      \_____________
 */
// clang-format on

// fext: min 12MHz, max 24 MHz
//  tc1: min  41.25 ns ; 1/fext
//  tc2: min 165 ns ; 4tc1 cycle of CLKOUT
//  tWH: min  82 ns ; CLKOUT high pulse width
//  tWL: min  82 ns ; CLKOUT low pulse width

namespace {

// constexpr auto phi_hi_ns = 23;  // (2MHz)
// constexpr auto phi_hi_ns = 110;  // (1MHz)
constexpr auto phi_hi_ns = 23;
constexpr auto clk_lo_ns = 0;
constexpr auto phi3_addr = 0;
constexpr auto phi4_cntl = 23;
constexpr auto phi1_read = 0;
constexpr auto phi1_inject = 23;
constexpr auto phi2_read = 0;
constexpr auto phi3_read = 0;
constexpr auto phi1_write = 23;
constexpr auto phi2_write = 0;
constexpr auto phi3_write = 0;
constexpr auto phi3_capture = 23;

const uint8_t PINS_LOW[] = {
        PIN_RESET,
        PIN_READY,
        PIN_CRUCLK,
        PIN_SEL0,
        PIN_SEL1,
        PIN_SEL2,
        PIN_DATA,
        PIN_PHICLK,
};

const uint8_t PINS_HIGH[] = {
        PIN_OUTCLK,
};

const uint8_t PINS_INPUT[] = {
        PIN_MEMEN,
        PIN_DBIN,
        PIN_WE,
        PIN_IAQ,
        PIN_D15,
        PIN_D14,
        PIN_D13,
        PIN_D12,
        PIN_D11,
        PIN_D10,
        PIN_D9,
        PIN_D8,
        PIN_D7,
        PIN_D6,
        PIN_D5,
        PIN_D4,
        PIN_D3,
        PIN_D2,
        PIN_D1,
        PIN_D0,
        PIN_CRUOUT,
        PIN_ADR14,
        PIN_ADR13,
        PIN_ADR12,
        PIN_ADR11,
        PIN_ADR10,
        PIN_ADR9,
        PIN_ADR8,
};

void phi_clock() {
    digitalWriteFast(PIN_PHICLK, HIGH);
    delayNanoseconds(clk_lo_ns);
    digitalWriteFast(PIN_PHICLK, LOW);
}

void phi1_hi() {
    digitalWriteFast(PIN_DATA, HIGH);
    phi_clock();
}

void phi2_hi() {
    digitalWriteFast(PIN_DATA, LOW);
    phi_clock();
    phi_clock();
}

void phi3_hi() {
    digitalWriteFast(PIN_DATA, LOW);
    phi_clock();
    phi_clock();
}

void phi4_hi() {
    digitalWriteFast(PIN_DATA, LOW);
    phi_clock();
    phi_clock();
}

void phi4_lo() {
    digitalWriteFast(PIN_DATA, LOW);
    phi_clock();
}

void system_cycle() {
    phi1_hi();
    delayNanoseconds(phi_hi_ns);
    phi2_hi();
    delayNanoseconds(phi_hi_ns);
    phi3_hi();
    delayNanoseconds(phi_hi_ns);
    phi4_hi();
    delayNanoseconds(phi_hi_ns);
    phi4_lo();
}

void out_clock() {
    delayNanoseconds(clk_lo_ns);
    digitalWriteFast(PIN_OUTCLK, LOW);
    delayNanoseconds(clk_lo_ns);
    digitalWriteFast(PIN_OUTCLK, HIGH);
}

void control_low(uint_fast8_t sel) {
    busWrite(SEL, sel);
    digitalWriteFast(PIN_DATA, LOW);
    out_clock();
}

void control_high(uint_fast8_t sel) {
    busWrite(SEL, sel);
    digitalWriteFast(PIN_DATA, HIGH);
    out_clock();
}

void negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
}

void assert_ready() {
    digitalWriteFast(PIN_READY, HIGH);
}

void negate_load() {
    control_high(SEL_LOAD);
}

void assert_load() {
    control_low(SEL_LOAD);
}

void negate_intreq() {
    control_high(SEL_INTREQ);
}

void assert_intreq() {
    control_low(SEL_INTREQ);
}

void negate_hold() {
    control_high(SEL_HOLD);
}

auto memen_asserted() {
    return digitalReadFast(PIN_MEMEN) == LOW;
}

}  // namespace

PinsTms9900::PinsTms9900() {
    _devs = new DevsTms9900();
    _mems = new MemsTms9900(_devs);
    _regs = new RegsTms9900(this, _mems);
}

void PinsTms9900::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    negate_hold();
    negate_load();
    negate_intreq();

    for (auto i = 0; i < 10; ++i)
        system_cycle();
    negate_reset();
    while (!memen_asserted()) {
        system_cycle();
    }

    Signals::resetCycles();
    pauseCycle();
    _regs->save();
    _regs->reset();
}

Signals *PinsTms9900::prepareCycle() {
    auto s = SignalsTms9900::put();
    // phi1
    phi1_hi();
    delayNanoseconds(phi_hi_ns);
    // phi2
    phi2_hi();
    delayNanoseconds(phi_hi_ns);
    // phi3
    phi3_hi();
    // assert_debug();
    s->getAddress();
    if (phi3_addr)
        delayNanoseconds(phi3_addr);
    // phi4
    phi4_hi();
    if (phi4_cntl)
        delayNanoseconds(phi4_cntl);
    s->getControl();
    // negate_debug();
    phi4_lo();
    return s;
}

Signals *PinsTms9900::completeCycle(Signals *_s) {
    auto s = static_cast<SignalsTms9900 *>(_s);
    // phi1
    phi1_hi();
    if (s->memory()) {
        if (s->read()) {
            if (s->readMemory()) {
                auto m = mems<MemsTms9900>();
                s->data = m->read(s->addr);
                if (phi1_read)
                    delayNanoseconds(phi1_read);
            } else {
                delayNanoseconds(phi1_inject);
            }
            // phi2
            phi2_hi();
            // assert_debug();
            s->outData();
            // negate_debug();
            if (phi2_read)
                delayNanoseconds(phi2_read);
            // phi3
            phi3_hi();
            if (phi3_read)
                delayNanoseconds(phi3_read);
            Signals::nextCycle();
            s->inputMode();
        } else {
            delayNanoseconds(phi1_write);
            // phi2
            phi2_hi();
            if (phi2_write)
                delayNanoseconds(phi2_write);
            Signals::nextCycle();
            // assert_debug();
            s->getData();
            // negate_debug();
            // phi3
            phi3_hi();
            if (s->writeMemory()) {
                auto m = mems<MemsTms9900>();
                m->write(s->addr, s->data);
                if (phi3_write)
                    delayNanoseconds(phi3_write);
            } else {
                delayNanoseconds(phi3_capture);
            }
        }
    } else {
        delayNanoseconds(phi_hi_ns);
        // phi2
        phi2_hi();
        delayNanoseconds(phi_hi_ns);
        // phi3
        phi3_hi();
        delayNanoseconds(phi_hi_ns);
    }
    // phi4
    phi4_hi();
    delayNanoseconds(phi_hi_ns);
    phi4_lo();
    return s;
}

Signals *PinsTms9900::resumeCycle(uint16_t pc) {
    auto s = SignalsTms9900::put();
    s->getAddress();
    s->getControl();
    assert_ready();
    return s;
}

void PinsTms9900::injectReads(const uint16_t *data, uint_fast8_t len) {
    auto s = resumeCycle();
    for (uint_fast8_t i = 0; i < len;) {
        completeCycle(s->inject(data[i]));
        if (s->read()) {
            i++;
        }
        s = (i < len) ? prepareCycle() : pauseCycle();
    }
}

void PinsTms9900::captureWrites(uint16_t *buf, uint_fast8_t len) {
    auto s = resumeCycle();
    for (uint_fast8_t i = 0; i < len;) {
        completeCycle(s->capture());
        if (s->write()) {
            buf[i++] = s->data;
        }
        s = (i < len) ? prepareCycle() : pauseCycle();
    }
}

void set_ic(uint_fast8_t ic) {
    if (ic & 0x01) {
        control_high(SEL_IC3);
    } else {
        control_low(SEL_IC3);
    }
    if (ic & 0x02) {
        control_high(SEL_IC2);
    } else {
        control_low(SEL_IC2);
    }
    if (ic & 0x04) {
        control_high(SEL_IC1);
    } else {
        control_low(SEL_IC1);
    }
    if (ic & 0x08) {
        control_high(SEL_IC0);
    } else {
        control_low(SEL_IC0);
    }
}

void PinsTms9900::assertInt(uint8_t name_) {
    const auto name = static_cast<tms9900::IntrName>(name_);
    if (name == tms9900::INTR_NMI) {
        assert_load();
    } else {
        const auto ic = (name_ / 4);
        set_ic(ic);
        assert_intreq();
    }
}

void PinsTms9900::negateInt(uint8_t name_) {
    const auto name = static_cast<tms9900::IntrName>(name_);
    if (name == tms9900::INTR_NMI) {
        negate_load();
    } else {
        negate_intreq();
    }
}

}  // namespace tms9900
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
