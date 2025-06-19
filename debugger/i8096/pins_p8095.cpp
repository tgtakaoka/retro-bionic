#include "pins_p8095.h"
#include "debugger.h"
#include "devs_i8096.h"
#include "mems_i8096.h"
#include "regs_i8096.h"
#include "signals_p8095.h"

namespace debugger {
namespace p8095 {

using i8096::InstI8096;

// clang-format off
/**
 * P8095 bus cycle
 *           ___     ___     ___     ___     ___     ___     __
 *  XTAK1 __|   |___|   |___|   |___|   |___|   |___|   |___|
 *        ___\           \__________\           \ __________\
 * CLKOUT    |___________|           |___________|           |__
 *        ___ _______                 _______                 __
 *   #MEM ___X       |_______________|       |_______________|
 */

// clang-format on

namespace {

constexpr auto xtal1_lo_ns = 200;  // 25 ns
constexpr auto xtal1_hi_ns = 200;  // 25 ns

const uint8_t PINS_OPENDRAIN[] = {
        PIN_RESET,
};

const uint8_t PINS_HIGH[] = {
        PIN_READY,
};

const uint8_t PINS_LOW[] = {
        PIN_EXTINT,
        PIN_XTAL1,
        PIN_RXD,
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

void xtal1_cycle_hi() {
    xtal1_lo();
    delayNanoseconds(xtal1_lo_ns);
    xtal1_hi();
}

void xtal1_cycle() {
    xtal1_cycle_hi();
    delayNanoseconds(xtal1_hi_ns);
}

void negate_reset() {
    pinMode(PIN_RESET, INPUT_PULLUP);
}

}  // namespace

PinsP8095::PinsP8095() {
    _devs = new i8096::DevsI8096();
    auto regs = new i8096::RegsI8096(this);
    _regs = regs;
    auto mems = new i8096::MemsI8096(_devs, regs);
    _mems = mems;
    _inst = new InstI8096(mems);
}

void PinsP8095::resetPins() {
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

Signals *PinsP8095::prepareCycle() {
    auto s = Signals::put();
    while (!s->addrValid()) {
        xtal1_cycle();
    }
    assert_debug();
    s->getAddr();
    _addr = s->addr;
    negate_debug();
    while (!s->getControl()) {
        xtal1_cycle();
    }
    return s;
}

Signals *PinsP8095::completeCycle(Signals *s) {
    if (s->read()) {
        if (s->readMemory()) {
            s->data = _mems->read(s->addr);
        }
        xtal1_cycle();
        assert_debug();
        s->outData();
        negate_debug();
        xtal1_cycle();
        s->inputMode();
    } else if (s->write()) {
        assert_debug();
        s->getData();
        negate_debug();
        xtal1_cycle();
        if (s->writeMemory()) {
            _mems->write(s->addr, s->data);
        }
        xtal1_cycle();
    }
    Signals::nextCycle();
    while (s->addrValid()) {
        xtal1_cycle();
    }
    return s;
}

uint16_t PinsP8095::injectReads(const uint8_t *data, uint_fast8_t len) {
    uint16_t addr = 0;
    for (uint_fast8_t i = 0; i < len; i++) {
        auto s = prepareCycle();
        completeCycle(s->inject(data[i]));
        if (i == 0)
            addr = s->addr;
    }
    return addr;
}

uint16_t PinsP8095::captureWrites(uint8_t *data, uint_fast8_t len) {
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

Signals *PinsP8095::jumpHere(uint_fast8_t len) {
    constexpr auto disp = -2;
    static constexpr uint8_t SJMP_HERE[] = {
            InstI8096::SJMP(disp),
            lo(disp),
            InstI8096::NOP,
            InstI8096::NOP,
            InstI8096::NOP,
            InstI8096::NOP,
    };
    auto s = Signals::put();
    injectReads(SJMP_HERE, len);
    return s;
}

void PinsP8095::idle() {
    // The maximu duration of READY=L is 1us and useless for idle.
    Signals::discard(jumpHere());
}

bool PinsP8095::rawStep(bool step) {
    if (!_inst->set(_regs->nextIp()))
        return false;
    const auto len = _inst->instLength();
    const auto cycles = _inst->busCycles();
    for (uint8_t i = 0; i < len; i++) {
        completeCycle(prepareCycle());
    }
    if (step) {
        auto n = 4;
        if (len & 1)
            n++;
        jumpHere(n);
    }
    for (uint8_t i = 0; i < cycles; i++) {
        completeCycle(prepareCycle());
    }
    return true;
}

bool PinsP8095::step(bool show) {
    Signals::resetCycles();
    _regs->restore();
    if (show)
        Signals::resetCycles();
    if (rawStep(true)) {
        if (show)
            printCycles();
        _regs->save();
        return true;
    }
    return false;
}

void PinsP8095::loop() {
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

void PinsP8095::run() {
    _regs->restore();
    Signals::resetCycles();
    saveBreakInsts();
    loop();
    restoreBreakInsts();
    disassembleCycles();
}

void PinsP8095::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, 0xA000);
}

void PinsP8095::assertInt(uint8_t) {
    digitalWriteFast(PIN_EXTINT, HIGH);
}

void PinsP8095::negateInt(uint8_t) {
    digitalWriteFast(PIN_EXTINT, LOW);
}

void PinsP8095::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

void PinsP8095::disassembleCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles;) {
        const auto s = g->next(i);
        if (s->fetch()) {
            const auto cyc = 0;  // TODO: @@@@@
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

}  // namespace p8095
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
