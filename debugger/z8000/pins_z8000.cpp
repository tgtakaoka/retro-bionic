#include "pins_z8000.h"
#include "debugger.h"
#include "devs_z8000.h"
#include "inst_z8000.h"
#include "mems_z8000.h"
#include "regs_z8000.h"
#include "signals_z8000.h"

namespace debugger {
namespace z8000 {

using z80::DevsZ80;

// clang-format off
/**
 * Z8000 memory cycle (Z8000 CPU User's Reference Manual, Section 9.4).
 *
 * The board drives CLOCK; everything the CPU drives follows its edges.
 * A memory cycle is counted from the edge #AS falls on; I/O and
 * acknowledge cycles, which add waits, and a cycle resumed from #WAIT are
 * clocked half a period at a time, watching for #DS.
 *
 *      |      T1       |      T2       |      T3       |
 *      |_______        |_______        |_______        |
 * CLK  |       |_______|       |_______|       |_______|
 *      _________________________________________________
 * ST   X________________________________________________X
 *      ___       _______________________________________
 * AS      |_____|
 *      __________________                      _________
 * DS                     |____________________|
 *         ________          ____________________
 * AD   --<__addr__>--------<_____read data______>-------  the board drives
 *         ________  ____________________________
 * AD   --<__addr__><_______write data___________>-------  the CPU drives
 *                                ^ #WAIT        ^ read data
 *
 * ST is ST3-ST0 with N/#S, R/#W, B/#W and #MREQ: valid from T1 through
 * the cycle. The address on AD is valid at the #AS rise, and AD turns
 * around after it. A read is driven from #DS falling until it rises and
 * sampled on the T3 falling edge; write data is valid while #DS is low.
 * #WAIT is sampled on the T2 falling edge, and on each wait state.
 *
 * Refresh and internal operation strobe #AS but not #DS; I/O and
 * acknowledge cycles add automatic waits before #DS.
 */
// clang-format on

namespace {

// Measured at the CPU, #AS, #DS, #MREQ and the status move within 10ns of
// the CLOCK edge. A half period lasts as long as its work; one with none
// gets these: delayNanoseconds() stretches them, to about 40ns low and
// 50ns high as measured, over the 10 MHz parts' 40ns.
constexpr auto clock_hi_ns = 30;
constexpr auto clock_lo_ns = 20;
// After a clock edge, for the strobes it moves to settle: 10ns measured.
constexpr auto strobe_delay_ns = 10;
// Periods without #AS before the CPU counts as halted.
constexpr auto no_as_periods = 2048;
// Refresh and internal-operation cycles skipped in a row before the CPU
// counts as halted, which may be how it shows a HALT.
constexpr auto no_data_cycles = 256;
// Half periods a strobe may take to come.
constexpr auto strobe_halves = 256;
// Bus cycles between polls of the halt switch while running.
constexpr auto halt_poll_cycles = 16;
// Bus transactions allowed for an acknowledge and its frame.
constexpr auto nmi_cycles = 256;
// Where a trap the debugger takes parks the CPU: the PC it serves for
// the handler. Nothing runs from there; injection answers its fetches.
constexpr uint16_t PARK_PC = 0x0000;

const uint8_t PINS_LOW[] = {
        PIN_CLOCK,
        PIN_RESET,
};

const uint8_t PINS_HIGH[] = {
        PIN_NMI,
        PIN_NVI,
        PIN_VI,
        PIN_WAIT,
        PIN_STOP,
};

const uint8_t PINS_INPUT[] = {
        PIN_AS,
        PIN_DS,
        PIN_RW,
        PIN_BW,
        PIN_NS,
        PIN_MREQ,
        PIN_ST0,
        PIN_ST1,
        PIN_ST2,
        PIN_ST3,
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
};

// CLOCK is low between bus cycles: after T3, after T1 when a cycle is
// parked, after a parked period, and after reset.
inline void clock_hi() {
    digitalWriteFast(PIN_CLOCK, HIGH);
}

inline void clock_lo() {
    digitalWriteFast(PIN_CLOCK, LOW);
}

// A high half, ending low.
void clock_cycle_lo() {
    clock_hi();
    delayNanoseconds(clock_hi_ns);
    clock_lo();
}

// A whole period where nothing else runs between the edges.
void clock_cycle() {
    clock_cycle_lo();
    delayNanoseconds(clock_lo_ns);
}

void assert_reset() {
    digitalWriteFast(PIN_RESET, LOW);
}

void negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
}

void assert_wait() {
    digitalWriteFast(PIN_WAIT, LOW);
}

void negate_wait() {
    digitalWriteFast(PIN_WAIT, HIGH);
}

// #NMI is edge-triggered and latched: negate once acknowledged.
void assert_nmi() {
    digitalWriteFast(PIN_NMI, LOW);
}

void negate_nmi() {
    digitalWriteFast(PIN_NMI, HIGH);
}

inline auto signal_as() {
    return digitalReadFast(PIN_AS);
}

inline auto signal_ds() {
    return digitalReadFast(PIN_DS);
}

}  // namespace

PinsZ8000::PinsZ8000() {
    _devs = new DevsZ80(USART_BASE);
    _mems = new MemsZ8000();
    _regs = new RegsZ8000(this);
}

// After reset the CPU reads the FCW at 0002 and the PC at 0004 in system
// mode, then fetches there (Section 7.4). The FCW is served as the
// debugger's, system mode with interrupts off, so the parked CPU may run
// privileged sequences; the program's is the one in memory.
void PinsZ8000::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    // #RESET must be held 5 clocks at least.
    assert_reset();
    for (auto i = 0; i < 16; i++)
        clock_cycle();
    Cycles::reset();
    negate_reset();

