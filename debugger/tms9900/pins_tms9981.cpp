#include "pins_tms9981.h"
#include "debugger.h"
#include "devs_tms9900.h"
#include "digital_bus.h"
#include "mems_tms9900.h"
#include "regs_tms9900.h"
#include "signals_tms9981.h"

namespace debugger {
namespace tms9981 {

// clang-format off
/**
 * TMS9981 bus cycle
 *    PHI  |  1  |  2  |  3  |  4  |  1  |  2  |  3  |  4  |  1  |  2  |  3  |  4 |
 *            __    __    __    __    __    __    __    __    __    __    __    __
 *   CKIN |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|
 *        ______\_____\     \ ____\___________\     \ ____\_____\_____\     \ ____
 *  #PHI3        |     |_____|     |           |_____|     |     |     |_____|
 *        _______|                 |                       |     |________________
 * #MEMEN _______\_________________|_______________________|_____/________________
 *        _______|_________________|_______________________|_____|________________
 *   DBIN _______/                 |                       |     \________________
 *        _________________________|                       |______________________
 *    #WE                          \_______________________/
 *        _______ _______________________________________________ ________________
 *   DATA _______X_______________________________________________X________________
 *                _______________________________________________
 *    IAQ _______/                                               \________________
 *        ________________________________________________________________________
 *  READY _____________________/       \__________________________________________
 */
// clang-format on

// fext: min 6 MHz, max 10 MHz
//   tw: 1/fext
//  tWH: min  40 ns ; CKIN high pulse width
//  tWL: min  40 ns ; CKIN low pulse width
//  tOV: min tw-50 ns ; output valid to #PHI3 low
//  tOX: max tw    ns ; output invalid to #PHI3 low
//  tsu: min tw-30 ns ; input data setup from #PHI3+
//   th: min 2tw+10ns ; input data hold from #PHI3+
// tPHL: max tw+20 ns ; #PHI3+ to #WE-
// tPLH: min tw    ns ; #PHI3+ to #WE+

namespace {

constexpr auto ckin_lo_ns = 40;       // 50
constexpr auto ckin_hi_ns = 15;       // 50
constexpr auto ckin_lo_laddr = 0;     // 50
constexpr auto ckin_hi_maddr = 5;     // 50
constexpr auto ckin_lo_haddr = 2;     // 50
constexpr auto ckin_lo_phi1 = 1;      // 50
constexpr auto ckin_lo_inject = 30;   // 50
constexpr auto ckin_lo_get = 5;       // 50
constexpr auto ckin_hi_capture = 30;  // 50
constexpr auto ckin_lo_next = 0;      // 50
constexpr auto ckin_hi_input = 1;     // 50

const uint8_t PINS_LOW[] = {
        PIN_CKIN,
        PIN_INT2,  // reset
        PIN_INT1,  // reset
        PIN_INT0,  // reset
        PIN_READY,
        PIN_CRUCLK,
        PIN_CRUIN,
};

const uint8_t PINS_HIGH[] = {
        PIN_HOLD,
};

const uint8_t PINS_INPUT[] = {
        PIN_PHI3,
        PIN_MEMEN,
        PIN_DBIN,
        PIN_WE,
        PIN_IAQ,
        PIN_HOLDA,
        PIN_D7,
        PIN_D6,
        PIN_D5,
        PIN_D4,
        PIN_D3,
        PIN_D2,
        PIN_D1,
        PIN_D0,
        PIN_AL13,
        PIN_AL12,
        PIN_AL11,
        PIN_AL10,
        PIN_AL9,
        PIN_AL8,
        PIN_AL7,
        PIN_AL6,
        PIN_AM5,
        PIN_AM4,
        PIN_AM3,
        PIN_AM2,
        PIN_AH1,
        PIN_AH0,
};

inline void ckin_lo() {
    digitalWriteFast(PIN_CKIN, LOW);
}

inline void ckin_hi() {
    digitalWriteFast(PIN_CKIN, HIGH);
}

auto signal_phi3() {
    return digitalReadFast(PIN_PHI3);
}

void ckin_cycle_hi() {
    ckin_lo();
    delayNanoseconds(ckin_lo_ns);
    ckin_hi();
}

void ckin_cycle() {
    ckin_cycle_hi();
    delayNanoseconds(ckin_hi_ns);
}

void system_cycle() {
    ckin_cycle();
    ckin_cycle();
    ckin_cycle();
    ckin_cycle();
}

void negate_reset() {
    busWrite(INT, 7);
}

void assert_ready() {
    digitalWriteFast(PIN_READY, HIGH);
}

auto ready_asserted() {
    return digitalReadFast(PIN_READY) != LOW;
}

}  // namespace

PinsTms9981::PinsTms9981() {
    _devs = new tms9900::DevsTms9900();
    _mems = new tms9900::MemsTms9900(14, _devs);
    _regs = new tms9900::RegsTms9900(this, _mems);
}

void PinsTms9981::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    // Synchronize CKIN to #PHI3
    while (signal_phi3() != LOW)
        ckin_cycle();
    ckin_cycle();  // phi4

