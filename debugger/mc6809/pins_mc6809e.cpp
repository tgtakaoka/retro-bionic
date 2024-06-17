#include "pins_mc6809e.h"
#include "debugger.h"
#include "devs_mc6809.h"
#include "digital_bus.h"
#include "inst_mc6809.h"
#include "mems_mc6809.h"
#include "regs_mc6809.h"
#include "signals_mc6809e.h"

namespace debugger {
namespace mc6809e {

/**
 * MC6809E bus cycle.
 *               ___________             ___________
 *     Q _______|c2         |c4_________|c2         |c4_________|
 *              \      ___________      \      ___________
 *     E |c1__________|c3         |c1_________|c3         |c1____
 *       \      __________________\                       \
 *   R/W _>----<                   |_______________________>-----
 *                           ____               _________
 *  Data -------------------<rrrr>-------------<wwwwwwwww>-------
 *       _________      _________________       _________________
 *  AVMA _________>----<_________________>-----<_________________>
 */

namespace {

// tCYC: min 1,000 ns ; E/Q cycle
// PWEL: min   450 ns ; e_lo
// PWEH: min   450 ns ; e_hi
// PWQH: min   450 ns ; q_hi
//  tEQ: min   200 ns ; e to q
//  tAD: max   200 ns ; e_lo to addr/ba/bs/rw
//  tCD: max   300 ns ; q_hi to avma/lic/busy
// tDDQ: max   200 ns ; q_hi to write data
// tDSR: min    80 ns ; data to e_lo
// tPCS: min   200 ns ; reset_hi to q_lo
constexpr auto cycle_ns = 70;     // 250 EQ=LL
constexpr auto c1_ns = 70;        // 250 EQ=LL
constexpr auto c2_ns = 156;       // 250 EQ=LH
constexpr auto c3_novma = 180;    // 250 EQ=HH
constexpr auto c4_novma = 132;    // 250 EQ=HL
constexpr auto c3_write = 162;    // 250 EQ=HH
constexpr auto c4_write = 44;     // 250 EQ=HL
constexpr auto c4_capture = 130;  // 250 EQ=HL
constexpr auto c3_read = 90;      // 250 EQ=HH
constexpr auto c3_inject = 188;   // 250 EQ=HH
constexpr auto c4_read = 94;      // 250 EQ=HL

inline void c1_clock() {
    digitalWriteFast(PIN_E, LOW);
}

inline void c2_clock() {
    digitalWriteFast(PIN_Q, HIGH);
}

inline void c3_clock() {
    digitalWriteFast(PIN_E, HIGH);
}

inline void c4_clock() {
    digitalWriteFast(PIN_Q, LOW);
}

inline auto signal_avma() {
    return digitalReadFast(PIN_AVMA);
}

void negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
}

constexpr uint8_t PINS_LOW[] = {
        PIN_E,
        PIN_Q,
        PIN_RESET,
        PIN_TSC,
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_HALT,
        PIN_IRQ,
        PIN_NMI,
        PIN_FIRQ,
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
        PIN_BS,
        PIN_BA,
        PIN_LIC,
        PIN_AVMA,
        PIN_BUSY,
};

}  // namespace

void PinsMc6809E::resetPins() {
    // Assert reset condition
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    // At least one #RESET cycle.
    for (auto i = 0; i < 5; i++)
        cycle();
    negate_reset();
}

mc6809::Signals *PinsMc6809E::rawCycle() const {
    static uint8_t vma_next = LOW;
    // c1
    busMode(D, INPUT);
    auto s = Signals::put();
    delayNanoseconds(c1_ns);
    // c2
    c2_clock();
    delayNanoseconds(c2_ns);
    s->getDirection();
    s->getHighAddr();
    // c3
    c3_clock();
    s->getLowAddr();
    if (vma_next == LOW) {
        delayNanoseconds(c3_novma);
        // c4
        c4_clock();
        Signals::nextCycle();
        delayNanoseconds(c4_novma);
    } else if (s->write()) {
        delayNanoseconds(c3_write);
        s->getData();
        // c4
        c4_clock();
        if (s->writeMemory()) {
            _mems.write(s->addr, s->data);
            delayNanoseconds(c4_write);
        } else {
            delayNanoseconds(c4_capture);
        }
        Signals::nextCycle();
    } else {
        if (s->readMemory()) {
            s->data = _mems.read(s->addr);
            delayNanoseconds(c3_read);
        } else {
            delayNanoseconds(c3_inject);
        }
        // c4
        c4_clock();
        busMode(D, OUTPUT);
        busWrite(D, s->data);
        Signals::nextCycle();
        delayNanoseconds(c4_read);
    }
    s->getControl();
    vma_next = signal_avma();
    // c1
    c1_clock();

    return s;
}

mc6809::Signals *PinsMc6809E::cycle() const {
    delayNanoseconds(cycle_ns);
    return rawCycle();
}

const mc6809::Signals *PinsMc6809E::findFetch(
        mc6809::Signals *begin, const mc6809::Signals *end) {
    const auto native6309 = _regs.contextLength() == 14;
    const auto cycles = begin->diff(end);
    if (!native6309) {
        for (uint8_t i = 1; i <= cycles; ++i) {
            auto s = begin->next(cycles - i);
            if (s->fetch()) {
                s->clearFetch();
                s->next()->markFetch(1);
            }
        }
    }
    for (uint8_t i = 0; i < cycles; ++i) {
        const auto s = begin->next(i);
        if (s->fetch())
            return s;
    }
    return end;
}

}  // namespace mc6809e
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
