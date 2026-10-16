#include "pins_tms320c25.h"
#include "debugger.h"
#include "devs_tms320.h"
#include "inst_tms320c2x.h"
#include "mems_tms320c25.h"
#include "regs_tms320c2x.h"
#include "signals_tms320c25.h"

namespace debugger {
namespace tms320c25 {

using tms320c2x::InstTms320C2X;
using tms320c2x::RegsTms320C2X;
using Inst = InstTms320C2X;

// clang-format off
/**
 * TMS320C25 bus cycle, no wait states
 *         __    __    __    __    __    __
 * CLKIN _|  |__|  |__|  |__|  |__|  |__|  |_
 *       _______             ___________
 * CLKOUT1      \___________/           \____
 *       ___ ________________________ _______
 *  ADDR ___X________________________X_______
 *       ______             ___________
 *  #STRB      \___________/           \_____
 *               ___________
 *  DATA -------<___________>----------------
 *
 * Each signal follows a rising CLKIN edge by 25-60 ns; a CLKIN period is
 * one quarter phase Q, #STRB low for 2Q. The CPU reads data at #STRB
 * rising, 40 ns setup and 0 hold; a write's data is valid 2Q-45 ns before
 * #STRB rises.
 */
// tc(C): min 195, max 597 ns; CLKOUT1 cycle time, 4 CLKIN periods
// clang-format on

namespace {

constexpr auto clkin_lo_ns = 25;
constexpr auto clkin_hi_ns = 25;
// a rising CLKIN edge to #STRB, data and the controls following it
constexpr auto clkin_settle_ns = 60;

const uint8_t PINS_LOW[] = {
        PIN_RS,
        PIN_CLKIN,
        PIN_ASEL,
};

const uint8_t PINS_HIGH[] = {
        PIN_BIO,
        PIN_INT0,
        PIN_INT1,
        PIN_READY,
};

const uint8_t PINS_INPUT[] = {
        PIN_PS,
        PIN_DS,
        PIN_IS,
        PIN_RW,
        PIN_STRB,
        PIN_XF,
        PIN_CLKOUT1,
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
        PIN_AL0,
        PIN_AL1,
        PIN_AL2,
        PIN_AL3,
        PIN_AH0,
        PIN_AH1,
        PIN_AH2,
        PIN_AH3,
};

inline void clkin_lo() {
    digitalWriteFast(PIN_CLKIN, LOW);
}

inline void clkin_hi() {
    digitalWriteFast(PIN_CLKIN, HIGH);
}

// One CLKIN period ending on a settled rising edge: the next quarter phase.
void clkin_quarter() {
    clkin_lo();
    delayNanoseconds(clkin_lo_ns);
    clkin_hi();
    delayNanoseconds(clkin_settle_ns);
}

void clkin_cycle() {
    clkin_lo();
    delayNanoseconds(clkin_lo_ns);
    clkin_hi();
    delayNanoseconds(clkin_hi_ns);
}

void negate_reset() {
    digitalWriteFast(PIN_RS, HIGH);
}

// I/O ports are A0-A3 of an #IS cycle
constexpr uint16_t port(uint16_t addr) {
    return addr & 0xF;
}

}  // namespace

PinsTms320C25::PinsTms320C25() {
    _devs = new tms320::DevsTms320();
    auto regs = new RegsTms320C2X(this);
    _regs = regs;
    _mems = new MemsTms320C25(regs);
}

void PinsTms320C25::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    // #RS low at least 3 CLKOUT1 cycles
    for (auto i = 0; i < 4 * 6; i++)
        clkin_cycle();
    Cycles::reset();
    negate_reset();
    // the reset fetch from 0 waits at Signals::put()
    prepareCycle();
    _regs->save();
}

// CLKIN periods to wait for #STRB: a running CPU strobes every machine
// cycle of 4 it spends off-chip, one held in reset or IDLE never.
// ATTENTION: a run in IDLE fetches nothing, so the halt switch cannot stop
// it; only an interrupt ends IDLE.
constexpr auto strobe_clocks = 1000;

Signals *PinsTms320C25::prepareCycle() {
    auto s = Signals::put();
    for (auto n = 0; !s->getControl(); ++n) {
        if (n >= strobe_clocks) {
            // Give up rather than wedge the board, leaving the halt switch
            // to stop the run.
            cli.println("?halt: no bus cycle");
            return s;
        }
        clkin_quarter();
    }
    s->getAddr();
    return s;
}

// #STRB went low on the last rising edge; it rises on the second one after.
void PinsTms320C25::completeCycle(Signals *s) {
    if (!s->strobe())
        return;
    if (s->read()) {
        if (s->readMemory()) {
            if (s->is()) {
                const auto pa = port(s->addr);
                s->data = _devs->isSelected(pa) ? _devs->read(pa) : 0;
            } else {
                s->data = _mems->read(s->addr);
            }
        }
        s->outData();
        clkin_quarter();
        Cycles::next();
        clkin_quarter();
        s->inputMode();
    } else {
        clkin_quarter();
        s->getData();
        // A held ring reuses this slot, and next() clears its capture flag
        const auto store = s->writeMemory();
        Cycles::next();
        clkin_quarter();
        if (store) {
            if (s->is()) {
                const auto pa = port(s->addr);
                if (_devs->isSelected(pa))
                    _devs->write(pa, s->data);
            } else {
                _mems->write(s->addr, s->data);
            }
        }
    }
}

Signals *PinsTms320C25::nextCycle(Signals *s) {
    completeCycle(s);
    return prepareCycle();
}

// A data or I/O cycle of an injected sequence: data space is the program's
// own memory, I/O belongs to the injection and never reaches the devices.
static void injected(Signals *s) {
    if (s->is())
        s->read() ? s->inject(0) : s->capture();
}

uint16_t PinsTms320C25::injectRead(uint16_t inst) {
    for (auto s = Signals::put();; s = nextCycle(s)) {
        if (s->programRead()) {
            const auto addr = s->addr;
            nextCycle(s->inject(inst));
            return addr;
        }
        injected(s);
    }
}

// Run the injected sequence on to its |read| or write cycle in I/O space
// (port |addr|) or data space (at |addr|), feeding NOPs to the fetches;
// other data cycles, an earlier store still in the pipeline, go to memory.
uint16_t PinsTms320C25::transfer(
        bool read, bool io, uint16_t addr, uint16_t data) {
    for (auto s = Signals::put();; s = nextCycle(s)) {
        if (s->programRead()) {
            s->inject(Inst::NOP);
            continue;
        }
        const auto at = s->is() ? port(s->addr) : s->addr;
        if (s->strobe() && s->read() == read && s->is() == io && at == addr) {
            if (read) {
                nextCycle(s->inject(data));
                return data;
            }
            completeCycle(s->capture());
            data = s->data;
            prepareCycle();
            return data;
        }
        injected(s);
    }
}

uint16_t PinsTms320C25::captureWrite() {
    return transfer(false, true, 0, 0);
}

uint16_t PinsTms320C25::captureData(uint16_t addr) {
    return transfer(false, false, addr, 0);
}

void PinsTms320C25::injectIo(uint16_t data) {
    transfer(true, true, 0, data);
}

void PinsTms320C25::injectData(uint16_t addr, uint16_t data) {
    transfer(true, false, addr, data);
}

uint16_t PinsTms320C25::fetchAddr() const {
    return Signals::put()->addr;
}

// The first fetch off the sequence after a branch is its target, the words
// fetched before it being discarded; feed those NOPs.
// ATTENTION: how many a taken branch discards is unverified; a target
// equal to the next sequential fetch after them is not told apart.
uint16_t PinsTms320C25::branchTo(uint16_t pc) {
    // a target the NOP run could reach in sequence goes by a far address
    if (static_cast<uint16_t>(pc - fetchAddr()) <= 8) {
        const auto far = static_cast<uint16_t>(fetchAddr() + 0x100);
        branchTo(far);
    }
    injectRead(Inst::B);
    uint16_t expect = injectRead(pc) + 1;
    uint_fast8_t n = 0;
    for (auto s = Signals::put();; s = nextCycle(s)) {
        if (s->programRead()) {
            if (s->addr != expect)
                return s->addr;
            if (++n > 8) {
                cli.println("?branch: no target fetch");
                return s->addr;
            }
            s->inject(Inst::NOP);
            expect++;
        } else {
            injected(s);
        }
    }
}

// Halt at the pending fetch: feed NOPs while the instructions already in
// the pipeline finish with their real cycles, and settle where the program
// goes on, the fetch after a discontinuity being a taken branch or an
// interrupt. Off-sequence program reads while a table instruction is in
// flight are its table reads.
// ATTENTION: |table| comes from the last words fetched, which may be
// operands or discarded prefetches.
PinsTms320C25::Halt PinsTms320C25::drain(bool table) {
    Halt h{fetchAddr(), fetchAddr(), false};
    uint16_t expect = h.pc;
    uint_fast8_t n = 0;
    // more table reads than a repeated table instruction makes
    uint_fast16_t reads = 0;
    for (auto s = Signals::put();; s = nextCycle(s)) {
        if (s->programRead()) {
            if (s->addr != expect && table && ++reads > 260)
                table = false;
            if (s->addr != expect && !table) {
                h.pc = expect = s->addr;
                if (Inst::isVector(h.pc)) {
                    h.interrupt = true;
                } else {
                    h.ret = h.pc;
                }
                n = 0;
            }
            if (s->addr == expect) {
                if (n >= RegsTms320C2X::DRAIN_FETCHES)
                    return h;
                s->inject(Inst::NOP);
                expect++;
                n++;
            }
        }
    }
}

void PinsTms320C25::idle() {
    auto s = Signals::put();
    if (s->programRead()) {
        s->inject(Inst::NOP);
    } else {
        injected(s);
    }
    nextCycle(s);
}

bool PinsTms320C25::step(bool show) {
    // Cycles::reset() drops the pending fetch's slot; prepareCycle() reads
    // it back, the CPU waiting with #STRB low
    Cycles::reset();
    prepareCycle();
    _regs->restore();
    if (show) {
        Cycles::reset();
        prepareCycle();
    }
    // the stepped instruction, and the one an RPT repeats with it
    const auto pc = fetchAddr();
    auto inst = _mems->read(pc);
    auto words = Inst::words(inst);
    auto table = Inst::table(inst);
    if (Inst::repeat(inst)) {
        inst = _mems->read(pc + words);
        words += Inst::words(inst);
        table |= Inst::table(inst);
    }
    // an interrupt the restore let in fetches off the sequence
    uint16_t expect = pc;
    for (auto s = Signals::put();; s = nextCycle(s)) {
        if (!s->programRead())
            continue;
        if (words == 0 || s->addr != expect)
            break;
        words--;
        expect++;
    }
    const auto h = drain(table);
    if (show)
        printCycles();
    regs<RegsTms320C2X>()->save(h.pc, h.interrupt ? h.ret : -1);
    return true;
}

void PinsTms320C25::loop() {
    // the last two words fetched, for drain()
    uint16_t recent[2] = {Inst::NOP, Inst::NOP};
    // the first fetch is where the run starts, perhaps at a break point
    auto first = true;
    for (auto s = Signals::put();; s = nextCycle(s)) {
        if (!s->strobe()) {
            // nothing on the bus: IDLE, or a CPU gone astray
            if (haltSwitch()) {
                cli.println("?halt: no bus cycle; registers not saved");
                return;
            }
            continue;
        }
        if (!s->programRead())
            continue;
        // the instruction after RPT stays whole: a NOP there would be the
        // one repeated
        if (!first && !Inst::repeat(recent[0]) &&
                (haltSwitch() || isBreakPoint(s->addr))) {
            const auto h =
                    drain(Inst::table(recent[0]) || Inst::table(recent[1]));
            if (haltSwitch() || isBreakPoint(h.pc)) {
                const auto end = Signals::put();
                {
                    Cycles::Hold hold;
                    regs<RegsTms320C2X>()->save(h.pc, h.interrupt ? h.ret : -1);
                }
                Cycles::discard(end);
                prepareCycle();
                return;
            }
            // a break point fetched only to be discarded: go on from where
            // the program went, its fetch now pending
            branchTo(h.pc);
            s = Signals::put();
        }
        first = false;
        recent[1] = recent[0];
        recent[0] = _mems->read(s->addr);
        _devs->loop();
    }
}

void PinsTms320C25::run() {
    _regs->restore();
    Cycles::reset();
    prepareCycle();
    saveBreakInsts();
    startRunTimer();
    loop();
    stopRunTimer();
    restoreBreakInsts();
    disassembleCycles();
}

void PinsTms320C25::assertInt(uint8_t name) {
    digitalWriteFast(name == 1 ? PIN_INT1 : PIN_INT0, LOW);
}

void PinsTms320C25::negateInt(uint8_t name) {
    digitalWriteFast(name == 1 ? PIN_INT1 : PIN_INT0, HIGH);
}

void PinsTms320C25::setBio(bool high) {
    digitalWriteFast(PIN_BIO, high ? HIGH : LOW);
}

void PinsTms320C25::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

// A program read is a fetch, a table read or a discarded prefetch alike,
// so the backtrace shows the bus cycles as they came.
void PinsTms320C25::printBacktrace() {
    printCycles();
}

}  // namespace tms320c25
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
