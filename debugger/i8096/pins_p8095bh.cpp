#include "pins_p8095bh.h"
#include "debugger.h"
#include "devs_i8096.h"
#include "inst_i8096.h"
#include "mems_i8096.h"
#include "regs_i8096.h"
#include "signals_p8095bh.h"

namespace debugger {
namespace p8095bh {

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
    Signals::resetCycles();
    negate_reset();
    _regs->reset();
    _regs->save();
}

Signals *PinsP8095BH::prepareCycle() {
    auto s = _idle ? &_idleSignals : Signals::put();
    noInterrupts();
    // assert_debug();
    xtal1_hi();
    while (!s->getAddrValid()) {
        xtal1_lo();
        delayNanoseconds(xtal1_lo_ns);
        xtal1_hi();
    }
    // negate_debug();
    s->getAddr();
    xtal1_lo();
    // assert_debug();
    while (!s->getControl()) {
        xtal1_cycle_lo();
    }
    // negate_debug();
    interrupts();
    return s;
}

Signals *PinsP8095BH::completeCycle(Signals *s) {
    xtal1_hi();
    if (s->read()) {
        if (s->readMemory()) {
            s->data = _mems->read(s->addr);
        } else {
            delayNanoseconds(xtal1_hi_ns);
        }
        xtal1_lo();
        // assert_debug();
        s->outData();
        // negate_debug();
        xtal1_hi();
        s->inputMode();
    } else if (s->write()) {
        // assert_debug();
        s->getData();
        // negate_debug();
        xtal1_lo();
        if (s->writeMemory()) {
            _mems->write(s->addr, s->data);
        } else {
            delayNanoseconds(xtal1_lo_ns);
        }
        xtal1_hi();
    }
    if (_idle) {
        delayNanoseconds(xtal1_hi_ns);
    } else {
        Signals::nextCycle();
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

uint16_t PinsP8095BH::injectReads(
        const uint8_t *data, uint_fast8_t len, bool idle) {
    _idle = idle;
    uint16_t addr = 0;
    for (uint_fast8_t i = 0; i < len; i++) {
        auto s = prepareCycle();
        completeCycle(s->inject(data[i]));
        if (i == 0)
            addr = s->addr;
    }
    return addr;
}

uint16_t PinsP8095BH::captureWrites(uint8_t *data, uint_fast8_t len) {
    _idle = false;
    uint16_t addr = 0;
    for (uint_fast8_t i = 0; i < len; i++) {
        auto s = prepareCycle();
        completeCycle(s->capture());
        data[i] = s->data;
        if (i == 0)
            addr = s->addr;
    }
    return addr;
}

Signals *PinsP8095BH::jumpHere(uint_fast8_t len, bool idle) {
    constexpr auto disp = -2;
    static constexpr uint8_t SJMP_HERE[] = {
            SJMP(disp),  // SJMP $-2
            0xFD,        // NOP
            0xFD,        // NOP
            0xFD,        // NOP
            0xFD,        // NOP
    };
    auto s = Signals::put();
    injectReads(SJMP_HERE, len, idle);
    return s;
}

void PinsP8095BH::idle() {
    _idle = true;
    // The maximu duration of READY=L is 1us and useless for idle.
    Signals::discard(jumpHere(4, true));
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
    if (!inst.set(_regs->nextIp(), mems<MemsI8096>()))
        return false;
    const auto len = inst.instLength();
    const auto next = _regs->nextIp() + len;
    const auto opc = inst.opc();
    const auto target = jumpTarget(next, opc);
    auto stepTrap = opc == InstI8096::TRAP;
    _regs->restore();
    if (show)
        Signals::resetCycles();
    for (uint_fast8_t i = 0; i < len; i++) {
        auto s = completeCycle(prepareCycle());
        if (i == 0)
            s->markFetch();
    }
    for (uint_fast8_t i = 0; i < 20; i++) {
        auto s = prepareCycle();
        if (s->read()) {
            if (s->addr == next) {
                s->inject(InstI8096::TRAP);
            } else if (s->addr == target) {
                s->inject(InstI8096::TRAP);
            } else if (s->addr == InstI8096::VEC_TRAP) {
                if (stepTrap) {
                    stepTrap = false;
                } else {
                    handleTrap(s, 0x2345, true);
                    if (show)
                        Signals::discard(s);
                    break;
                }
            }
        }
        completeCycle(s);
    }
    return true;
}

bool PinsP8095BH::step(bool show) {
    Signals::resetCycles();
    if (rawStep(show)) {
        if (show)
            printCycles();
        return true;
    }
    return false;
}

void PinsP8095BH::handleTrap(Signals *s, uint16_t vector, bool breakTrap) {
    completeCycle(s->inject(lo(vector)));
    injectRead(hi(vector));
    regs<RegsI8096>()->captureContext(breakTrap);
}

Signals *PinsP8095BH::loop() {
    int16_t tryHalt = 0;
    while (true) {
        auto s = prepareCycle();
        if (s->addr == InstI8096::VEC_TRAP && s->read()) {
            for (uint_fast8_t i = 3; i < 10; ++i) {
                const auto trap = s->prev(i);
                if (isBreakPoint(trap->addr) && trap->read() &&
                        trap->data == InstI8096::TRAP) {
                    handleTrap(s, 0x3456, true);
                    return s;
                }
            }
            if (_mems->read16(InstI8096::VEC_TRAP) == InstI8096::VEC_TRAP) {
                handleTrap(s, 0x4567, true);
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

void PinsP8095BH::run() {
    _regs->restore();
    Signals::resetCycles();
    saveBreakInsts();
    const auto s = loop();
    Signals::discard(s);
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
    for (auto i = 0; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

bool PinsP8095BH::matchAll(Signals *begin, const Signals *end) {
    const auto cycles = begin->diff(end->prev());
    LOG_MATCH(cli.print("@@  matchAll: begin="));
    LOG_MATCH(begin->print());
    LOG_MATCH(cli.print("@@           cycles="));
    LOG_MATCH(cli.printlnDec(cycles));
    for (auto i = 0; i < cycles;) {
        idle();
        auto s = begin->next(i);
        while (s->write() || s->isOperand()) {
            LOG_MATCH(cli.print("@@             skip="));
            LOG_MATCH(s->print());
            s = s->next();
            i++;
        }
        InstI8096 inst;
        if (inst.match(s, end, mems<MemsI8096>())) {
            i += inst.nexti();
            continue;
        }
        return inst.isEnd();
    }
    return true;
}

const Signals *PinsP8095BH::findFetch(Signals *begin, const Signals *end) {
    const auto cycles = begin->diff(end);
    const auto limit = cycles >= 14 ? 14 : cycles;
    LOG_MATCH(cli.println());
    LOG_MATCH(cli.print("@@ findFetch: cycle="));
    LOG_MATCH(cli.printlnDec(cycles));
    LOG_MATCH(cli.print("@@            begin="));
    LOG_MATCH(begin->print());
    LOG_MATCH(cli.print("@@              end="));
    LOG_MATCH(end->print());
    for (auto i = 0; i < limit; ++i) {
        for (auto j = 0; j < cycles; ++j)
            begin->next(j)->clearMark();
        auto s = begin->next(i);
        if (matchAll(s, end))
            return s;
        idle();
    }
    for (auto j = 0; j < cycles; ++j)
        begin->next(j)->clearMark();
    return end;
}

void PinsP8095BH::disassembleCycles() {
    const auto end = Signals::put();
    const auto begin = findFetch(Signals::get(), end);
    cli.println();
    printCycles(begin);
    const auto cycles = begin->diff(end);
    for (auto i = 0; i < cycles;) {
        const auto s = begin->next(i);
        if (s->fetch()) {
            const auto nexti = _mems->disassemble(s->addr, 1);
            const uint_fast8_t len = nexti - s->addr;
            idle();
            for (uint_fast8_t j = 0; j < len; j++) {
                const auto t = s->next(j);
                if (t->addr < s->addr || t->addr >= nexti ||
                        Debugger.verbose()) {
                    t->print();
                    idle();
                }
            }
            i += len;
        } else {
            s->print();
            idle();
            i++;
        }
    }
}

}  // namespace p8095bh
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
