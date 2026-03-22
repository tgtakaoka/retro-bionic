#include "pins_z280.h"
#include "debugger.h"
#include "devs_z280.h"
#include "mems_z280.h"
#include "regs_z280.h"
#include "signals_z280.h"

namespace debugger {
namespace z280 {

// clang-format off
/**
 * Z280 bus cycle
 *           ___     ___     ___     ___
 *  CLKIN __|   |___|   |___|   |___|   |___
 *        ___\           \__________\
 * CLKOUT    |___________|           |______
 *        ___ _______                 ______
 *   #MEM ___X       |_______________|
 *        ___ _______                 ______
 *   #DEN ___X       |_______________|
 *        ___ ___________             ______
 *    #WE ___X           |___________|
 */

// tMC: nom:  50 ns; CLKIN cycle time
//  tc: nom: 200 ns; CLKOUT cycle time
// clang-format on

namespace {

constexpr auto clkin_lo_ns = 20;      // 25 ns
constexpr auto clkin_hi_ns = 20;      // 25 ns
constexpr auto clkin_hi_cntl = 20;    // 25 ns
constexpr auto clkin_hi_input = 5;    // 25 ns
constexpr auto clkin_hi_inject = 10;  // 25 ns

const uint8_t PINS_LOW[] = {
        PIN_RS,
        PIN_CLKIN,
};

const uint8_t PINS_HIGH[] = {
        PIN_BIO,
        PIN_INT,
};

const uint8_t PINS_INPUT[] = {
        PIN_CLKOUT,
        PIN_MEM,
        PIN_WE,
        PIN_DEN,
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
        PIN_AM4,
        PIN_AM5,
        PIN_AM6,
        PIN_AM7,
        PIN_AH8,
        PIN_AH9,
        PIN_AH10,
        PIN_AH11,
};

inline void clkin_lo() {
    digitalWriteFast(PIN_CLKIN, LOW);
}

inline void clkin_hi() {
    digitalWriteFast(PIN_CLKIN, HIGH);
}

void clkin_cycle_hi() {
    clkin_lo();
    delayNanoseconds(clkin_lo_ns);
    clkin_hi();
}

void clkin_cycle() {
    clkin_cycle_hi();
    delayNanoseconds(clkin_hi_ns);
}

void negate_reset() {
    digitalWriteFast(PIN_RS, HIGH);
}

}  // namespace

PinsZ280::PinsZ280() {
    _devs = new z280::DevsZ280();
    auto regs = new z2801x::RegsZ2801X(this);
    _regs = regs;
    _mems = new MemsZ280(regs);
}

void PinsZ280::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    for (auto i = 0; i < 10 * 2; i++)
        clkin_cycle();
    Signals::resetCycles();
    negate_reset();
    _regs->save();
}

void PinsZ280::idle() {
    auto s = completeCycle(prepareCycle()->inject(InstZ2801X::B));
    const auto addr = s->addr;
    Signals::discard(s);
    Signals::discard(completeCycle(prepareCycle()->inject(addr)));
}

Signals *PinsZ280::prepareCycle() {
    auto s = Signals::put();
    while (!s->getControl()) {
        clkin_cycle_hi();
        delayNanoseconds(clkin_hi_cntl);
    }
    clkin_lo();
    // assert_debug();
    s->getAddr();
    // negate_debug();
    clkin_hi();
    return s;
}

Signals *PinsZ280::completeCycle(Signals *s) {
    clkin_lo();
    if (s->fetch()) {
        if (s->readMemory()) {
            s->data = _mems->read(s->addr);
        } else {
            delayNanoseconds(clkin_hi_inject);
        }
        clkin_hi();
        // assert_debug();
        s->outData();
        // negate_debug();
        clkin_lo();
        Signals::nextCycle();
        clkin_hi();
        delayNanoseconds(clkin_hi_input);
        s->inputMode();
        clkin_cycle_hi();
    } else if (s->read()) {
        if (s->readMemory()) {
            s->data = _devs->read(s->addr & 7);  // IN
        } else {
            delayNanoseconds(clkin_hi_inject);
        }
        clkin_hi();
        // assert_debug();
        s->outData();
        // negate_debug();
        clkin_lo();
        Signals::nextCycle();
        clkin_hi();
        delayNanoseconds(clkin_hi_input);
        s->inputMode();
        clkin_cycle_hi();
    } else if (s->write()) {
        // assert_debug();
        s->getData();
        // negate_debug();
        clkin_hi();
        Signals::nextCycle();
        clkin_lo();
        if (s->writeMemory()) {
            if (s->addr >= 8) {
                _mems->write(s->addr, s->data);  // TBLW
            } else {
                _devs->write(s->addr & 7, s->data);  // OUT
            }
        }
        clkin_hi();
    }
    return s;
}

uint16_t PinsZ280::injectRead(uint16_t data) {
    auto s = prepareCycle();
    completeCycle(s->inject(data));
    return s->addr;
}

uint16_t PinsZ280::captureWrite() {
    const auto s = completeCycle(prepareCycle()->capture());
    return s->data;
}

bool PinsZ280::rawStep() {
    auto s = prepareCycle();
interrupt:
    const auto inst = _mems->read(s->addr);
    const auto cycles = InstZ2801X::cycles(inst);
    if (cycles == 0) {
        completeCycle(s->inject(InstZ2801X::B));
        completeCycle(prepareCycle()->inject(s->addr));
        Signals::discard(s);
        return false;
    }
    completeCycle(s->inject(inst));
    for (uint_fast8_t i = 1; i < cycles; i++) {
        s = prepareCycle();
        if (s->fetch() && s->addr == InstZ2801X::ORG_INT) {
            goto interrupt;
        }
        completeCycle(s);
    }
    return true;
}

bool PinsZ280::step(bool show) {
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

void PinsZ280::loop() {
    while (true) {
        if (!rawStep() || haltSwitch()) {
            auto s = Signals::put();
            _regs->save();
            Signals::discard(s);
            return;
        }
        _devs->loop();
    }
}

void PinsZ280::run() {
    _regs->restore();
    Signals::resetCycles();
    saveBreakInsts();
    loop();
    restoreBreakInsts();
    disassembleCycles();
}

void PinsZ280::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, 0xA000);
}

void PinsZ280::assertInt(uint8_t) {
    digitalWriteFast(PIN_INT, LOW);
}

void PinsZ280::negateInt(uint8_t) {
    digitalWriteFast(PIN_INT, HIGH);
}

void PinsZ280::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

void PinsZ280::disassembleCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles;) {
        const auto s = g->next(i);
        if (s->fetch()) {
            const auto cyc = 0;      // TODO: @@@@@
            const auto len = _mems->disassemble(s->addr, 1) - s->addr;
            for (uint_fast8_t j = len; j < cyc; j++) {
                const auto t = s->next(j);
                t->print();
            }
            i += cyc;
        } else {
            s->print();
            ++i;
        }
        idle();
    }
}

}  // namespace z280
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
