#include "pins_mc9s12x.h"
#include "clock_mc68hc12.h"
#include "debugger.h"
#include "digital_bus.h"
#include "mc9s12x_init.h"
#include "queue_hcs12x.h"
#include "signals_mc68hc12.h"

namespace debugger {
namespace mc68hc12 {

// clang-format off
/**
 * MC9S12XDP512 bus cycles, a read then a write, as the emulation modes
 * show them (S12XEBIV2 4.2.2); special test mode is taken to do the same.
 * With the PLL off the bus clock is EXTAL/2, so a cycle is four EXTAL
 * edges, ECLK high first.
 *          q     a     q     a     q     a     q     a     q
 *                 _____       _____       _____       _____
 *  EXTAL ________/     \_____/     \_____/     \_____/     \_
 *                 ___________             ___________
 *   ECLK ________/           \___________/           \_______
 *        _________________________
 *    R/W                          \__________________________
 *   ADDR   <addr0----><acc0/iq/ivd><addr1----><acc1/iq/ivd>
 *   DATA   ----------------------<_R0_______>-<W1____________
 *
 * - The address, R/W and #LSTRB are valid in ECLK high; the access
 *   source ACC2:0, the queue status IQSTAT3:0 and the previous cycle's
 *   internal read data IVD15:0 replace the address in ECLK low.
 * - External read data is due in the next cycle's ECLK high, before
 *   ECLK falls; write data is valid from this cycle's ECLK low.
 * - ACC tells a CPU access from a free cycle (no access), an XGATE or a
 *   BDM one; the address tells inside from outside.
 */
// clang-format on

using namespace clock;

namespace {

// Special test mode, MODC:MODB:MODA=010, with ROMCTL low: the Flash off.
// ATTENTION: special test mode is for Freescale's own use; whether it
// shows ACC, IQSTAT and IVD as the emulation modes do is unbenched
// (S12XEBIV2 Table 21-1 says it does, S12X9SECV2 Table 30-1 not).

// ATTENTION: A chip powered up without EXTAL runs on its self-clock; it
// returns to EXTAL once a clock check passes. Run EXTAL this long before
// releasing #RESET.
constexpr uint32_t clock_check_ms = 100;
// ATTENTION: The CRG holds the internal reset for 192 SYSCLK cycles; hold
// #RESET for more EXTAL periods than that.
constexpr auto reset_hold_pairs = 256 * 2;
// ATTENTION: the cycle whose ECLK fall asserts a step's #XIRQ, counted
// from the one after the fetch restore() leaves held, unlike the HCS12's
// count; RTI pulls 10 bytes (UUUUUPPP), unbenched.
constexpr uint8_t step_at = 2;
// ATTENTION: ASEL to the 74HCS153 outputs, at least.
constexpr auto mux_ns = 30;
// ECLK fall to ACC, IQSTAT, IVD and write data (tIVDD, tDDW: 5ns).
constexpr auto low_ns = 20;

// ACC2:0
constexpr uint8_t ACC_CPU = 1;
constexpr uint8_t ACC_NONE = 4;

constexpr uint8_t PINS_LOW[] = {
        PIN_EXTAL,
        PIN_BKGD,    // special mode
        PIN_MODA,    // special test
        PIN_ROMCTL,  // the Flash off
        PIN_XCLKS,   // EXTAL takes an external clock
        PIN_ASEL0,
        PIN_ASEL1,
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_MODB,
        PIN_IRQ,
        PIN_XIRQ,
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
        PIN_D8,
        PIN_D9,
        PIN_D10,
        PIN_D11,
        PIN_D12,
        PIN_D13,
        PIN_D14,
        PIN_D15,
        PIN_RW,
        PIN_LSTRB,  // EROMCTL in reset, pulled up inside
        PIN_ECLK,
        PIN_MUX0,
        PIN_MUX1,
        PIN_MUX2,
        PIN_MUX3,
        PIN_MUX4,
        PIN_MUX5,
        PIN_MUX6,
        PIN_MUX7,
};

// The mode pins are latched at the #RESET rise: MODA/MODB turn into the
// TAGLO/TAGHI inputs, kept high, BKGD into the BDM pin, XCLKS into PE7
// (ECLKX2 stays off in special test mode; the board pulls it down) and
// ROMCTL into the EWAIT input.
void release_mode() {
    pinMode(PIN_MODA, INPUT_PULLUP);
    pinMode(PIN_MODB, INPUT_PULLUP);
    pinMode(PIN_BKGD, INPUT_PULLDOWN);
    pinMode(PIN_XCLKS, INPUT);
    pinMode(PIN_ROMCTL, INPUT);
}

// One mux selection: P30-P33 in the low nibble, P34-P37 in the high one.
uint8_t mux(uint8_t sel) {
    busWriteFast(ASEL, sel);
    delayNanoseconds(mux_ns);
    const auto p = PIN(MUXL);
    return busValue(MUXL, p) | busValue(MUXH, p);
}

// ADDR15-ADDR0, or IVD15-IVD0 in ECLK low, from |m0| and |m1|, the low
// nibbles of selections 00 and 01, and selections 10 and 11.
uint16_t low16(uint8_t m0, uint8_t m1) {
    return (m0 & 0xF) | ((m1 & 0xF) << 4) | ((mux(3) & 0xF) << 8) |
           ((mux(2) & 0xF) << 12);
}

}  // namespace

PinsMc9s12x::PinsMc9s12x(Mc9s12xInit &init) : PinsMc68hc12(init, step_at) {}

void PinsMc9s12x::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    Signals::inputMode();
    _xirq = false;
    _held = false;
    _driving = false;
    _ivd = nullptr;
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
        cli.println("?no ECLK");
    if (clock_e() != LOW)
        edge_pair();
    startPark();
}

// From ECLK low, through a whole cycle: ECLK high, then low again.
Signals *PinsMc9s12x::prepareCycle() {
    auto s = Signals::put();
    if (_held) {
        _held = false;
        *s = _heldSignals;
        return s;
    }
    quiet_edge();
    delayNanoseconds(extal_ns);
    active_edge();
    if (!wait_e(HIGH)) {
        // STOP freezes ECLK until an interrupt; walk until it comes back.
        for (auto n = 0; clock_e() == LOW; ++n) {
            if (n >= 1000)
                return noBusCycle(s);
            edge_pair();
        }
        for (auto n = 0; clock_e() != LOW && n < 8; ++n)
            edge_pair();
        return prepareCycle();
    }
    // ECLK high
    delayNanoseconds(addr_ns);
    const auto m0 = mux(0);
    const auto m1 = mux(1);
    const uint32_t global = low16(m0, m1) |
                            (static_cast<uint32_t>(m0 >> 4) << 16) |
                            (static_cast<uint32_t>((m1 >> 4) & 7) << 20);
    const auto read = digitalReadFast(PIN_RW) != LOW;
    const auto lstrb = digitalReadFast(PIN_LSTRB) != LOW;
    quiet_edge();
    armStep();
    delayNanoseconds(extal_ns);
    active_edge();
    wait_e(LOW);
    // ECLK low: the CPU has taken the read data.
    if (_driving) {
        Signals::inputMode();
        _driving = false;
    }
    delayNanoseconds(low_ns);
    const auto q0 = mux(0);
    const auto q1 = mux(1);
    if (_ivd) {
        _ivd->data = low16(q0, q1);
        _ivd = nullptr;
    }
    const uint8_t acc = (q1 >> 4) & 7;
    s->setAccess(Mc9s12xInit::local(global), read, lstrb);
    s->setIqstat(q0 >> 4);
    if (acc == ACC_NONE) {
        s->setFree();
    } else if (read) {
        const auto outside = acc == ACC_CPU && Mc9s12xInit::is_external(global);
        s->setExternal(outside);
        if (!outside)
            _ivd = s;
    } else {
        s->getData();
    }
    return s;
}

// The read data goes out now and stays through the next cycle's ECLK
// high.
Signals *PinsMc9s12x::completeCycle(Signals *s) {
    if (s->external()) {
        if (s->readMemory())
            s->data = readBus(s);
        s->outData();
        _driving = true;
    } else if (s->write() && s->writeMemory()) {
        writeBus(s);
    }
    Cycles::next();
    return s;
}

void PinsMc9s12x::replay(Signals *begin, uint_fast8_t cycles) {
    QueueHcs12x().replay(begin, cycles);
}

}  // namespace mc68hc12
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