    const auto fcw = _mems->read(InstZ8000::ORG_FCW);
    auto s = prepareCycle();
    if (s == nullptr || !s->memReq() || !s->read() ||
            s->addr != InstZ8000::ORG_FCW) {
        cli.print("?reset: FCW ");
        if (s)
            s->print();
        return;
    }
    completeCycle(s->inject(InstZ8000::SYS_FCW));
    s = prepareCycle();
    if (s == nullptr || !s->memReq() || !s->read() ||
            s->addr != InstZ8000::ORG_PC) {
        cli.print("?reset: PC ");
        if (s)
            s->print();
        return;
    }
    completeCycle(s);
    const uint16_t pc = s->data;
    s = prepareCycle();
    if (s == nullptr || s->st() != ST_FETCH || s->addr != pc) {
        cli.print("?reset: fetch ");
        if (s)
            s->print();
        return;
    }
    assert_wait();
    auto regs = this->regs<RegsZ8000>();
    regs->setIp(pc);
    regs->setFcw(fcw);
    regs->save();
}

void PinsZ8000::idle() {
    // The CPU is parked in #WAIT; this only keeps the clock alive. The
    // caller's own work makes the low half.
    clock_cycle_lo();
}

// Up to the #AS rise of the next cycle that moves data, sampling the
// address just before it and the status just after. Refresh and internal
// operation are clocked through unrecorded. Null where the CPU has
// stopped strobing, as it does halted.
Signals *PinsZ8000::prepareCycle() {
    _resumed = false;
    for (auto skipped = 0; skipped < no_data_cycles; ++skipped) {
        auto s = Signals::put();
        s->clearMark();  // Cycles::next() keeps the slot's old mark
        // #AS falls just after a rising edge, the first one after a cycle
        // while running; only a CPU that doesn't strobe gets counted.
        for (auto guard = no_as_periods;;) {
            clock_hi();
            delayNanoseconds(strobe_delay_ns);
            if (signal_as() == LOW)
                break;
            if (guard-- == 0)
                return nullptr;
            clock_lo();
            delayNanoseconds(clock_lo_ns);
        }
        // The address is on AD while #AS is low. The falling edge raises
        // #AS, by when the status is valid; reading the port takes longer
        // than the CPU does.
        s->getAddr();
        clock_lo();  // T1 falling
        s->getControl();
        if (!s->noData())
            return s;
    }
    return nullptr;
}

// Let the parked CPU go; AD no longer carries the address, so the caller
// gives the one captured when it parked.
Signals *PinsZ8000::resumeCycle(uint32_t addr) {
    auto s = Signals::put();
    s->addr = addr;
    s->getControl();
    _resumed = true;
    negate_wait();
    return s;
}

