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
//  tsu: min tw-30 ns ; input setup to #PHI3+, READY too
//   th: min 2tw+10ns ; input hold from #PHI3+, READY too
// tPHL: max tw+20 ns ; #PHI3+ to #WE-
// tPLH: min tw    ns ; #PHI3+ to #WE+

// READY is sampled at each #PHI3+ of a memory cycle but its first (Figure
// 9b): low inserts a wait clock with the outputs held.

// CKIN runs free from the PWM, and READY, low but to end a cycle, holds
// the CPU in each memory cycle until it is served. A cycle starts at a
// #PHI3 fall; it is served while the CPU waits. READY changes only past
// the hold after a #PHI3 rise: asserted there, it is sampled by the next
// rise, which latches read data, and negated after that rise's hold, well
// before the next cycle's first sample two clocks on.

namespace {

constexpr uint32_t ckin_hz = 10000000;  // 150 MHz / 15: halves of 47/53 ns
constexpr uint32_t ckin_min_hz = 6000000;
constexpr auto ckin_half_min_ns = 40;
constexpr auto tw_ns = 1000000000 / ckin_hz;
constexpr auto ready_hold_ns = 2 * tw_ns + 10;
// Asserted between the hold and the setup, tw-30, for the next rise; the
// edge wait and the delay add some 80 ns to this.
constexpr auto ready_assert_ns = 170;
// Longer than any clock: no edge means no clock or no chip.
constexpr auto edge_timeout_ns = 16 * tw_ns;
constexpr auto reset_clocks = 10;

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

static_assert(PIN_PHI3 == PIN_TIMING, "#PHI3 is the shared timing pin");

void negate_reset() {
    busWrite(INT, 7);
}

void assert_ready() {
    digitalWriteFast(PIN_READY, HIGH);
}

void negate_ready() {
    digitalWriteFast(PIN_READY, LOW);
}

}  // namespace

PinsTms9980::PinsTms9980() : _clock(ckin_half_min_ns, ckin_min_hz) {
    _devs = new tms9900::DevsTms9900();
    _mems = new MemsTms9980(_devs);
    _regs = new RegsTms9980(this, _mems);
}

void PinsTms9980::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    _clock.begin(ckin_hz);
    _clock.start();

    delayNanoseconds(reset_clocks * 4 * tw_ns);
    negate_reset();

    Cycles::reset();
    prepareCycle();
    _regs->save();
    _regs->reset();
}

// The clock runs by itself.
void PinsTms9980::idle() {}

// Masked so the outputs are read right after the #PHI3 fall: a non-memory
// clock holds them only until the next.
Signals *PinsTms9980::prepareCycle() {
    auto s = SignalsTms9980::put();
    noInterrupts();
    waitEdge(false, edge_timeout_ns);
    // assert_debug();
    s->getLowAddr();
    s->getHighAddr();
    s->getControl();
    // negate_debug();
    interrupts();
    return s;
}

// Write data is valid with the address. Read data is held past the #PHI3
// rise that latches it, then released to the bus holder.
Signals *PinsTms9980::completeCycle(Signals *_s) {
    auto s = static_cast<SignalsTms9980 *>(_s);
    if (!s->memory())
        return s;
    const auto read = s->read();
    if (read) {
        if (s->readMemory())
            s->data = _mems->read(s->addr);
        s->outData();
    } else {
        s->getData();
        if (s->writeMemory())
            _mems->write(s->addr, s->data);
    }
    noInterrupts();
    waitEdge(true, edge_timeout_ns);
    delayNanoseconds(ready_assert_ns);
    assert_ready();
    waitEdge(true, edge_timeout_ns);
    delayNanoseconds(ready_hold_ns);
    if (read)
        s->inputMode();
    negate_ready();
    interrupts();
    Cycles::next();
    return s;
}

// Read again at a #PHI3 fall: parked anywhere but a memory cycle, the CPU
// has run on.
Signals *PinsTms9980::resumeCycle(uint16_t) {
    return prepareCycle();
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
