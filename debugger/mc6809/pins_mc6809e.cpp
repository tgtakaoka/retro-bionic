#include "pins_mc6809e.h"
#include "debugger.h"
#include "devs_mc6809.h"
#include "inst_hd6309.h"
#include "mems_mc6809.h"
#include "regs_mc6809e.h"
#include "signals_mc6809.h"

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

// The bus cycle's delays, each in a quarter cycle of 250 ns.
enum Delay : uint8_t {
    cycle_ns,
    c1_ns,
    c2_ns,
    c3_novma,
    c4_novma,
    c3_write,
    c4_write,
    c4_capture,
    c3_read,
    c3_inject,
    c4_read,
    DELAYS,
};

// Long enough that no E/Q cycle or phase runs under the 1 MHz part's minimum.
constexpr uint16_t DELAY_NS[] = {
        87,   // cycle_ns   EQ=LL
        87,   // c1_ns      EQ=LL
        195,  // c2_ns      EQ=LH
        225,  // c3_novma   EQ=HH
        165,  // c4_novma   EQ=HL
        202,  // c3_write   EQ=HH
        55,   // c4_write   EQ=HL
        162,  // c4_capture EQ=HL
        112,  // c3_read    EQ=HH
        235,  // c3_inject  EQ=HH
        117,  // c4_read    EQ=HL
};
// MC68B09E (2 MHz): E 1.91 MHz on the bench.
// constexpr uint16_t DELAY_NS[] = {
//         23,  // cycle_ns   EQ=LL
//         23,  // c1_ns      EQ=LL
//         52,  // c2_ns      EQ=LH
//         60,  // c3_novma   EQ=HH
//         44,  // c4_novma   EQ=HL
//         54,  // c3_write   EQ=HH
//         14,  // c4_write   EQ=HL
//         43,  // c4_capture EQ=HL
//         30,  // c3_read    EQ=HH
//         62,  // c3_inject  EQ=HH
//         31,  // c4_read    EQ=HL
// };
// HD63C09E (3 MHz): E 2.44 MHz on the bench, tEQ 68 ns of 65.
// constexpr uint16_t DELAY_NS[] = {
//         7,   // cycle_ns   EQ=LL
//         7,   // c1_ns      EQ=LL
//         15,  // c2_ns      EQ=LH
//         18,  // c3_novma   EQ=HH
//         13,  // c4_novma   EQ=HL
//         16,  // c3_write   EQ=HH
//         4,   // c4_write   EQ=HL
//         13,  // c4_capture EQ=HL
//         9,   // c3_read    EQ=HH
//         18,  // c3_inject  EQ=HH
//         9,   // c4_read    EQ=HL
// };

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

template <uint8_t F>
inline uint16_t PinsMc6809E::ns() const {
    return DELAY_NS[F];
}

PinsMc6809E::PinsMc6809E() {
    _regs = new RegsMc6809E(this);
    auto devs = new mc6809::DevsMc6809();
    _devs = devs;
    _mems = new mc6809::MemsMc6809(devs);
    _inst = new hd6309::InstHd6309(_mems);
}

void PinsMc6809E::resetPins() {
    // Assert reset condition
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    // At least one #RESET cycle.
    for (auto i = 0; i < 5; i++)
        cycle();
    negate_reset();

    PinsMc6809Engine::resetPins();
}

mc6809::Signals *PinsMc6809E::rawCycle() const {
    // AVMA, read at the end of a cycle, tells whether the next is valid.
    static uint8_t vma_next = LOW;
    // c1
    mc6809::Signals::inputMode();
    auto s = mc6809::Signals::put();
    delayNanoseconds(ns<c1_ns>());
    // c2
    c2_clock();
    delayNanoseconds(ns<c2_ns>());
    s->getDirection();
    s->getHighAddr();
    // c3
    c3_clock();
    s->getLowAddr();
    if (vma_next == LOW) {
        delayNanoseconds(ns<c3_novma>());
        // c4
        c4_clock();
        Cycles::next();
        delayNanoseconds(ns<c4_novma>());
    } else if (s->write()) {
        delayNanoseconds(ns<c3_write>());
        s->getData();
        // c4
        c4_clock();
        if (s->writeMemory()) {
            mems<mc6809::MemsMc6809>()->write(s->addr, s->data);
            delayNanoseconds(ns<c4_write>());
        } else {
            delayNanoseconds(ns<c4_capture>());
        }
        Cycles::next();
    } else {
        if (s->readMemory()) {
            s->data = mems<mc6809::MemsMc6809>()->read(s->addr);
            delayNanoseconds(ns<c3_read>());
        } else {
            delayNanoseconds(ns<c3_inject>());
        }
        // c4
        c4_clock();
        s->outData();
        Cycles::next();
        delayNanoseconds(ns<c4_read>());
    }
    s->getControl();
    vma_next = signal_avma();
    // c1
    c1_clock();

    return s;
}

mc6809::Signals *PinsMc6809E::cycle() const {
    delayNanoseconds(ns<cycle_ns>());
    return rawCycle();
}

}  // namespace mc6809e

namespace mc6809 {
template struct PinsMc6809Engine<mc6809e::PinsMc6809E>;
}  // namespace mc6809
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