Signals *PinsZ8000::completeCycle(Signals *s) {
    // No data moves: prepareCycle() skips these, but a sequence cut short
    // may leave one parked.
    if (s->noData())
        return s;
    // A read's data is looked up while the clock runs on to #DS, and
    // driven once #DS falls, when the CPU has let go of AD.
    if (s->read()) {
        if (s->memReq()) {
            if (s->readMemory()) {
                if (s->wordAccess()) {
                    s->data = _mems->read16(s->addr & ~1);
                } else {
                    // Both halves: the CPU takes the one A0 picks.
                    const uint16_t v = _mems->read_byte(s->addr);
                    s->data = v | (v << 8);
                }
            }
        } else if (s->ack()) {
            // An identifier: a device's vector, or nothing for #NMI.
            s->data = s->intAck() ? _devs->vector() : 0;
        } else if (s->ioReq() && _devs->isSelected(s->addr) &&
                   s->readMemory()) {
            const uint16_t v = _devs->read(s->addr);
            s->data = s->wordAccess() ? v : (v | (v << 8));
        }
    }

    if (!_resumed && s->memReq()) {
        // A memory cycle, counted from the edge #AS fell on, as measured on
        // the board: a read's #DS falls on the T2 rising edge and a write's
        // on the T2 falling edge; both rise on the T3 falling edge, where
        // the CPU takes read data. The port accesses take longer than the
        // CPU's 10ns.
        clock_hi();  // T2 rising
        if (s->read()) {
            s->outData();
        } else {
            delayNanoseconds(clock_hi_ns);
        }
        clock_lo();  // T2 falling
        if (s->write()) {
            // Storing alone falls short of the 40ns low.
            delayNanoseconds(strobe_delay_ns);
            s->getData();
            storeData(s);
        } else {
            delayNanoseconds(clock_lo_ns);
        }
        clock_hi();  // T3 rising
        Cycles::next();
        clock_lo();  // T3 falling: the CPU takes the data, #DS rises
        s->inputMode();
        return s;
    }

    // #DS may have fallen already, coming back out of a #WAIT stretch:
    // watch for it rather than count to it. The clock is low here; each
    // half gets its floor, which also settles the strobes.
    auto high = false;
    for (auto guard = strobe_halves; signal_ds() != LOW && guard; --guard) {
        clock_hi();
        delayNanoseconds(clock_hi_ns);
        if (signal_ds() == LOW) {
            high = true;
            break;
        }
        clock_lo();
        delayNanoseconds(clock_lo_ns);
    }

    if (s->read()) {
        s->outData();
    } else {
        s->getData();
        storeData(s);
    }

    // Hold until #DS rises, just after a falling edge: a read drives AD
    // until then. Compared against LOW, never HIGH.
    if (!high) {
        clock_hi();
        delayNanoseconds(clock_hi_ns);
    }
    clock_lo();
    delayNanoseconds(clock_lo_ns);
    for (auto guard = strobe_halves; signal_ds() == LOW && guard; --guard) {
        clock_hi();
        delayNanoseconds(clock_hi_ns);
        clock_lo();
        delayNanoseconds(clock_lo_ns);
    }
    s->inputMode();

    // The fetch an acknowledge follows was nullified: the instruction is
    // fetched again after the interrupt.
    if (s->ack()) {
        const auto held = Signals::get()->diff(s);
        for (uint_fast8_t i = 1; i <= 4 && i <= held; ++i) {
            const auto t = s->prev(i);
            if (t->st() == ST_FETCH) {
                t->nullify();
                break;
            }
        }
    }
    Cycles::next();
    return s;
}

// A write's data to memory or a device.
void PinsZ8000::storeData(Signals *s) {
    if (s->memReq()) {
        if (s->writeMemory()) {
            if (s->wordAccess()) {
                _mems->write16(s->addr & ~1, s->data);
            } else {
                // Driven on both halves; A0 picks.
                _mems->write_byte(
                        s->addr, (s->addr & 1) ? lo(s->data) : hi(s->data));
            }
        }
    } else if (s->ioReq() && _devs->isSelected(s->addr) && s->writeMemory()) {
        // A byte rides AD0-AD7.
        _devs->write(s->addr, s->wordAccess() ? s->data : lo(s->data));
    }
}

