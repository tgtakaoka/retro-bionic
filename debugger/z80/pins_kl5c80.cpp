#include "pins_kl5c80.h"
#include "debugger.h"
#include "devs_z80.h"
#include "inst_z80.h"
#include "mems_z80.h"
#include "regs_z80.h"
#include "signals_kl5c80.h"

namespace debugger {
namespace kl5c80 {

using z80::InstZ80;
using z80::MemsZ80;

// clang-format off
/**
 * KL5C80 bus cycle.
 *          |   T1  |   T2  |   T3  |   T4  |   T1  |   T2  |   T3  |   T1  |   T2  |   T3  |
 *          |___    |___    |___    |___    |___    |___    |___    |___    |___    |___    |___
 *  CLK  ___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|
 *       ___\ __\___________\ __\___________\ __\_______________\____\__\_______\___________\ __
 * ADDR  ____X___|___________X___|___________X___|_______________|___X___|_______|___________X__
 *       ____|   |           |___|___________|___|_______________|_______|_______|___________|__
 *  #M1      |___|___________|   |           |   |               |       |       |           |
 *       ________|           |___|           |___|               |_______|       |           |__
 * #MREQ         |___________|   |___________|   |_______________|       |_______|___________|
 *       ________|___________|               |___|_______________|_______________|___________|__
 * #REFS         |           |_______________|   |               |               |           |
 *       ________|           |___________________|               |_______________|___________|__
 *  #RD          |___________|                   |_______________|               |           |
 *       ________________________________________________________________________|           |__
 *  #WR                                                                          |___________|
 *                          __                                  __          __________________
 * DATA  ------------------<__>--------------------------------<__>--------<__________________>--
 *
 *          |   T1  |   T2  |   TW  |   TW  |   T3  |   T4  |   T1  |   T2  |   TW  |   T3  |
 *          |___    |___    |___    |___    |___    |___    |___    |___    |___    |___    |___
 *  CLK  ___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|
 *       ___\___________________\___________\ __\___________\_______\___________________\____\__
 * ADDR  ____X___________________|___________X___|___________X_______|___________________|___X__
 *       ____|                   |           |___|___________|_______|___________________|______
 *  #M1      |___________________|___________|   |           |       |                   |
 *       ________________________|___________|___|           |_______|___________________|______
 * #REFS                         |           |   |___________|       |                   |
 *       ________________________|___________|___|           |_______|___________________|______
 * #MREQ                         |           |   |___________|       |                   |
 *       ________________________|           |_______________________|                   |______
 * #IORQ                         |___________|                       |___________________|
 *       ____________________________________________________________|                   |______
 *  #RD                                                              |___________________|
 *       ____________________________________________________________|                   |______
 *  #WR                                                              |___________________|
 *                                          __                     ____________________ __
 * DATA  ----------------------------------<__>-------------------<____________________<__>-----
 */
// clang-format on

namespace {

constexpr auto xin_hi_ns = 100;
constexpr auto xin_lo_ns = 100;

inline void xin_hi() {
    digitalWriteFast(PIN_XIN, HIGH);
}

inline void xin_lo() {
    digitalWriteFast(PIN_XIN, LOW);
}

inline void assert_nmi() {
    digitalWriteFast(PIN_NMI, LOW);
}

inline void negate_nmi() {
    digitalWriteFast(PIN_NMI, HIGH);
}

inline void assert_int() {
    digitalWriteFast(PIN_IR0, HIGH);
}

inline void negate_int() {
    digitalWriteFast(PIN_IR0, LOW);
}

inline void assert_ready() {
    digitalWriteFast(PIN_ERDY, HIGH);
}

inline void negate_ready() {
    digitalWriteFast(PIN_ERDY, LOW);
}

void negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
}

constexpr uint8_t PINS_LOW[] = {
        PIN_XIN,
        PIN_RESET,
        PIN_IR0,
        PIN_ASEL,
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_ERDY,
        PIN_BREQ,
        PIN_NMI,
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
        PIN_CLK,
        PIN_M1,
        PIN_EMRD,
        PIN_EMWR,
        PIN_EIORD,
        PIN_EIOWR,
        PIN_BACK,
};

inline void xin_cycle_lo() {
    xin_hi();
    delayNanoseconds(xin_hi_ns);
    xin_lo();
}

inline void xin_cycle() {
    xin_cycle_lo();
    delayNanoseconds(xin_lo_ns);
}

}  // namespace

PinsKl5c80::PinsKl5c80() : PinsZ80Base() {
    mems<MemsZ80>()->setMaxAddr(0xFFFFF);  // 1MB
}

void PinsKl5c80::resetPins() {
    // Assert reset condition
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    // #RESET must be active for at least three CLK cycles for CPU to
    // properly accept it.
    for (auto i = 0; i < 100; i++)
        xin_cycle();
    negate_reset();
    // Once #RESET goes inactive, two internal T cycles are consumed
    // before the CPU resumes normal processing operation.
    while (true)
        xin_cycle();
    Signals::resetCycles();
    prepareWait();
    // TODO: Set SCR1(3BH) to 0FH (enable #M1, #HALT, #BREQ, #BACK)
    _regs->setIp(InstZ80::ORG_RESET);
    _regs->save();
}

Signals *PinsKl5c80::prepareCycle() const {
    const auto s = Signals::put();
    do {
        delayNanoseconds(xin_lo_ns);
        // TnH
        xin_hi();
        delayNanoseconds(xin_hi_ns);
        s->getAddress();
        // TnL
        xin_lo();
        delayNanoseconds(xin_lo_ns);
        s->getControl();
    } while (s->nobus());
    // #MREQ:T1L, #IORQ:T2L/Twa1L
    return s;
}

Signals *PinsKl5c80::completeCycle(Signals *s) const {
    // #MREQ:T2H, #IORQ:TwaH/Twa2H
    xin_hi();
    if (s->mrd()) {  // Memory read or write cycles
        if (s->readMemory()) {
            s->data = _mems->read(s->addr);
            delayNanoseconds(xin_hi_ns);
        } else {
            delayNanoseconds(xin_hi_ns);
        }
        // #MREQ:T2L
        xin_lo();
        s->outData();
        delayNanoseconds(xin_lo_ns);
        // #MREQ:T3H
        xin_hi();
        Signals::nextCycle();
        delayNanoseconds(xin_hi_ns);
    } else if (s->mwr()) {
        delayNanoseconds(xin_hi_ns);
        Signals::nextCycle();
        // #MREQ:T2L
        xin_lo();
        delayNanoseconds(xin_lo_ns);
        s->getData();
        // #MREQ:T3H
        xin_hi();
        if (s->writeMemory()) {
            _mems->write(s->addr, s->data);
            delayNanoseconds(xin_hi_ns);
        } else {
            delayNanoseconds(xin_hi_ns);
        }
    } else {  // Input or output cycles
        delayNanoseconds(xin_hi_ns);
        const uint8_t ioaddr = s->addr;
        // #IORQ:TwaL/Twa2L
        xin_lo();
        if (s->iord()) {
            if (_devs->isSelected(ioaddr))
                s->data = _devs->read(ioaddr);
            s->outData();
        } else if (s->iowr()) {
            s->getData();
            if (_devs->isSelected(ioaddr))
                _devs->write(ioaddr, s->data);
        }
        // T3H
        xin_hi();
        Signals::nextCycle();
        delayNanoseconds(xin_hi_ns);
    }
    // T3L
    xin_lo();
    delayNanoseconds(xin_lo_ns);
    s->inputMode();
    if (s->fetch()) {
        delayNanoseconds(xin_lo_ns);
        // T4H
        xin_hi();
        delayNanoseconds(xin_hi_ns);
        // T4L
        xin_lo();
    }
    return s;
}

Signals *PinsKl5c80::prepareWait() const {
    negate_ready();
    const auto s = prepareCycle();
    // #MREQ:T2, #IORQ:Twa/Twa2
    xin_cycle();
    return s;
}

Signals *PinsKl5c80::resumeCycle(uint16_t pc) const {
    const auto s = Signals::put();
    assert_ready();
    s->setAddress(pc);
    s->getControl();
    return completeCycle(s);
}

Signals *PinsKl5c80::inject(uint8_t data) const {
    return completeCycle(prepareCycle()->inject(data));
}

uint16_t PinsKl5c80::execute(
        const uint8_t *inst, uint_fast8_t len, uint8_t *buf, uint_fast8_t max) {
    uint16_t addr = 0;
    uint8_t inj = 0;
    uint8_t cap = 0;
    auto s = Signals::put();
    while (true) {
        if (inj < len)
            s->inject(inst[inj]);
        if (cap < max)
            s->capture();
        if (inj == 0) {
            resumeCycle(_regs->nextIp());
        } else {
            completeCycle(s);
        }
        if (s->mrd()) {
            if (inj < len)
                ++inj;
        } else if (s->mwr()) {
            if (cap == 0)
                addr = s->addr;
            if (cap < max && buf)
                buf[cap++] = s->data;
        }
        if (inj >= len && cap >= max) {
            prepareWait();
            return addr;
        }
        delayNanoseconds(xin_lo_ns);
        s = prepareCycle();
    }
}

void PinsKl5c80::idle() {
    // #READY=L
    xin_cycle_lo();
}

void PinsKl5c80::loop() {
    resumeCycle(_regs->nextIp());
    while (true) {
        const auto s = prepareCycle();
        if (s->fetch() && _mems->read_byte(s->addr) == InstZ80::HALT) {
            completeCycle(s->inject(InstZ80::JR));
            inject(InstZ80::JR_HERE);
            prepareWait();
            return;
        }
        completeCycle(s);
        _devs->loop();
        if (haltSwitch()) {
            suspend();
            return;
        }
    }
}

void PinsKl5c80::run() {
    _regs->restore();
    Signals::resetCycles();
    saveBreakInsts();
    loop();
    restoreBreakInsts();
    disassembleCycles();
    _regs->save();
}

void PinsKl5c80::suspend() {
    assert_nmi();
    while (true) {
        const auto s = prepareCycle();
        if (s->fetch() && s->addr == InstZ80::ORG_NMI) {
            negate_nmi();
            completeCycle(s->inject(InstZ80::RETN_PREFIX));
            inject(InstZ80::RETN);
            inject(s->prev()->data);
            inject(s->prev(2)->data);
            Signals::discard(s->prev(3));
            prepareWait();
            return;
        }
        completeCycle(s);
    }
}

bool PinsKl5c80::rawStep() {
    const auto pc = _regs->nextIp();
    if (_mems->read_byte(pc) == InstZ80::HALT)
        return false;
    assert_nmi();
    resumeCycle(pc);
    suspend();
    return true;
}

bool PinsKl5c80::step(bool show) {
    Signals::resetCycles();
    _regs->restore();
    if (show)
        Signals::resetCycles();
    if (rawStep()) {
        if (show)
            printCycles();
        _regs->save();
        return true;
    }
    return false;
}

void PinsKl5c80::assertInt(uint8_t name) {
    assert_int();
}

void PinsKl5c80::negateInt(uint8_t name) {
    negate_int();
}

void PinsKl5c80::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

void PinsKl5c80::disassembleCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles;) {
        const auto s = g->next(i);
        if (s->fetch()) {
            const auto next = _mems->disassemble(s->addr, 1);
            i += next - s->addr;
        } else {
            s->print();
            ++i;
        }
        idle();
    }
}

}  // namespace kl5c80
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
