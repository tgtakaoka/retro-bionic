#include "pins_mc6809.h"
#include "debugger.h"
#include "devs_mc6809.h"
#include "inst_hd6309.h"
#include "mems_mc6809.h"
#include "regs_mc6809.h"
#include "signals_mc6809.h"

namespace debugger {
namespace mc6809 {

/**
 * MC6809 bus cycle.
 *       _    __    __    __    __    __    __    __    __    _
 * EXTAL  |_c|1 |_c|2 |_c|3 |_c|4 |_c|1 |_c|2 |_c|3 |_c|4 |_c|1
 *        \     \_____\_____\     \     \ ____\______\    \
 *     Q __|____|      |     |_____|_____|     |      |____|___
 *       __|           |___________|           |___________|
 *     E   |___________|           |___________|           |___
 *       \     ____________________\                       \
 *   R/W _>---<                     |_______________________>-----
 *                           ____               _________
 *  Data -------------------<rrrr>-------------<wwwwwwwww>-------
 *       _________       _________________       _________________
 *  Addr _________>-----<_________________>-----<_________________>
 */

namespace {

// tCYC: min 1,000 ns ; E/Q cycle
// PWEL: min   430 ns ; e_lo
// PWEH: min   450 ns ; e_hi
// PWQL: min   450 ns ; q_lo
// PWQH: min   430 ns ; q_hi
//  tEQ: min   200 ns ; e to q
//  tAQ: max   200 ns ; addr/ba/bs/rw to q_hi
// tDDQ: max   200 ns ; q_hi to write data
// tDSR: min    80 ns ; data to e_lo
// tPCS: min   200 ns ; reset_hi to q_lo
constexpr auto extal_hi_ns = 105;   // 125
constexpr auto extal_lo_ns = 89;    // 125
constexpr auto c1_lo_ns = 28;       // 125
constexpr auto c1_hi_ns = 66;       // 125
constexpr auto c2_lo_ns = 50;       // 125
constexpr auto c2_hi_ns = 76;       // 125
constexpr auto c3_lo_write = 72;    // 125
constexpr auto c3_hi_write = 78;    // 125
constexpr auto c4_lo_write = 1;     // 125
constexpr auto c4_lo_capture = 85;  // 125
constexpr auto c4_hi_write = 52;    // 125
constexpr auto c3_lo_read = 70;     // 125
constexpr auto c3_hi_read = 7;      // 125
constexpr auto c3_hi_inject = 80;   // 125
constexpr auto c4_lo_read = 50;     // 125
constexpr auto c4_hi_read = 52;     // 125

inline void extal_hi() {
    digitalWriteFast(PIN_EXTAL, HIGH);
}

inline void extal_lo() {
    digitalWriteFast(PIN_EXTAL, LOW);
}

inline auto clock_q() {
    return digitalReadFast(PIN_Q);
}

inline auto clock_e() {
    return digitalReadFast(PIN_E);
}

void negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
}

constexpr uint8_t PINS_LOW[] = {
        PIN_EXTAL,
        PIN_RESET,
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_HALT,
        PIN_IRQ,
        PIN_NMI,
        PIN_FIRQ,
        PIN_BREQ,
        PIN_MRDY,
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
        PIN_Q,
        PIN_E,
        PIN_BS,
        PIN_BA,
        PIN_XTAL,
};

inline void extal_cycle() {
    extal_hi();
    delayNanoseconds(extal_hi_ns);
    extal_lo();
    delayNanoseconds(extal_lo_ns);
}

}  // namespace

PinsMc6809::PinsMc6809() {
    _regs = new RegsMc6809(this);
    _devs = new DevsMc6809();
    _mems = new MemsMc6809(_devs);
    _inst = new hd6309::InstHd6309(_mems);
}

void PinsMc6809::resetPins() {
    // Assert reset condition
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    // Synchronize EXTAL input and Q and E output
    while (true) {
        extal_cycle();
        // Synchoronize to C4L
        if (clock_q() == LOW && clock_e() != LOW)
            break;
    }
    // C4H
    extal_hi();
    delayNanoseconds(extal_hi_ns);
    // C1L
    extal_lo();
    // At least one #RESET cycle.
    for (auto i = 0; i < 3; ++i)
        cycle();
    negate_reset();

    PinsMc6809Base::resetPins();
}

Signals *PinsMc6809::rawCycle() const {
    // C1H
    extal_hi();
    Signals::inputMode();
    auto s = Signals::put();
    delayNanoseconds(c1_hi_ns);
    // C2L
    extal_lo();
    delayNanoseconds(c2_lo_ns);
    s->getHighAddr();
    // C2H
    extal_hi();
    s->getDirection();
    delayNanoseconds(c2_hi_ns);
    // C3L
    extal_lo();
    s->getLowAddr();
    if (s->write()) {
        delayNanoseconds(c3_lo_write);
        // C3H
        extal_hi();
        s->getData();
        delayNanoseconds(c3_hi_write);
        // C4L
        extal_lo();
        if (s->writeMemory()) {
            _mems->write(s->addr, s->data);
            if (c4_lo_write)
                delayNanoseconds(c4_lo_write);
        } else {
            delayNanoseconds(c4_lo_capture);
        }
        // C4H
        extal_hi();
        Signals::nextCycle();
        delayNanoseconds(c4_hi_write);
    } else {
        delayNanoseconds(c3_lo_read);
        // C3H
        extal_hi();
        if (s->readMemory()) {
            s->data = _mems->read(s->addr);
            delayNanoseconds(c3_hi_read);
        } else {
            delayNanoseconds(c3_hi_inject);
        }
        // C4L
        extal_lo();
        s->outData();
        delayNanoseconds(c4_lo_read);
        // C4H
        extal_hi();
        Signals::nextCycle();
        delayNanoseconds(c4_hi_read);
    }
    s->getControl();
    // C1L
    extal_lo();

    return s;
}

Signals *PinsMc6809::cycle() const {
    delayNanoseconds(c1_lo_ns);
    return rawCycle();
}

}  // namespace mc6809
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
