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
constexpr auto extal_lo_ns = 109;   // 125

// The bus cycle's delays, each in an EXTAL half of 125 ns.
enum Delay : uint8_t {
    c1_lo_ns,
    c1_hi_ns,
    c2_lo_ns,
    c2_hi_ns,
    c3_lo_write,
    c3_hi_write,
    c4_lo_write,
    c4_lo_capture,
    c4_hi_write,
    c3_lo_read,
    c3_hi_read,
    c3_hi_inject,
    c4_lo_read,
    c4_hi_read,
    DELAYS,
};

// Long enough that no E cycle runs under tCYC, nor EXTAL faster than fXTAL,
// the 1 MHz part's.
constexpr uint16_t DELAY_NS[] = {
        60,   // c1_lo_ns
        91,   // c1_hi_ns
        68,   // c2_lo_ns
        104,  // c2_hi_ns
        99,   // c3_lo_write
        107,  // c3_hi_write
        60,   // c4_lo_write
        117,  // c4_lo_capture
        71,   // c4_hi_write
        96,   // c3_lo_read
        60,   // c3_hi_read
        110,  // c3_hi_inject
        68,   // c4_lo_read
        71,   // c4_hi_read
};
// MC68B09 (2 MHz): E 1.63 MHz, EXTAL under 8 MHz on the bench.
// constexpr uint16_t DELAY_NS[] = {
//         10,  // c1_lo_ns
//         24,  // c1_hi_ns
//         18,  // c2_lo_ns
//         27,  // c2_hi_ns
//         26,  // c3_lo_write
//         28,  // c3_hi_write
//         1,   // c4_lo_write
//         30,  // c4_lo_capture
//         18,  // c4_hi_write
//         25,  // c3_lo_read
//         2,   // c3_hi_read
//         29,  // c3_hi_inject
//         18,  // c4_lo_read
//         18,  // c4_hi_read
// };

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

template <uint8_t F>
inline uint16_t PinsMc6809::ns() const {
    return DELAY_NS[F];
}

PinsMc6809::PinsMc6809() {
    _regs = new RegsMc6809(this);
    auto devs = new DevsMc6809();
    _devs = devs;
    _mems = new MemsMc6809(devs);
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
        // Synchronize to C4L
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

    PinsMc6809Engine::resetPins();
}

Signals *PinsMc6809::rawCycle() const {
    // C1H
    extal_hi();
    Signals::inputMode();
    auto s = Signals::put();
    delayNanoseconds(ns<c1_hi_ns>());
    // C2L
    extal_lo();
    delayNanoseconds(ns<c2_lo_ns>());
    s->getHighAddr();
    // C2H
    extal_hi();
    s->getDirection();
    delayNanoseconds(ns<c2_hi_ns>());
    // C3L
    extal_lo();
    s->getLowAddr();
    if (s->write()) {
        delayNanoseconds(ns<c3_lo_write>());
        // C3H
        extal_hi();
        s->getData();
        delayNanoseconds(ns<c3_hi_write>());
        // C4L
        extal_lo();
        if (s->writeMemory()) {
            mems<MemsMc6809>()->write(s->addr, s->data);
            delayNanoseconds(ns<c4_lo_write>());
        } else {
            delayNanoseconds(ns<c4_lo_capture>());
        }
        // C4H
        extal_hi();
        Cycles::next();
        delayNanoseconds(ns<c4_hi_write>());
    } else {
        delayNanoseconds(ns<c3_lo_read>());
        // C3H
        extal_hi();
        if (s->readMemory()) {
            s->data = mems<MemsMc6809>()->read(s->addr);
            delayNanoseconds(ns<c3_hi_read>());
        } else {
            delayNanoseconds(ns<c3_hi_inject>());
        }
        // C4L
        extal_lo();
        s->outData();
        delayNanoseconds(ns<c4_lo_read>());
        // C4H
        extal_hi();
        Cycles::next();
        delayNanoseconds(ns<c4_hi_read>());
    }
    s->getControl();
    // C1L
    extal_lo();

    return s;
}

Signals *PinsMc6809::cycle() const {
    delayNanoseconds(ns<c1_lo_ns>());
    return rawCycle();
}

template struct PinsMc6809Engine<PinsMc6809>;

}  // namespace mc6809
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