void PinsZ8000::execute(const uint8_t *inst, uint_fast8_t len, uint8_t *buf,
        uint_fast8_t max, uint32_t &org, uint32_t exit) {
    const uint32_t leaves = exit == EXIT_END ? org + len : exit;
    uint_fast8_t cap = 0;
    // The exit counts once the window's last word was read: the CPU
    // fetches ahead, and an exit inside the window is read on the way --
    // as restore() returning to the PC it is parked at, its window's
    // first word.
    bool whole = false;
    auto s = resumeCycle(org);
    // Bound the damage: a wrong dump beats a hung debugger.
    auto guard = Cycles::MAX_CYCLES;
    while (s && guard--) {
        const auto reading = s->memReq() && s->read();
        if (reading && whole && cap >= max && s->addr == leaves)
            break;
        const uint32_t off = s->addr - org;
        const auto injecting = reading && off < len;
        const auto capturing = s->memReq() && s->write() && cap < max;
        if (injecting) {
            const auto at = off & ~1;
            const uint16_t word =
                    uint16(inst[at], at + 1 < len ? inst[at + 1] : 0);
            if (s->wordAccess()) {
                s->inject(word);
            } else {
                const uint8_t b = (off & 1) ? lo(word) : hi(word);
                s->inject(uint16(b, b));
            }
            if (at + 2 >= len)
                whole = true;
        } else if (capturing) {
            s->capture();
        }
        completeCycle(s);
        if (capturing && buf) {
            if (s->wordAccess()) {
                buf[cap++] = hi(s->data);
                if (cap < max)
                    buf[cap++] = lo(s->data);
            } else {
                buf[cap++] = (s->addr & 1) ? lo(s->data) : hi(s->data);
            }
        }
        s = prepareCycle();
    }
    // Park in it, and hand the caller where that is.
    assert_wait();
    if (s)
        org = s->addr;
}

void PinsZ8000::execInst(
        const uint8_t *inst, uint_fast8_t len, uint32_t &org, uint32_t exit) {
    execute(inst, len, nullptr, 0, org, exit);
}

void PinsZ8000::captureWrites(const uint8_t *inst, uint_fast8_t len,
        uint8_t *buf, uint_fast8_t max, uint32_t &org, uint32_t exit) {
    execute(inst, len, buf, max, org, exit);
}

// A trap has just pushed its frame, |push| its PC: serve the handler's
// FCW and PC from the Program Status Area as the debugger's, and park on
// the fetch there. The ring drops everything from |from|.
bool PinsZ8000::parkAfterFrame(Signals *push, Signals *from) {
    const uint16_t pc = push->data;
    const uint16_t fcw = push->next()->data;
    const uint16_t psa[] = {InstZ8000::SYS_FCW, PARK_PC};
    for (auto word : psa) {
        auto s = prepareCycle();
        if (s == nullptr || !s->memReq() || !s->read())
            return false;
        completeCycle(s->inject(word));
    }
    auto s = prepareCycle();
    if (s == nullptr || !s->read() || s->addr != PARK_PC)
        return false;
    assert_wait();
    Cycles::discard(from);
    this->regs<RegsZ8000>()->parkInTrap(pc, fcw, s->addr);
    return true;
}

// |id| was just written: whether it ends the frame of an SC the debugger
// takes, a breakpoint or the samples' exit, then parked after it.
bool PinsZ8000::scBreak(Signals *id) {
    if (!(id->memReq() && id->write()))
        return false;
    if (id->data != InstZ8000::SC_BREAK && id->data != InstZ8000::SC_EXIT)
        return false;
    // The pushes go down: the PC, the FCW, then the SC word itself.
    const auto fcw = id->prev(1);
    const auto push = id->prev(2);
    if (!(fcw->memReq() && fcw->write() && fcw->addr == id->addr + 2 &&
                push->memReq() && push->write() && push->addr == id->addr + 4))
        return false;
    // The SC's address: the PC pushed is the next instruction's.
    const uint16_t sc = push->data - 2;
    // A program may use SC #0FFH itself.
    if (id->data == InstZ8000::SC_BREAK && !isBreakPoint(sc))
        return false;
    // The CPU fetched the next word ahead before trapping; nothing ran it.
    const auto held = Signals::get()->diff(push);
    for (uint_fast8_t i = 1; i <= 4 && i <= held; ++i) {
        const auto t = push->prev(i);
        if (t->st() == ST_FETCH && t->addr == uint16_t(sc + 2)) {
            t->nullify();
            break;
        }
    }
    if (!parkAfterFrame(push, push))
        return false;
    // Report the SC; continuing runs it again, as the Z80's RST 38H.
    auto regs = this->regs<RegsZ8000>();
    regs->parkInTrap(sc, fcw->data, regs->parkedAt());
    return true;
}

