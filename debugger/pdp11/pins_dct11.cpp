#include "pins_dct11.h"
#include "debugger.h"
#include "devs_pdp11.h"
#include "digital_bus.h"
#include "inst_pdp11.h"
#include "mems_pdp11.h"
#include "regs_pdp11.h"
#include "signals_dct11.h"

namespace debugger {
namespace dct11 {

using pdp11::DevsPdp11;
using pdp11::InstPdp11;
using pdp11::MemsPdp11;
using pdp11::RegsPdp11;

// clang-format off
/**
 * DCT11 16-bit static read and write (DCT11 User's Guide, Figures A-2
 * and A-3), standard microcycle: a transaction is two microcycles of
 * three XTL1 periods.
 *
 * The board drives XTL1. The DCT11 is static down to DC, and its
 * outputs move after XTL1's falling edge. A transaction is counted from
 * the period #RAS falls in; the board stops the clock high there to park
 * the CPU.
 *
 *          ________          ____________________________
 * DAL     X__addr__X--------<_______read data____________>----  the board drives
 *          ________  ________________________________
 * DAL     X__addr__><__________write data____________>-----  the CPU drives
 *         ____                                       ________
 * #RAS        |_____________________________________|
 *         __________                              ______
 * #CAS              |____________________________|
 *                          _________________
 * PI      ________________|                 |____________
 *
 * The address, SEL1-SEL0 and R/#WHB-R/#WLB are valid at the #RAS fall.
 * A read is latched at the #CAS rise; write data is valid from the PI
 * rise. An IACK strobes #RAS alone with SEL1 high, and latches a vector
 * from DAL7-DAL2 at the SEL1 fall. Busnops strobe nothing; an ASPI
 * strobes #CAS and PI without #RAS.
 */
// clang-format on

namespace {

// A half period with no work gets these; the 10 MHz part needs 20ns
// each, and the delay of its NMOS outputs is not specified.
constexpr auto clock_lo_ns = 40;
constexpr auto clock_hi_ns = 50;
// Periods without #RAS before a run counts the CPU as in WAIT.
constexpr auto no_ras_periods = 256;
// Periods a strobe may take to come within a transaction.
constexpr auto strobe_periods = 32;
// Periods #BCLR may stay low; a power-up holds it 100 and RESET 84.
constexpr auto bclr_periods = 256;
// Bus cycles between halt switch polls while running.
constexpr auto halt_poll_cycles = 16;
// Bus cycles allowed for a halt or a step to reach the debugger.
constexpr auto trap_cycles = 256;

// Start 173000 (restart 173004), user mode, 16-bit bus, static memory,
// normal read/write, standard microcycle, processor clock (DCT11 User's
// Guide, Table 4-1). DAL7-DAL2 aren't read; they are left at the
// pull-ups' level.
constexpr uint16_t MODE_REGISTER = 0153377;
constexpr uint16_t ORG_START = 0173000;
// A HALT, the HALT input and the debugger's traps go here; the debugger
// answers its fetch.
constexpr uint16_t ORG_RESTART = 0173004;

const uint8_t PINS_HIGH[] = {
        PIN_XTL1,
        PIN_PUP,
        PIN_READY,
        PIN_AI0,
        PIN_AI1,
        PIN_AI2,
        PIN_AI3,
        PIN_AI4,
        PIN_AI5,
        PIN_AI6,
        PIN_AI7,
};

const uint8_t PINS_INPUT[] = {
        PIN_RAS,
        PIN_CAS,
        PIN_PI,
        PIN_RWHB,
        PIN_RWLB,
        PIN_SEL0,
        PIN_SEL1,
        PIN_BCLR,
        PIN_COUT,
        PIN_DAL0,
        PIN_DAL1,
        PIN_DAL2,
        PIN_DAL3,
        PIN_DAL4,
        PIN_DAL5,
        PIN_DAL6,
        PIN_DAL7,
        PIN_DAL8,
        PIN_DAL9,
        PIN_DAL10,
        PIN_DAL11,
        PIN_DAL12,
        PIN_DAL13,
        PIN_DAL14,
        PIN_DAL15,
};

inline void xtl1_hi() {
    digitalWriteFast(PIN_XTL1, HIGH);
}

inline void xtl1_lo() {
    digitalWriteFast(PIN_XTL1, LOW);
}

// One period from the falling edge, ending high: the outputs that edge
// moved have had the whole period to settle, and the next falling edge,
// which may take the address off DAL, waits for the next call.
void clock_cycle() {
    xtl1_lo();
    delayNanoseconds(clock_lo_ns);
    xtl1_hi();
    delayNanoseconds(clock_hi_ns);
}

void assert_pup() {
    digitalWriteFast(PIN_PUP, HIGH);
}

void negate_pup() {
    digitalWriteFast(PIN_PUP, LOW);
}

// The HALT request is pseudo-edge triggered: negate it once taken.
void assert_halt() {
    digitalWriteFast(PIN_AI7, LOW);
}

void negate_halt() {
    digitalWriteFast(PIN_AI7, HIGH);
}

// digitalReadFast() may return the port bit, not 1: compare with LOW.
inline bool ras_asserted() {
    return digitalReadFast(PIN_RAS) == LOW;
}

inline bool cas_asserted() {
    return digitalReadFast(PIN_CAS) == LOW;
}

inline bool pi_asserted() {
    return digitalReadFast(PIN_PI) != LOW;
}

inline bool bclr_asserted() {
    return digitalReadFast(PIN_BCLR) == LOW;
}

// Clocks until |cond| holds; false if it doesn't come.
template <typename COND>
bool clockUntil(COND cond) {
    for (auto guard = strobe_periods; guard > 0; --guard) {
        clock_cycle();
        if (cond())
            return true;
    }
    return false;
}

// The mode register is read from DAL15-DAL8 and DAL1-DAL0 while #BCLR
// is low, at power-up and on each RESET instruction.
void loadModeRegister() {
    busWriteFast(DAL, MODE_REGISTER);
    busMode(DAL, OUTPUT);
    for (auto guard = bclr_periods; guard > 0 && bclr_asserted(); --guard)
        clock_cycle();
    busMode(DAL, INPUT);
}

}  // namespace

PinsDct11::PinsDct11() {
    _devs = new DevsPdp11();
    _mems = new MemsPdp11();
    _regs = new RegsPdp11(this);
}

void PinsDct11::resetPins() {
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    // PUP must be held 100us; #BCLR follows it.
    assert_pup();
    for (auto i = 0; i < 16; i++)
        clock_cycle();
    delayMicroseconds(100);
    busWriteFast(DAL, MODE_REGISTER);
    busMode(DAL, OUTPUT);
    negate_pup();
    loadModeRegister();
    Cycles::reset();
    _devPending = false;

    // Busnops and an ASPI, then the first fetch, about 300 periods on.
    Signals *s = nullptr;
    for (auto i = 0; i < 4 && s == nullptr; ++i)
        s = prepareCycle();
    if (s == nullptr || !s->fetch() || s->addr != ORG_START) {
        cli.print("?reset: fetch ");
        if (s)
            s->print();
        else
            cli.println();
        return;
    }
    _regs->setIp(ORG_START);
    _regs->save();
}

// Clocks to the next transaction, parking at its #RAS fall. Null when
// none comes, as in a WAIT. A RESET instruction's #BCLR gets the mode
// register on the way.
Signals *PinsDct11::prepareCycle() {
    for (auto guard = no_ras_periods; guard > 0; --guard) {
        clock_cycle();
        if (ras_asserted()) {
            auto s = Signals::put();
            s->clearMark();  // Cycles::next() keeps the slot's old mark
            s->getAddr();
            s->getControl();
            return s;
        }
        if (bclr_asserted())
            loadModeRegister();
    }
    return nullptr;
}

// Takes the parked transaction up again; the clock stopped with the
// address on DAL, but the caller passes the one it captured.
Signals *PinsDct11::resumeCycle(uint16_t addr) {
    auto s = Signals::put();
    s->clearMark();
    s->addr = addr;
    s->getControl();
    return s;
}

Signals *PinsDct11::completeCycle(Signals *s) {
    _inCycle = true;
    settleDevRead(s);
    if (s->iack()) {
        s->data = _devs->vector();
        s->outVector();
        clockUntil([] { return !ras_asserted(); });
    } else if (s->read()) {
        // The CPU lets DAL go by the #CAS fall and latches at its rise.
        retrieveData(s);
        clockUntil(cas_asserted);
        s->outData();
        clockUntil([] { return !cas_asserted(); });
    } else {
        clockUntil(pi_asserted);
        s->getData();
        storeData(s);
        clockUntil([] { return !cas_asserted(); });
    }
    s->inputMode();
    // PI falls 10ns after the #CAS rise, within the period that saw it.
    _inCycle = false;
    driveInt();
    Cycles::next();
    return s;
}

// Reads are whole words; the CPU picks the byte it needs.
void PinsDct11::retrieveData(Signals *s) {
    if (!s->readMemory())
        return;
    const uint16_t addr = s->addr & ~1;
    if (_devs->isSelected(addr)) {
        s->data = devs<DevsPdp11>()->peek(addr);
        _devRead = addr;
        _devPending = true;
    } else {
        s->data = _mems->read16(addr);
    }
}

// A write's data on the halves R/#WHB and R/#WLB pick.
void PinsDct11::storeData(Signals *s) {
    if (!s->writeMemory())
        return;
    const uint16_t addr = s->addr & ~1;
    if (_devs->isSelected(addr)) {
        uint16_t val = s->data;
        if (s->byteWrite())
            val = s->writeLow() ? lo(s->data) : hi(s->data);
        _devs->write(addr, val);
    } else if (!s->byteWrite()) {
        _mems->write16(addr, s->data);
    } else if (s->writeLow()) {
        _mems->write_byte(addr, lo(s->data));
    } else {
        _mems->write_byte(addr | 1, hi(s->data));
    }
}

// A device read takes effect unless the transaction after it writes the
// same word: that read was the one every write starts with.
void PinsDct11::settleDevRead(const Signals *s) {
    if (!_devPending)
        return;
    _devPending = false;
    if (s->write() && (s->addr & ~1) == _devRead)
        return;
    _devs->read(_devRead);
}

void PinsDct11::execute(const uint16_t *inst, uint_fast8_t words,
        uint16_t *buf, uint_fast8_t max, uint16_t &org, uint32_t exit,
        const uint16_t *stack, uint16_t stackAt, uint_fast8_t stackWords) {
    const uint16_t bytes = 2 * words;
    uint_fast8_t cap = 0;
    // The exit counts only once the sequence's last word was read: it
    // may lie inside the sequence (save() returns to its own start).
    bool whole = false;
    auto s = resumeCycle(org);
    // Bound the damage: a wrong dump beats a hung debugger.
    for (auto guard = Cycles::MAX_CYCLES; s && guard > 0; --guard) {
        const uint16_t off = s->addr - org;
        if (s->fetch() && whole && cap >= max &&
                (s->addr == exit || off >= bytes))
            break;
        if (s->read()) {
            const uint16_t at = s->addr - stackAt;
            if (off < bytes) {
                s->inject(inst[off / 2]);
                if (off / 2 + 1u == words)
                    whole = true;
            } else if (stack && at < 2u * stackWords) {
                s->inject(stack[at / 2]);
            }
        } else if (s->write() && cap < max) {
            s->capture();
        }
        const auto capturing = !s->writeMemory();
        completeCycle(s);
        if (capturing && buf) {
            const auto high = s->byteWrite() && s->writeHigh();
            buf[cap] = high ? hi(s->data) : s->data;
            ++cap;
        }
        s = prepareCycle();
    }
    // Parked in it; hand the caller where that is.
    if (s)
        org = s->addr;
}

void PinsDct11::execInst(const uint16_t *inst, uint_fast8_t words,
        uint16_t &org, uint32_t exit, const uint16_t *stack, uint16_t stackAt,
        uint_fast8_t stackWords) {
    execute(inst, words, nullptr, 0, org, exit, stack, stackAt, stackWords);
}

void PinsDct11::captureWrites(const uint16_t *inst, uint_fast8_t words,
        uint16_t *buf, uint_fast8_t max, uint16_t &org) {
    execute(inst, words, buf, max, org, org, nullptr, 0, 0);
}

// Whether |s|, a prepared transaction, enters the debugger: a HALT or the
// HALT input fetches the restart address, BPT and a T-bit trace read the
// vector at 14. Each pushed the PS, then the PC. Parks at the restart
// address if so. A BPT's PC is its own unless |step| ran it.
bool PinsDct11::parkAfterFrame(Signals *s, bool step) {
    const auto halt = s->fetch() && s->addr == ORG_RESTART;
    const auto trap =
            s->read() && !s->fetch() && s->addr == InstPdp11::VEC_BPT;
    if (!halt && !trap)
        return false;
    const auto pc = s->prev(1);
    const auto ps = s->prev(2);
    if (!(pc->write() && ps->write() && pc->addr + 2 == ps->addr))
        return false;
    const uint16_t frame[] = {pc->data, ps->data};
    if (trap) {
        // Serve the debugger's vector.
        completeCycle(s->inject(ORG_RESTART));
        s = prepareCycle();
        if (s == nullptr || s->addr != InstPdp11::VEC_BPT + 2)
            return false;
        completeCycle(s->inject(InstPdp11::PSW_PRI7));
        s = prepareCycle();
        if (s == nullptr || !s->fetch() || s->addr != ORG_RESTART)
            return false;
    }
    negate_halt();
    Cycles::discard(ps);
    const uint16_t at = (trap && !step) ? frame[0] - 2 : frame[0];
    // A trace trap pushed the T bit the step set.
    const uint16_t psw = frame[1] & ~InstPdp11::PSW_T;
    regs<RegsPdp11>()->parkInTrap(at, psw, ORG_RESTART);
    return true;
}

// Raises the HALT input on |s|, a prepared transaction, or on a CPU in
// WAIT when null: the instruction finishes, then the PS and PC go on the
// stack and the restart address is fetched.
bool PinsDct11::suspend(Signals *s) {
    assert_halt();
    for (auto guard = trap_cycles; guard > 0; --guard) {
        if (s) {
            if (parkAfterFrame(s, false))
                return true;
            completeCycle(s);
        }
        s = prepareCycle();
    }
    negate_halt();
    return false;
}

bool PinsDct11::rawStep() {
    // A BPT or HALT traps with the trace trap still pending.
    const auto regs = this->regs<RegsPdp11>();
    const auto inst = _mems->read(regs->nextIp());
    if (inst == InstPdp11::BPT || inst == InstPdp11::HALT)
        return false;
    auto s = resumeCycle(regs->parkedAt());
    for (auto guard = trap_cycles; guard > 0; --guard) {
        completeCycle(s);
        s = prepareCycle();
        if (s == nullptr)
            break;  // in WAIT
        if (parkAfterFrame(s, true))
            return true;
    }
    return suspend(s);
}

bool PinsDct11::step(bool show) {
    Cycles::reset();
    auto regs = this->regs<RegsPdp11>();
    regs->setTrace(true);
    _regs->restore();
    regs->setTrace(false);
    if (show)
        Cycles::reset();
    if (rawStep()) {
        if (show)
            printCycles();
        _regs->save();
        return true;
    }
    return false;
}

// Runs until a BPT, a HALT or the halt switch. Returns whether the
// registers can be saved.
bool PinsDct11::loop() {
    Signals *s = resumeCycle(regs<RegsPdp11>()->parkedAt());
    for (uint_fast8_t n = 0;; ++n) {
        if (s)
            completeCycle(s);
        _devs->loop();
        s = prepareCycle();
        if (s && parkAfterFrame(s, false))
            return true;
        // Not every cycle: it polls USB and stretches the clock.
        if ((s == nullptr || (n % halt_poll_cycles) == 0) && haltSwitch())
            return suspend(s);
    }
}

void PinsDct11::run() {
    _regs->restore();
    Cycles::reset();
    saveBreakInsts();
    startRunTimer();
    const auto stopped = loop();
    stopRunTimer();
    restoreBreakInsts();
    if (stopped) {
        Cycles::Hold hold;
        _regs->save();
    }
    disassembleCycles();
}

void PinsDct11::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, InstPdp11::BPT);
}

