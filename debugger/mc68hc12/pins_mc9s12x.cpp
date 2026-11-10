#include "pins_mc9s12x.h"
#include "clock_mc68hc12.h"
#include "pins_mc68hc12_cycle.h"
#include "debugger.h"
#include "digital_bus.h"
#include "mc9s12x_init.h"
#include "mems_mc68hc12.h"
#include "queue_hcs12x.h"
#include "signals_mc68hc12.h"

namespace debugger {
namespace mc68hc12 {

// clang-format off
/**
 * MC9S12XDP512 bus cycles, a read then a write, in emulation expanded mode
 * (S12XEBIV2 4.2.2). With the PLL off the bus clock is EXTAL/2, so a cycle
 * is four EXTAL edges, ECLK high first.
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
 * - An emulation memory access takes one cycle, a PRR access two and any
 *   other external one at least two; the cycles after the first show
 *   ACC=000 and the same address. Read data is due in the cycle after the
 *   last, write data comes in the last, and IQSTAT shows the access in the
 *   first repeat, 0000 after.
 */
// clang-format on

using namespace clock;

namespace {

// Emulation expanded mode, MODC:MODB:MODA=011, with ROMCTL high and
// EROMCTL low: emulation memory, the debugger's, in place of the Flash.
// Special test mode shows neither ACC nor IQSTAT nor IVD: its ECLK low
// phase carries the next address.

// ATTENTION: A chip powered up without EXTAL runs on its self-clock; it
// returns to EXTAL once a clock check passes. Run EXTAL this long before
// releasing #RESET.
constexpr uint32_t clock_check_ms = 100;
// ATTENTION: The CRG holds the internal reset for 192 SYSCLK cycles; hold
// #RESET for more EXTAL periods than that.
constexpr auto reset_hold_pairs = 256 * 2;
// ATTENTION: the access whose ECLK fall asserts a step's #XIRQ, counted
// from the one after the fetch restore() leaves held, without the repeat
// cycles, as on the HCS12; RTI pulls 10 bytes (UUUUUPPP).
constexpr uint8_t step_at = 2;
// ATTENTION: ASEL to the 74HCS153 outputs, at least; half the first
// guess, under test.
constexpr auto mux_ns = 15;
// ATTENTION: the address phase's 00 to 01 step, which overlaps the R/W
// and #LSTRB read.
constexpr auto sel01_ns = 5;

// ACC2:0
constexpr uint8_t ACC_REPEAT = 0;
constexpr uint8_t ACC_CPU = 1;
constexpr uint8_t ACC_NONE = 4;

constexpr uint8_t PINS_LOW[] = {
        PIN_EXTAL,
        PIN_BKGD,    // special mode
        PIN_LSTRB,   // EROMCTL: emulation memory
        PIN_XCLKS,   // EXTAL takes an external clock
        PIN_ASEL0,
        PIN_ASEL1,
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_MODA,    // emulation expanded
        PIN_ROMCTL,  // the Flash area in the map
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
// TAGLO/TAGHI inputs, kept high, BKGD into the BDM pin, XCLKS into PE7,
// ROMCTL into the EWAIT input and EROMCTL into the #LSTRB output. The board's pull-down alone loses to
// PE7's pull-up in reset, which leaves the chip on its self-clock; ECLKX2
// stays off (ECLKCTL NCLKX2=1).
void release_mode() {
    pinMode(PIN_MODA, INPUT_PULLUP);
    pinMode(PIN_MODB, INPUT_PULLUP);
    pinMode(PIN_BKGD, INPUT_PULLDOWN);
    pinMode(PIN_XCLKS, INPUT);
    pinMode(PIN_ROMCTL, INPUT);
    pinMode(PIN_LSTRB, INPUT);
}

// ASEL1:ASEL0 rests at 00. Stepped in gray order 00, 01, 11, 10 the muxes
// hand out A0/A16, A4/A20, A8/-, A12/- in turn, one select changing per
// step. P30-P33 and P34-P37 are bits 0-3 and 16-19 of one port, so each
// step's nibbles shift together by 0, 4, 8 and 12: A0-A22 in bits 0-22.
// In ECLK low, IVD15:0 come the same way, IQSTAT3:0 in bits 16-19 and
// ACC2:0 in 20-22. Each step is put away while the next select settles.
constexpr uint32_t NIBBLES = 0x000F000F;

// Selections 00 and 01, left at 01.
uint32_t mux_low8() {
    const uint32_t sel00 = PIN(MUXL);
    digitalWriteFast(PIN_ASEL0, HIGH);  // 01
    uint32_t v = sel00 & NIBBLES;
    delayNanoseconds(mux_ns);
    return v | ((PIN(MUXL) & NIBBLES) << 4);
}

// Selections 11 and 10 after mux_low8(), back at 00.
uint32_t mux_high8() {
    digitalWriteFast(PIN_ASEL1, HIGH);  // 11
    delayNanoseconds(mux_ns);
    const uint32_t sel11 = PIN(MUXL);
    digitalWriteFast(PIN_ASEL0, LOW);  // 10
    uint32_t v = (sel11 & NIBBLES) << 8;
    delayNanoseconds(mux_ns);
    v |= (PIN(MUXL) & NIBBLES) << 12;
    digitalWriteFast(PIN_ASEL1, LOW);  // 00
    return v;
}

}  // namespace

// What the PRRs read after reset in emulation expanded mode with ROMCTL
// high and EROMCTL low; the ports and the rest read 0.
void PinsMc9s12x::resetPrr() const {
    const auto m = mems<MemsMc68hc12>();
    for (uint16_t addr = 0; addr < Mc9s12xInit::PRR_END; ++addr) {
        if (Mc9s12xInit::is_prr(addr))
            m->write_byte(addr, 0);
    }
    m->write_byte(Mc9s12xInit::MODE, 0x60);     // MODC:MODB:MODA=011
    m->write_byte(Mc9s12xInit::EBICTL0, 0x3F);  // ASIZ=11111
    m->write_byte(Mc9s12xInit::EBICTL1, 0x07);  // EXSTR=111
    m->write_byte(Mc9s12xInit::MMCCTL0, 0x01);  // CS0E=ROMON
    m->write_byte(Mc9s12xInit::MMCCTL1, 0x01);  // ROMON=1, EROMON=0
    m->write_byte(Mc9s12xInit::ECLKCTL, 0x40);  // NCLKX2=1
}

PinsMc9s12x::PinsMc9s12x(Mc9s12xInit &init)
    : PinsMc68hc12(init, step_at, RATED_HZ) {
#ifdef TUNE_CLOCK
    setClockInput(DEFAULT_HZ);
#endif
}


void PinsMc9s12x::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    Signals::inputMode();
    _xirq = false;
    _held = false;
    _driving = false;
    _ivd = nullptr;
    _ahead = false;
    _repeat = false;
    _repeats = 0;
    _writes = 0;
    resetPrr();

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

// One bus cycle from ECLK low: ECLK high, then low again, leaving the
// status of the low phase in |c|. ECLK follows the edges, so it is looked
// at once, to catch STOP. Returns false when ECLK stays stopped.
bool PinsMc9s12x::clockCycle(Cycle &c) {
    quiet_edge();
    delay<&Timing::extal>();
    active_edge();
    delay<&Timing::addr>();
    if (clock_e() == LOW) {
        // STOP freezes ECLK until an interrupt; walk until it comes back.
        for (auto n = 0; clock_e() == LOW; ++n) {
            if (n >= 1000)
                return false;
            edge_pair();
        }
        for (auto n = 0; clock_e() != LOW && n < 8; ++n)
            edge_pair();
        return clockCycle(c);
    }
    // ECLK high. A repeat shows the address before it, so it isn't read.
    c.skipped = _repeats != 0;
    if (c.skipped) {
        c.global = _global;
        c.cntl = PIN(CNTL);
    } else {
        const uint32_t sel00 = PIN(MUXL);
        digitalWriteFast(PIN_ASEL0, HIGH);  // 01
        uint32_t low = sel00 & NIBBLES;
        // R/W and #LSTRB take up part of the settle.
        c.cntl = PIN(CNTL);
        delayNanoseconds(sel01_ns);
        low |= (PIN(MUXL) & NIBBLES) << 4;
        c.global = _global = (low | mux_high8()) & 0x7FFFFF;
    }
    quiet_edge();
    if (_repeats) {
        --_repeats;
    } else {
        armStep();
    }
    delay<&Timing::extal>();
    active_edge();
    // ECLK falls some 16ns after the edge, and the CPU takes the read data
    // then; a write drives DATA from there.
    delay<&Timing::data>();
    if (_driving && !c.read()) {
        Signals::inputMode();
        _driving = false;
    }
    c.status = mux_low8();
    if (_ivd) {
        c.status |= mux_high8();
    } else {
        digitalWriteFast(PIN_ASEL0, LOW);  // 00
    }
    return true;
}

// Clocks through the repeats of the access before, up to the first cycle
// of the next one, and decodes it into |s|.
bool PinsMc9s12x::nextAccess(Signals *s, Signals *before) {
    Cycle c;
    for (;;) {
        if (!clockCycle(c))
            return false;
        if (c.acc() != ACC_REPEAT) {
            // Its address went unread: nothing to decode.
            if (c.skipped)
                cli.println("?no repeat");
            break;
        }
        if (!_repeat) {
            _repeat = true;
            _repeatIq = c.iqstat();
        }
        if (before && before->write())
            before->getData();
    }
    if (_driving) {
        Signals::inputMode();
        _driving = false;
    }
    if (_ivd) {
        _ivd->data = static_cast<uint16_t>(c.status);
        _ivd = nullptr;
    }
    const auto global = c.global;
    // |_aheadSignals| keeps the flags an injected sequence left.
    s->clear();
    s->setAccess(Mc9s12xInit::local(global), c.read(), c.lstrb());
    s->setIqstat(_repeat ? _repeatIq : c.iqstat());
    _repeat = false;
    const auto cpu = c.acc() == ACC_CPU;
    const auto prr = cpu && Mc9s12xInit::is_prr(global);
    const auto outside = prr || (cpu && Mc9s12xInit::is_external(global));
    // The one repeat a PRR access takes; emulation memory takes none.
    _repeats = prr ? 1 : 0;
    if (c.acc() == ACC_NONE) {
        s->setFree();
    } else if (c.read()) {
        s->setExternal(outside);
        if (!outside)
            _ivd = s;
    } else {
        s->getData();
    }
    return true;
}

// A write's data is final in its last cycle, so a write comes back once
// the next access has started; that one waits in |_aheadSignals|.
template <typename>
Signals *PinsMc9s12x::prepareCycleOf() {
    auto s = Signals::put();
    if (_held) {
        _held = false;
        *s = _heldSignals;
        return s;
    }
    if (_ahead) {
        _ahead = false;
        *s = _aheadSignals;
        if (_ivd == &_aheadSignals)
            _ivd = s;
    } else if (!nextAccess(s, nullptr)) {
        return noBusCycle(s);
    }
    if (s->write()) {
        if (!nextAccess(&_aheadSignals, s))
            return noBusCycle(s);
        _ahead = true;
    }
    return s;
}

// The read data goes out now and stays through the access's repeats and
// the next cycle's ECLK high. A PRR is the debugger's to keep, whoever
// asks.
template <typename>
Signals *PinsMc9s12x::completeCycleOf(Signals *s) {
    const auto prr = Mc9s12xInit::is_prr(s->addr);
    if (s->external()) {
        if (s->readMemory() || prr)
            s->data = readBus(s);
        s->outData();
        _driving = true;
    } else if (s->write() && (s->writeMemory() || prr)) {
        writeBus(s);
    }
    Cycles::next();
    return s;
}

Signals *PinsMc9s12x::prepareCycle() {
    return prepareCycleOf<PinsMc9s12x>();
}

Signals *PinsMc9s12x::completeCycle(Signals *s) {
    return completeCycleOf<PinsMc9s12x>(s);
}

const Signals *PinsMc9s12x::loop() {
    return loopOf<PinsMc9s12x>();
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
