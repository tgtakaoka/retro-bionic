#include "pins_mc9s12d.h"
#include "clock_mc68hc12.h"
#include "debugger.h"
#include "mc9s12d_init.h"
#include "queue_hcs12.h"
#include "signals_mc68hc12.h"

namespace debugger {
namespace mc68hc12 {

// clang-format off
/**
 * MC9S12DG128B bus cycles, a read then a write, in special test mode (the
 * HCS12's special expanded wide) with the PLL off: the bus clock is
 * EXTAL/2, so a cycle is four EXTAL edges, as on the MC68HC912BD32.
 *          q     a     q     a     q     a     q     a     q
 *                 _____       _____       _____       _____
 *  EXTAL ________/     \_____/     \_____/     \_____/     \_
 *                 ___________             ___________
 *      E ________/           \___________/           \_______
 *        __________________________
 *    R/W                           \_________________________
 *     AD   <addr>------<_R__>------<addr><____W_____>--
 *
 * - The address, R/W, #LSTRB, NOACC and the execution start are valid
 *   before the E rise; the data movement before the E fall.
 * - There is no #DBE. Internal accesses show on the bus (IVIS) with the
 *   CPU's data, and NOACC marks the free cycles, so the debugger drives AD
 *   for a read that is neither internal nor free.
 * - Read data is needed 13ns before the E fall; write data is valid 7ns
 *   after the E rise.
 * - Until MISC clears it, an external access stretches E high for three
 *   more cycles.
 */
// clang-format on

using namespace clock;

namespace {

// Special test mode, MODC:MODB:MODA=010. The emulation expanded wide mode
// (011) is the fallback, but there PEAR and MODE are write-never.
constexpr uint8_t MODA_LEVEL = LOW;

// ATTENTION: A chip powered up without EXTAL runs on its self-clock; it
// returns to EXTAL once a clock check passes, 50000 VCO cycles at 1-5.5MHz
// (CRG V3 4.2.5). Run EXTAL this long before releasing #RESET.
constexpr uint32_t clock_check_ms = 100;
// ATTENTION: The CRG holds the internal reset for 192 SYSCLK cycles; hold
// #RESET for more EXTAL periods than that.
constexpr auto reset_hold_pairs = 256 * 2;
// ATTENTION: Cycles in a row without E before the CPU counts as stopped:
// free cycles may not show E.
constexpr uint16_t stop_cycles = 64;
// ATTENTION: the cycle after restore() whose E fall asserts a step's
// #XIRQ; an RTI with an interrupt pending is one cycle longer on the HCS12
// (uUUUUfVfPPP) than on the M68HC12, unbenched.
constexpr uint8_t step_at = 2;

constexpr uint8_t PINS_LOW[] = {
        PIN_EXTAL,
        PIN_BKGD,    // special mode
        PIN_MODA,    // special test
        PIN_XCLKS,   // EXTAL takes an external clock
        PIN_ROMCTL,  // the Flash off; special test forces it anyway
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_MODB,
        PIN_IRQ,
        PIN_XIRQ,
        PIN_RXD,
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
        PIN_AD8,
        PIN_AD9,
        PIN_AD10,
        PIN_AD11,
        PIN_AD12,
        PIN_AD13,
        PIN_AD14,
        PIN_AD15,
        PIN_RW,
        PIN_LSTRB,
        PIN_E,
        PIN_TXD,
        PIN_XADDR14,
        PIN_XADDR15,
        PIN_XADDR16,
        PIN_XADDR17,
        PIN_XADDR18,
        PIN_XADDR19,
        PIN_PT7,
        PIN_PT0,
};

// The mode pins are latched at the #RESET rise: MODA/MODB turn into
// IPIPE0/IPIPE1, BKGD into the BDM pin, XCLKS into NOACC (a port pin
// until NOACCE is set; the board pulls it down) and ROMCTL into a port pin.
void release_mode() {
    pinMode(PIN_MODA, INPUT);
    pinMode(PIN_MODB, INPUT);
    pinMode(PIN_BKGD, INPUT_PULLDOWN);
    pinMode(PIN_XCLKS, INPUT);
    pinMode(PIN_ROMCTL, INPUT);
}

}  // namespace

PinsMc9s12d::PinsMc9s12d(Mc9s12dInit &init) : PinsMc68hc12(init, step_at) {}

void PinsMc9s12d::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    digitalWriteFast(PIN_MODA, MODA_LEVEL);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    Signals::inputMode();
    _xirq = false;
    _held = false;
    _done = false;
    _stretch = true;
    _noacc = false;
    _noE = 0;
    _writes = 0;

    // EXTAL runs without a pause from here until configSystem() has
    // stopped the clock monitor.
    assert_reset();
    const auto start = millis();
    while (millis() - start < clock_check_ms)
        edge_pair();
    const auto synced = sync_e();
    for (auto i = 0; i < reset_hold_pairs; ++i)
        edge_pair();
    negate_reset();
    for (auto i = 0; reset_signal() == LOW && i < 20000; ++i)
        edge_pair();
    release_mode();
    if (!synced && !sync_e())
        cli.println("?no E clock");
    if (clock_e() != LOW)
        edge_pair();
    startPark();
}

Signals *PinsMc9s12d::prepareCycle() {
    auto s = Signals::put();
    if (_held) {
        _held = false;
        *s = _heldSignals;
        return s;
    }
    quiet_edge();
    delayNanoseconds(addr_ns);
    s->getAddr();
    const auto free = _noacc && digitalReadFast(PIN_NOACC) != LOW;
    active_edge();
    if (!wait_e(HIGH)) {
        // No E: a free cycle that doesn't show it, or STOP. Walk on until
        // it comes back, a cycle at a time.
        if (++_noE < stop_cycles) {
            quiet_edge();
            delayNanoseconds(extal_ns);
            s->setFree();
            _done = true;
            active_edge();
            return s;
        }
        for (auto n = 0; clock_e() == LOW; ++n) {
            if (n >= 1000)
                return noBusCycle(s);
            edge_pair();
        }
        for (auto n = 0; clock_e() != LOW && n < 8; ++n)
            edge_pair();
        _noE = 0;
        return prepareCycle();
    }
    _noE = 0;
    // E high
    delayNanoseconds(extal_ns);
    quiet_edge();
    delayNanoseconds(dbe_ns);
    s->setExternal(s->read() && !free && !_init.is_internal(s->addr));
    return s;
}

Signals *PinsMc9s12d::completeCycle(Signals *s) {
    if (_done) {
        _done = false;
    } else if (!s->none()) {
        const auto drive = s->external();
        if (drive) {
            if (s->readMemory())
                s->data = readBus(s);
            s->outData();
            delayNanoseconds(setup_ns);
        }
        s->getStart();
        armStep();
        if (!drive)
            s->getData();  // write data, or what an internal read shows
        active_edge();
        // A stretched cycle keeps E high; the data stays until E falls.
        for (auto n = 0; !wait_e(LOW) && n < 8; ++n) {
            quiet_edge();
            delayNanoseconds(dbe_ns);
            if (!drive)
                s->getData();
            s->getStart();
            active_edge();
        }
        if (drive)
            Signals::inputMode();
        if (s->write() && s->writeMemory())
            writeBus(s);
    }
    Cycles::next();
    return s;
}

void PinsMc9s12d::replay(Signals *begin, uint_fast8_t cycles) {
    QueueHcs12().replay(begin, cycles);
}

}  // namespace mc68hc12
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