    for (auto i = 0; i < 10; ++i)
        system_cycle();
    negate_reset();
    system_cycle();

    Signals::resetCycles();
    prepareCycle();
    _regs->save();
    _regs->reset();
}

tms9900::Signals *PinsTms9981::prepareCycle() const {
    auto s = Signals::put();
    // phi1
    ckin_cycle();
    // phi2
    ckin_cycle();
    // phi3
    ckin_lo();
    s->getLowAddr();
    delayNanoseconds(ckin_lo_laddr);
    ckin_hi();
    s->getMidAddr();
    delayNanoseconds(ckin_hi_maddr);
    // phi4
    ckin_lo();
    s->getHighAddr();
    delayNanoseconds(ckin_lo_haddr);
    ckin_hi();
    return s;
}

tms9900::Signals *PinsTms9981::completeCycle(tms9900::Signals *_s) const {
    auto s = static_cast<Signals *>(_s);
    // phi1
    ckin_lo();
    delayNanoseconds(ckin_lo_phi1);
    s->getControl();
    ckin_hi();
    if (ready_asserted() && s->memory()) {
        if (s->read()) {
            // phi2
            ckin_lo();
            if (s->readMemory()) {
                s->data = _mems->read(s->addr);
            } else {
                delayNanoseconds(ckin_lo_inject);
            }
            ckin_hi();
            s->outData();
        } else {
            // phi2
            ckin_lo();
            delayNanoseconds(ckin_lo_get);
            s->getData();
            ckin_hi();
            if (s->writeMemory()) {
                _mems->write(s->addr, s->data);
            } else {
                delayNanoseconds(ckin_hi_capture);
            }
        }
        // phi3
        ckin_lo();
        Signals::nextCycle();
        delayNanoseconds(ckin_lo_next);
        ckin_hi();
        delayNanoseconds(ckin_hi_input);
        s->inputMode();
    } else {
        // phi2
        ckin_cycle();
        // phi3
        ckin_cycle();
    }
    // phi4
    ckin_cycle_hi();
    return s;
}

tms9900::Signals *PinsTms9981::resumeCycle(uint16_t) const {
    auto s = Signals::put();
    s->getLowAddr();
    s->getMidAddr();
    s->getHighAddr();
    s->getControl();
    assert_ready();
    return s;
}

void PinsTms9981::injectReads(const uint16_t *data, uint_fast8_t len) {
    auto s = resumeCycle();
    auto high = true;
    for (uint_fast8_t i = 0; i < len;) {
        s->inject(high ? hi(data[i]) : lo(data[i]));
        completeCycle(s);
        if (s->read()) {
            high = !high;
            if (high)
                i++;
        }
        s = (i < len) ? prepareCycle() : pauseCycle();
    }
}

void PinsTms9981::captureWrites(uint16_t *buf, uint_fast8_t len) {
    auto s = resumeCycle();
    auto high = true;
    for (uint_fast8_t i = 0; i < len;) {
        completeCycle(s->capture());
        if (s->write()) {
            if (high) {
                buf[i] = uint16(s->data, 0);
            } else {
                buf[i] |= s->data;
            }
            high = !high;
            if (high)
                i++;
        }
        s = (i < len) ? prepareCycle() : pauseCycle();
    }
}

void PinsTms9981::assertInt(uint8_t name_) {
    const auto name = static_cast<tms9900::IntrName>(name_);
    if (name == tms9900::INTR_NMI) {
        busWrite(INT, 2);
    } else {
        busWrite(INT, (name_ / 4) + 2);
    }
}

void PinsTms9981::negateInt(uint8_t) {
    busWrite(INT, 7);
}

}  // namespace tms9981
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
