#include "pins_i8080.h"
#include "debugger.h"
#include "devs_i8080.h"
#include "inst_i8080.h"
#include "mems_i8080.h"
#include "regs_i8080.h"
#include "signals_i8080.h"

namespace debugger {
namespace i8080 {

// clang-format off
/**
 * P8080 bus cycle.
 */
// clang-format on

namespace {

const uint8_t PINS_LOW[] = {
        PIN_PHI1,
        PIN_PHI2,
        PIN_INT,
        PIN_HOLD,
};

const uint8_t PINS_HIGH[] = {
        PIN_RESET,
        PIN_READY,
};

const uint8_t PINS_INPUT[] = {
        PIN_D0,
        PIN_D1,
        PIN_D2,
        PIN_D3,
        PIN_D4,
        PIN_D5,
        PIN_D6,
        PIN_D7,
        PIN_ADDR0,
        PIN_ADDR1,
        PIN_ADDR2,
        PIN_ADDR3,
        PIN_ADDR4,
        PIN_ADDR5,
        PIN_ADDR6,
        PIN_ADDR7,
        PIN_ADDR8,
        PIN_ADDR9,
        PIN_ADDR10,
        PIN_ADDR11,
        PIN_ADDR12,
        PIN_ADDR13,
        PIN_ADDR14,
        PIN_ADDR15,
        PIN_DBIN,
        PIN_WR,
        PIN_SYNC,
        PIN_HLDA,
        PIN_INTE,
        PIN_WAIT,
};

}  // namespace

PinsI8080::PinsI8080() {
    auto regs = new RegsI8080(this);
    _regs = regs;
    _mems = new MemsI8080(regs);
    _devs = new DevsI8080();
}

void PinsI8080::resetPins() {
    // Assert reset condition
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    Signals::resetCycles();
    _regs->save();
}

void PinsI8080::execute(const uint8_t *inst, uint8_t len, uint16_t *addr,
        uint8_t *buf, uint8_t max) {
}

void PinsI8080::idle() {
}

void PinsI8080::loop() {
}

void PinsI8080::run() {
    _regs->restore();
    Signals::resetCycles();
    saveBreakInsts();
    loop();
    restoreBreakInsts();
    disassembleCycles();
    _regs->save();
}

void PinsI8080::suspend() {
}

bool PinsI8080::rawStep() {
    return true;
}

bool PinsI8080::step(bool show) {
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

void PinsI8080::assertInt(uint8_t name) {
}

void PinsI8080::negateInt(uint8_t name) {
}

void PinsI8080::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

void PinsI8080::disassembleCycles() {
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

}  // namespace i8080
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