// Every request is level 4, CP3-CP0 HHLL, with #VEC: the device's vector
// is served at the IACK (DCT11 data sheet, Table 5).
void PinsDct11::assertInt(uint8_t) {
    _intRequest = true;
    if (!_inCycle)
        driveInt();
}

void PinsDct11::negateInt(uint8_t) {
    _intRequest = false;
    if (!_inCycle)
        driveInt();
}

// CP3 stays high on AI1; AI2-AI5 carry CP2 high, CP1, CP0 and #VEC low.
void PinsDct11::driveInt() const {
    busWriteFast(CPV, _intRequest ? 0x1 : CPV_gm);
}

void PinsDct11::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

// SEL0 marks each instruction fetch.
const SignalsImpl *PinsDct11::findBacktraceStart() {
    return backtraceStartByFetchCount<Signals>(_lineLimit);
}

// One line per instruction, then its data cycles; its operand words only
// when verbose.
void PinsDct11::printBacktrace() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    uint32_t from = 0, to = 0;  // the last instruction's bytes
    for (auto i = 0u; i < cycles; ++i) {
        const auto s = g->next(i);
        if (s->fetch()) {
            if (Debugger.verbose())
                s->print();
            from = s->addr;
            to = _mems->disassemble(s->addr, 1);
        } else if (!s->read() || s->addr < from || s->addr >= to ||
                   Debugger.verbose()) {
            s->print();
        }
        idle();
    }
}

}  // namespace dct11
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