// Take #NMI on |s|, a cycle prepared but not completed: the instruction
// in progress runs, then the acknowledge, the frame and the debugger's
// handler. Null |s| is a CPU in HALT, which #NMI wakes; its frame holds
// the PC after the HALT.
bool PinsZ8000::suspend(Signals *s) {
    assert_nmi();
    if (s)
        completeCycle(s);
    const Signals *ack = nullptr;
    Signals *push = nullptr;
    auto writes = 0;
    for (auto guard = nmi_cycles; guard > 0; --guard) {
        s = prepareCycle();
        if (s == nullptr)
            break;
        completeCycle(s);
        if (ack == nullptr) {
            if (s->nmiAck()) {
                ack = s;
                negate_nmi();
            }
            continue;
        }
        if (s->memReq() && s->write()) {
            if (writes++ == 0)
                push = s;
            if (writes == 3)
                break;
        }
    }
    negate_nmi();
    if (writes < 3)
        return false;
    // From the nullified fetch on, the cycles are the debugger's.
    auto from = const_cast<Signals *>(ack);
    if (from->prev()->st() == ST_FETCH)
        from = from->prev();
    return parkAfterFrame(push, from);
}

bool PinsZ8000::rawStep() {
    // A HALT waits for an interrupt; stepping into it would hang.
    const auto regs = this->regs<RegsZ8000>();
    if (_mems->read(regs->nextIp()) == InstZ8000::HALT)
        return false;
    return suspend(resumeCycle(regs->parkedAt()));
}

bool PinsZ8000::step(bool show) {
    Cycles::reset();
    _regs->restore();
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

// Free-run until an SC the debugger takes, a halt, or the halt switch.
// Returns whether the CPU stopped where its registers can be saved.
bool PinsZ8000::loop() {
    auto s = resumeCycle(this->regs<RegsZ8000>()->parkedAt());
    for (uint_fast8_t n = 0;; ++n) {
        completeCycle(s);
        if (scBreak(s))
            return true;
        _devs->loop();
        s = prepareCycle();
        if (s == nullptr) {
            // Halted, as by a HALT: take #NMI to stop where the registers
            // can be saved.
            return suspend(nullptr);
        }
        // It polls USB and stretches the clock: not every cycle.
        if ((n % halt_poll_cycles) == 0 && haltSwitch())
            return suspend(s);
    }
}

void PinsZ8000::run() {
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

void PinsZ8000::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, InstZ8000::SC_BREAK);
}

// The program's FCW picks which of the two it takes; one that enables
// both takes two acknowledges for each interrupt.
void PinsZ8000::assertInt(uint8_t) {
    digitalWriteFast(PIN_VI, LOW);
    digitalWriteFast(PIN_NVI, LOW);
}

void PinsZ8000::negateInt(uint8_t) {
    digitalWriteFast(PIN_VI, HIGH);
    digitalWriteFast(PIN_NVI, HIGH);
}

void PinsZ8000::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

// ST 1101 marks each instruction's first word on the bus.
const SignalsImpl *PinsZ8000::findBacktraceStart() {
    return backtraceStartByFetchCount<Signals>(_lineLimit);
}

// One line per instruction, then its data cycles; its other words only
// when verbose.
void PinsZ8000::printBacktrace() {
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
        } else if (s->st() != ST_PROGRAM || s->addr < from || s->addr >= to ||
                   Debugger.verbose()) {
            s->print();
        }
        idle();
    }
}

}  // namespace z8000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
