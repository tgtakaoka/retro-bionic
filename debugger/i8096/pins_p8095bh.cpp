#include "pins_p8095bh.h"
#include "debugger.h"
#include "devs_i8096.h"
#include "inst_i8096.h"
#include "mems_i8096.h"
#include "regs_i8096.h"
#include "signals_p8095bh.h"

namespace debugger {
namespace p8095bh {

using i8096::ArchI8096;
using i8096::CpuMemory;
using i8096::InstI8096;
using i8096::MemsI8096;
using i8096::RegsI8096;

// clang-format off
/**
 * P8095BH bus cycle
 *          __    __    __    __    __    __    __    __    __    __    __    __    __
 * XTAL1 __|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |
 *       __\     \                 \     \_____\     \                 \     \____\
 *  #ADV    \____________________________/      \____________________________/     \___
 *       _________                  ___________________________________________________
 *   #RD          \________________/                                         
 *       _____________________________________________                  _______________
 *   #WR                                              \________________/     
 *       ___ ____________________________ ______ ____________________________ ______ __
 *    AD ___X_addr_______________________X_addr_X_________data_______________X_addr_X__
 *
 * The CCB selects the 16-bit bus: a read is a word's, the even byte on
 * AD0-AD7; a write moves the bytes A0 and #BHE pick.
 */

// clang-format on

namespace {

constexpr auto xtal1_lo_ns = 20;  // 25 ns
constexpr auto xtal1_hi_ns = 20;  // 25 ns

const uint8_t PINS_OPENDRAIN[] = {
        PIN_RESET,
};

const uint8_t PINS_HIGH[] = {
        PIN_READY,
        PIN_RXD,
};

const uint8_t PINS_LOW[] = {
        PIN_EXTINT,
        PIN_XTAL1,
};

const uint8_t PINS_INPUT[] = {
        PIN_ADV,
        PIN_RD,
        PIN_WR,
        PIN_BHE,
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
        PIN_HSO0,
        PIN_HSO1,
        PIN_HSO2,
        PIN_HSO3,
        PIN_HSI0,
        PIN_HSI1,
        PIN_HSI2,
        PIN_HSI3,
        PIN_PWM,
        PIN_TXD,
        // PIN_ACH7,
        PIN_ACH4,
        PIN_ACH5,
        PIN_ACH6,
};

inline void xtal1_lo() {
    digitalWriteFast(PIN_XTAL1, LOW);
}

inline void xtal1_hi() {
    digitalWriteFast(PIN_XTAL1, HIGH);
}

void xtal1_cycle_lo() {
    xtal1_hi();
    delayNanoseconds(xtal1_hi_ns);
    xtal1_lo();
}

void xtal1_cycle() {
    xtal1_cycle_lo();
    delayNanoseconds(xtal1_lo_ns);
}

void negate_reset() {
    pinMode(PIN_RESET, INPUT_PULLUP);
}

}  // namespace

PinsP8095BH::PinsP8095BH() {
    _devs = new i8096::DevsI8096();
    auto regs = new RegsI8096(this);
    _regs = regs;
    auto mems = new MemsI8096(_devs, regs);
    _mems = mems;
}

void PinsP8095BH::resetPins() {
    pinsMode(PINS_OPENDRAIN, sizeof(PINS_OPENDRAIN), OUTPUT_OPENDRAIN, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    for (auto i = 0; i < 10 * 4; i++)
        xtal1_cycle();
    Cycles::reset();
    negate_reset();
    _idle = false;
    _held = false;
    // The CCB at 2018H, read before the CCR selects the 16-bit bus: AD0-AD7
    // carry it in every bus mode.
    completeCycle(prepareCycle()->inject(_mems->read(MemsI8096::CCB)), true);
    const auto fetch = prepareCycle();
    if (fetch->addr != InstI8096::ORG_RESET) {
        cli.print("?reset fetch at ");
        cli.printlnHex(fetch->addr, 4);
    }
    hold(fetch, fetch->addr);
    _regs->reset();
    _regs->save();
}

// A full prefetch queue idles the bus for at most an instruction, even a
// DIVL's ~40 state times; a CPU missing or held in reset never answers.
constexpr auto no_bus_cycles = 10000;

Signals *PinsP8095BH::prepareCycle() {
    auto s = _idle ? &_idleSignals : Signals::put();
    if (_held) {
        _held = false;
        *s = _heldSignals;
        return s;
    }
    noInterrupts();
    // assert_debug();
    xtal1_hi();
    for (auto n = 0; !s->getAddrValid(); ++n) {
        if (n >= no_bus_cycles)
            return noBusCycle(s);
        xtal1_lo();
        delayNanoseconds(xtal1_lo_ns);
        xtal1_hi();
    }
    // negate_debug();
    s->getAddr();
    xtal1_lo();
    // assert_debug();
    for (auto n = 0; !s->getControl(); ++n) {
        if (n >= no_bus_cycles)
            return noBusCycle(s);
        xtal1_cycle_lo();
    }
    // negate_debug();
    interrupts();
    return s;
}

// Gives up on a cycle rather than wedge the board, leaving the halt
// switch to stop the run.
Signals *PinsP8095BH::noBusCycle(Signals *s) {
    interrupts();
    cli.println("?halt: no bus cycle");
    s->noCycle();
    return s;
}

// Keeps the CPU waiting in the read |s|, its next fetch at |park|; the
// next prepareCycle() takes it up.
void PinsP8095BH::hold(const Signals *s, uint16_t park) {
    _heldSignals = *s;
    _heldSignals.clear();
    _held = true;
    _park = park;
}

// A 16-bit read is always a word's, at an even address: the CPU discards
// the byte it doesn't need.
uint16_t PinsP8095BH::readBus(const Signals *s) const {
    const uint16_t even = s->addr & ~1;
    return uint16(_mems->read(even + 1), _mems->read(even));
}

void PinsP8095BH::writeBus(const Signals *s) const {
    for (uint_fast8_t i = 0; i < s->bytes(); ++i) {
        const uint16_t addr = s->addr + i;
        _mems->write(addr, s->byteAt(addr));
    }
}

// |low|: drive a read's data on AD0-AD7 only.
Signals *PinsP8095BH::completeCycle(Signals *s, bool low) {
    xtal1_hi();
    if (s->read()) {
        if (s->readMemory()) {
            s->data = readBus(s);
        } else {
            delayNanoseconds(xtal1_hi_ns);
        }
        xtal1_lo();
        // assert_debug();
        if (low) {
            s->outLow();
        } else {
            s->outData();
        }
        // negate_debug();
        xtal1_hi();
        s->inputMode();
    } else if (s->write()) {
        // assert_debug();
        s->getData();
        // negate_debug();
        xtal1_lo();
        if (s->writeMemory()) {
            writeBus(s);
        } else {
            delayNanoseconds(xtal1_lo_ns);
        }
        xtal1_hi();
    }
    if (_idle) {
        delayNanoseconds(xtal1_hi_ns);
    } else {
        Cycles::next();
    }
    noInterrupts();
    // assert_debug();
    while (s->getAddrValid()) {
        xtal1_lo();
        delayNanoseconds(xtal1_lo_ns);
        xtal1_hi();
    }
    xtal1_lo();
    // negate_debug();
    interrupts();
    return s;
}

uint16_t PinsP8095BH::execute(uint16_t org, const uint8_t *inst,
        uint_fast8_t len, uint8_t *buf, uint_fast8_t max, uint32_t exit,
        bool idle, uint16_t at, const uint8_t *data) {
    constexpr uint8_t NOP = 0xFD;
    _idle = idle;
    const uint16_t leaves = exit == EXIT_PARK ? org : exit;
    // The exit counts once the window's last byte was read: a sequence
    // that jumps back to its origin reads it first.
    bool whole = len == 0;
    uint_fast8_t cap = 0;
    uint16_t first = org;
    // Bound the damage: a lost CPU never reads the exit.
    for (auto guard = 0; guard < 100; ++guard) {
        auto s = prepareCycle();
        if (s->read()) {
            const uint16_t from = s->addr;
            const uint16_t to = from + s->bytes();
            if (whole && cap >= max && static_cast<uint16_t>(leaves - from) <
                                               static_cast<uint16_t>(to - from)) {
                hold(s, leaves);
                return first;
            }
            uint16_t word = 0;
            for (uint_fast8_t i = 0; i < s->bytes(); ++i) {
                const uint16_t addr = from + i;
                const uint16_t off = addr - org;
                const uint16_t pop = addr - at;
                uint8_t b = NOP;  // a fetch ahead
                if (off < len) {
                    b = inst[off];
                    if (off == len - 1)
                        whole = true;
                } else if (data && pop < 2) {
                    b = data[pop];
                }
                word |= b << ((addr & 1) ? 8 : 0);
            }
            s->inject(word);
            completeCycle(s);
        } else if (s->write()) {
            const auto capturing = cap < max;
            if (capturing)
                s->capture();
            completeCycle(s);
            if (capturing) {
                if (cap == 0)
                    first = s->addr;
                for (uint_fast8_t i = 0; i < s->bytes() && cap < max; ++i)
                    buf[cap++] = s->byteAt(s->addr + i);
            }
        } else {
            break;
        }
    }
    if (!idle) {
        cli.print("?lost the CPU running at ");
        cli.printHex(org, 4);
        cli.print(" to ");
        cli.printlnHex(leaves, 4);
    }
    _held = false;
    return first;
}

uint16_t PinsP8095BH::execInst(const uint8_t *inst, uint_fast8_t len,
        uint8_t *buf, uint_fast8_t max, uint32_t exit) {
    return execute(_park, inst, len, buf, max, exit, false);
}

void PinsP8095BH::popInst(const uint8_t *inst, uint_fast8_t len, uint16_t at,
        const uint8_t *data, uint32_t exit) {
    execute(_park, inst, len, nullptr, 0, exit, false, at, data);
}

void PinsP8095BH::idle() {
    // The maximum duration of READY=L is 1us and useless for idle.
    static constexpr uint8_t SJMP_HERE[] = {
            SJMP(-2),  // SJMP $
    };
    execute(_park, SJMP_HERE, sizeof(SJMP_HERE), nullptr, 0, EXIT_PARK, true);
}

uint16_t PinsP8095BH::jumpTarget(uint16_t next, uint_fast8_t opc) const {
    const auto pc = _regs->nextIp();
    if (opc >= 0x20 && opc < 0x30) {  // SJMP/SCALL
        const auto opr = ((opc & 7) << 8) | _mems->read(pc + 1);
        constexpr auto sign = 1 << 10;
        constexpr auto mask = (sign << 1) - 1;
        const auto delta = static_cast<int16_t>(((opr + sign) & mask) - sign);
        return next + delta;
    } else if (opc >= 0x30 && opc < 0x40) {  // JBC/JBS
        return next + static_cast<int8_t>(_mems->read(pc + 2));
    } else if (opc >= 0xD0 && opc < 0xE0) {  // Jcc
        return next + static_cast<int8_t>(_mems->read(pc + 1));
    } else if (opc == 0xE0) {  // DJNZ
        return next + static_cast<int8_t>(_mems->read(pc + 2));
    } else if (opc == 0xE3) {  // BR
        return regs<RegsI8096>()->read_data16(_mems->read(pc + 1));
    } else if (opc == 0xE7 || opc == 0xEF) {  // LJMP/LCALL
        return next + static_cast<int16_t>(_mems->read16(pc + 1));
    } else if (opc == 0xF0) {  // RET
        return _mems->read16(regs<RegsI8096>()->sp());
    } else if (opc == InstI8096::TRAP) {  // TRAP
        return _mems->read16(InstI8096::VEC_TRAP);
    }
    return next;
}

bool PinsP8095BH::rawStep(bool show) {
    show &= !Debugger.verbose();
    InstI8096 inst;
    const CpuMemory cpu(mems<MemsI8096>());
    if (!inst.set(_regs->nextIp(), &cpu))
        return false;
    const uint16_t pc = _regs->nextIp();
    const auto len = inst.instLength();
    const uint16_t next = pc + len;
    const auto opc = inst.opc();
    const auto target = jumpTarget(next, opc);
    auto stepTrap = opc == InstI8096::TRAP;
    _regs->restore();
    if (show)
        Cycles::reset();
    _idle = false;
    // The instruction's bytes come from memory until each was fetched;
    // after them, a fetch of the next or the target gets a TRAP.
    const uint8_t all = (1 << len) - 1;
    uint8_t fetched = 0;
    for (uint_fast8_t i = 0; i < 30; i++) {
        auto s = prepareCycle();
        if (s->read() && s->addr == InstI8096::VEC_TRAP) {
            if (stepTrap) {
                stepTrap = false;
            } else {
                handleTrap(s, 0x2344, true);
                if (show)
                    Cycles::discard(s);
                break;
            }
        } else if (s->read()) {
            const auto first = fetched == 0;
            auto word = readBus(s);
            auto trap = false;
            for (uint_fast8_t j = 0; j < s->bytes(); ++j) {
                const uint16_t addr = s->addr + j;
                const uint16_t off = addr - pc;
                if (off < len && fetched != all) {
                    fetched |= 1 << off;
                } else if (addr == next || addr == target) {
                    const auto lane = (addr & 1) ? 8 : 0;
                    word &= ~(0xFF << lane);
                    word |= InstI8096::TRAP << lane;
                    trap = true;
                }
            }
            if (trap)
                s->inject(word);
            completeCycle(s);
            if (first && fetched)
                s->markFetch();
            continue;
        }
        completeCycle(s);
    }
    return true;
}

bool PinsP8095BH::step(bool show) {
    Cycles::reset();
    if (rawStep(show)) {
        if (show)
            printCycles();
        return true;
    }
    return false;
}

// |s| reads the vector: answer |vector|, capture the PC the CPU pushes,
// and park at |vector|, an even address.
void PinsP8095BH::handleTrap(Signals *s, uint16_t vector, bool breakTrap) {
    const uint8_t vec[] = {lo(vector), hi(vector)};
    uint8_t pc[2];
    hold(s, s->addr);
    const auto sp = execute(s->addr, vec, sizeof(vec), pc, sizeof(pc),
            vector, false);
    regs<RegsI8096>()->captureContext(sp, pc[0] | (pc[1] << 8), breakTrap);
}

// Whether the read |s| fetched a breakpoint's TRAP.
bool PinsP8095BH::fetchedBreak(const Signals *s) const {
    if (!s->read())
        return false;
    for (uint_fast8_t i = 0; i < s->bytes(); ++i) {
        const uint16_t addr = s->addr + i;
        if (isBreakPoint(addr) && s->byteAt(addr) == InstI8096::TRAP)
            return true;
    }
    return false;
}

#ifdef PROFILE_CYCLES
// For scripts/record-cycles.py: stop at any TRAP and give up well before
// the ring wraps; the debug pin frames the run, to trigger a capture on.
Signals *PinsP8095BH::loop() {
    constexpr auto MAX_CYCLES = 96;
    int16_t tryHalt = 0;
    assert_debug();
    for (auto n = 0;; ++n) {
        auto s = prepareCycle();
        if (s->addr == InstI8096::VEC_TRAP && s->read()) {
            negate_debug();
            handleTrap(s, 0x4566, true);
            return s;
        }
        if (tryHalt) {
            if (s->addr == InstI8096::VEC_EXTINT && s->read()) {
                negateInt();
                handleTrap(s, 0x5678, false);
                return s;
            }
            if (++tryHalt >= 2000) {
                resetPins();
                return s;
            }
        }
        completeCycle(s);
        _devs->loop();
        if ((n >= MAX_CYCLES || haltSwitch()) && tryHalt == 0) {
            negate_debug();
            cli.println(n >= MAX_CYCLES ? "?cycles" : "?halt");
            assertInt();
            tryHalt = 1;
        }
    }
}
#else
Signals *PinsP8095BH::loop() {
    int16_t tryHalt = 0;
    while (true) {
        auto s = prepareCycle();
        if (s->addr == InstI8096::VEC_TRAP && s->read()) {
            for (uint_fast8_t i = 1; i < 10; ++i) {
                if (fetchedBreak(s->prev(i))) {
                    handleTrap(s, 0x3456, true);
                    return s;
                }
            }
            if (_mems->read16(InstI8096::VEC_TRAP) == InstI8096::VEC_TRAP) {
                handleTrap(s, 0x4566, true);
                return s;
            }
        }
        if (tryHalt) {
            if (s->addr == InstI8096::VEC_EXTINT && s->read()) {
                negateInt();
                handleTrap(s, 0x5678, false);
                return s;
            }
            if (++tryHalt >= 2000) {
                resetPins();
                return s;
            }
        }
        completeCycle(s);
        _devs->loop();
        if (haltSwitch() && tryHalt == 0) {
            assertInt();
            tryHalt = 1;
        }
    }
}
#endif

void PinsP8095BH::run() {
    _regs->restore();
    Cycles::reset();
    _idle = false;
    saveBreakInsts();
    startRunTimer();
#ifdef PROFILE_CYCLES
    // Keep the TRAP's cycles, its vector and pushes (1:~:Vr:Ww, a word
    // each): without them the matcher can't place its fetch, the end of the
    // pattern.
    _profileEnd = loop()->next(2);
    stopRunTimer();
#else
    const auto s = loop();
    stopRunTimer();
    Cycles::discard(s);
#endif
    restoreBreakInsts();
    disassembleCycles();
}

void PinsP8095BH::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, InstI8096::TRAP);
}

void PinsP8095BH::assertInt(uint8_t) {
    digitalWriteFast(PIN_EXTINT, HIGH);
}

void PinsP8095BH::negateInt(uint8_t) {
    digitalWriteFast(PIN_EXTINT, LOW);
}

void PinsP8095BH::printCycles(const Signals *end) {
    const auto g = Signals::get();
    const auto cycles = g->diff(end ? end : Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

const Signals *PinsP8095BH::findFetch(Signals *begin, const Signals *end) {
    const CpuMemory cpu(mems<MemsI8096>());
    ArchI8096 arch(&cpu);
    arch.setIdle(
            [](void *pins) { static_cast<PinsP8095BH *>(pins)->idle(); }, this);
    auto &walker = MatchWalker::shared();
    if (!walker.walk(arch, begin, end))
        return end;
    return begin->next(walker.start());
}

// fetch() isn't live here: findFetch() marks it only as a
// side effect of matching decoded instructions against real memory, so
// that has to run -- once, right here -- before backtraceStartFrom()
// counting fetch() cycles means anything. printBacktrace() re-runs it
// over the now-disposed range, which re-derives the same marks.
const SignalsImpl *PinsP8095BH::findBacktraceStart() {
    const auto end = Signals::put();
    const auto begin = findFetch(Signals::get(), end);
    return backtraceStartFrom<Signals>(begin, _lineLimit);
}

void PinsP8095BH::printBacktrace() {
    const auto end = Signals::put();
#ifdef PROFILE_CYCLES
    // Every cycle; the matcher's fetches print as I.
    findFetch(Signals::get(), _profileEnd ? _profileEnd : end);
    cli.println();
    printCycles(end);
#else
    const auto begin = findFetch(Signals::get(), end);
    cli.println();
    printCycles(begin);
    const auto cycles = begin->diff(end);
    for (auto i = 0u; i < cycles;) {
        const auto s = begin->next(i);
        if (s->fetch()) {
            const auto nexti = _mems->disassemble(s->addr, 1);
            const uint_fast8_t len = nexti - s->addr;
            idle();
            // Its fetches, a word bringing two bytes; others print.
            uint_fast8_t j = 0;
            for (uint_fast8_t got = 0; got < len && i + j < cycles; j++) {
                const auto t = s->next(j);
                const uint16_t from = t->addr;
                const uint16_t to = from + t->bytes();
                const auto inside = t->read() && to > s->addr && from < nexti;
                if (inside)
                    got += (to < nexti ? to : nexti) -
                           (from > s->addr ? from : s->addr);
                if (!inside || Debugger.verbose()) {
                    t->print();
                    idle();
                }
            }
            i += j;
        } else {
            s->print();
            idle();
            i++;
        }
    }
#endif
}

}  // namespace p8095bh
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
