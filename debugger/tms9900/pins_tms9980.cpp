#include "pins_tms9980.h"
#include "debugger.h"
#include "devs_tms9900.h"
#include "digital_bus.h"
#include "mems_tms9980.h"
#include "regs_tms9980.h"
#include "signals_tms9980.h"

namespace debugger {
namespace tms9980 {

// clang-format off
/**
 * TMS9980 bus cycle
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
//       min  90 ns ; CKIN- to #PHI3-
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

// ATTENTION: two TMS9980ANLs passed and failed from run to run (step,
// arith, echo) while the TMS9981 passed everything. CKIN is held past
// the 167 ns of fext min at each bus cycle boundary, by the memory read
// in phi1 and the work between cycles; a PWM clock with READY stalls
// would keep it in spec.

namespace {

constexpr auto ckin_lo_ns = 40;       // 50
constexpr auto ckin_hi_ns = 20;       // 50
constexpr auto ckin_hi_phi1 = 40;     // 50, the delay runs short after work
constexpr auto ckin_lo_phi4 = 30;     // 50, inputMode() takes the rest
constexpr auto phi3_polls = 40;       // #PHI3 falls some 30 ns after CKIN+
constexpr auto ckin_hi_inject = 30;   // 50
constexpr auto ckin_lo_capture = 30;  // 50
constexpr auto phi3_delay_ns = 100;   // CKIN- to #PHI3- is 70-100 ns

const uint8_t PINS_LOW[] = {
        PIN_CKIN,
        PIN_INT2,  // reset
        PIN_INT1,  // reset
        PIN_INT0,  // reset
        PIN_READY,
        PIN_CRUIN,
};

const uint8_t PINS_HIGH[] = {
        PIN_HOLD,
};

const uint8_t PINS_INPUT[] = {
        PIN_CRUCLK,
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

}  // namespace

PinsTms9980::PinsTms9980() {
    _devs = new tms9900::DevsTms9900();
    _mems = new MemsTms9980(_devs);
    _regs = new RegsTms9980(this, _mems);
}

void PinsTms9980::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    system_cycle();
    // Synchronize CKIN to #PHI3, which falls some 100 ns after CKIN does:
    // reading it sooner can see the fall a cycle late, one phase off.
    noInterrupts();
    do {
        ckin_cycle();
        delayNanoseconds(phi3_delay_ns);
    } while (signal_phi3() != LOW);
    interrupts();
    ckin_cycle();  // phi4

    for (auto i = 0; i < 10; ++i)
        system_cycle();
    negate_reset();
    system_cycle();

    Cycles::reset();
    prepareCycle();
    _regs->save();
    _regs->reset();
}

namespace {
bool phi3_fell() {
    for (auto n = 0; n < phi3_polls; ++n) {
        if (signal_phi3() == LOW)
            return true;
    }
    return false;
}
}  // namespace

Signals *PinsTms9980::prepareCycle() {
    auto s = SignalsTms9980::put();
    // Masked so an interrupt can't stretch a phase between an edge and its
    // samples
    noInterrupts();
    // phi1
    ckin_cycle();
    // phi2
    ckin_cycle();
    // phi3: #PHI3 falls some 100 ns after the CKIN fall that starts it, and
    // the outputs settle some 100 ns before that. Clock on until it has
    // fallen, at most a clock, so a phase that slipped heals here.
    for (auto n = 0; n < 4; ++n) {
        ckin_lo();
        delayNanoseconds(ckin_lo_ns);
        ckin_hi();
        if (phi3_fell())
            break;
    }
    // assert_debug();
    s->getLowAddr();
    // negate_debug();
    // phi4: the outputs hold through it
    ckin_lo();
    s->getHighAddr();
    s->getControl();
    ckin_hi();
    interrupts();
    return s;
}

// Read data goes on the bus in phi1, a clock and more ahead of the #PHI3
// rise that latches it, and is released in phi4, the bus holder keeping
// it; write data is valid with the address. The work in each low phase
// takes the place of its delay.
Signals *PinsTms9980::completeCycle(Signals *_s) {
    auto s = static_cast<SignalsTms9980 *>(_s);
    noInterrupts();
    const auto read = s->memory() && s->read();
    // phi1
    ckin_lo();
    if (read) {
        if (s->readMemory()) {
            s->data = _mems->read(s->addr);
        } else {
            delayNanoseconds(ckin_hi_inject);
        }
        // assert_debug();
        s->outData();
        // negate_debug();
    } else if (s->memory()) {
        s->getData();
        if (s->writeMemory()) {
            _mems->write(s->addr, s->data);
        } else {
            delayNanoseconds(ckin_lo_capture);
        }
    } else {
        delayNanoseconds(ckin_lo_ns);
    }
    ckin_hi();
    delayNanoseconds(ckin_hi_phi1);
    // phi2
    ckin_lo();
    if (s->memory()) {
        Cycles::next();
    } else {
        delayNanoseconds(ckin_lo_ns);
    }
    ckin_hi();
    delayNanoseconds(ckin_hi_ns);
    // phi3
    ckin_cycle();
    // phi4
    ckin_lo();
    if (read)
        s->inputMode();
    delayNanoseconds(ckin_lo_phi4);
    ckin_hi();
    interrupts();
    return s;
}

Signals *PinsTms9980::resumeCycle(uint16_t) {
    auto s = SignalsTms9980::put();
    s->getLowAddr();
    s->getHighAddr();
    s->getControl();
    assert_ready();
    return s;
}

void PinsTms9980::injectReads(const uint16_t *data, uint_fast8_t len) {
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

void PinsTms9980::captureWrites(uint16_t *buf, uint_fast8_t len) {
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

// INT2-INT0 carry a code: 0 resets, 2 is LOAD (NMI), level n is n+2 and 7
// is none.
void PinsTms9980::assertInt(uint8_t name_) {
    const auto name = static_cast<tms9900::IntrName>(name_);
    if (name == tms9900::INTR_NMI) {
        busWrite(INT, 2);
    } else {
        busWrite(INT, (name_ / 4) + 2);
    }
}

void PinsTms9980::negateInt(uint8_t) {
    busWrite(INT, 7);
}

}  // namespace tms9980
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
