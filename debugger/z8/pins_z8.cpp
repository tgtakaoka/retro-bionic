#include "pins_z8.h"
#include "debugger.h"
#include "digital_bus.h"
#include "inst_z8.h"
#include "mems_z8.h"
#include "regs_z8.h"

namespace debugger {
namespace z8 {

namespace {
void inline assert_irq0() {
    digitalWriteFast(PIN_IRQ0, LOW);
}

void inline negate_irq0() {
    digitalWriteFast(PIN_IRQ0, HIGH);
}

void inline assert_irq1() {
    digitalWriteFast(PIN_IRQ1, LOW);
}

void inline negate_irq1() {
    digitalWriteFast(PIN_IRQ1, HIGH);
}

void inline assert_irq2() {
    digitalWriteFast(PIN_IRQ2, LOW);
}

void inline negate_irq2() {
    digitalWriteFast(PIN_IRQ2, HIGH);
}

void inline negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
}

inline auto signal_as() {
    return digitalReadFast(PIN_AS);
}

inline auto signal_ds() {
    return digitalReadFast(PIN_DS);
}

constexpr uint8_t PINS_LOW[] = {
        PIN_XTAL1,
        PIN_RESET,
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_IRQ0,
        PIN_IRQ1,
        PIN_IRQ2,
};

constexpr uint8_t PINS_PULLDOWN[] = {
        PIN_ADDR8,
        PIN_ADDR9,
        PIN_ADDR10,
        PIN_ADDR11,
        PIN_ADDR12,
        PIN_ADDR13,
        PIN_ADDR14,
        PIN_ADDR15,
};

constexpr uint8_t PINS_INPUT[] = {
        PIN_DATA0,
        PIN_DATA1,
        PIN_DATA2,
        PIN_DATA3,
        PIN_DATA4,
        PIN_DATA5,
        PIN_DATA6,
        PIN_DATA7,
        PIN_AS,
        PIN_DS,
        PIN_RW,
        PIN_DM,
};

}  // namespace

void PinsZ8::reset() {
    // Assert reset condition
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_PULLDOWN, sizeof(PINS_PULLDOWN), INPUT_PULLDOWN);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    // #RESET must remain low for a minimum of 16 clocks.
    for (auto i = 0; i < 16 * 2; i++) {
        xtal1_cycle();
        delayNanoseconds(1);
    }
    negate_reset();
    Signals::resetCycles();
    // Wait for #AS pulse with #DS=H
    while (signal_ds() == LOW) {
        while (signal_as() == LOW)
            xtal1_cycle();
        while (signal_as() != LOW)
            xtal1_cycle();
    }

    _regs.reset();
    _regs.save();
}

Signals *PinsZ8::prepareCycle() {
    auto s = Signals::put();
    s->fetch() = false;
    // Wait for bus cycle
    while (signal_ds() != LOW) {
        if (signal_as() == LOW) {
            // Read address and bus status
            s->getAddr();
        }
        xtal1_cycle();
    }
    // #DS is low
    return s;
}

Signals *PinsZ8::completeCycle(Signals *s) {
    // #DS is low
    if (s->write()) {
        ++_writes;
        s->getData();
        if (s->writeMemory()) {
            _mems.write(s->addr, s->data);
        } else {
            ;  // capture data to s->data
        }
    } else {
        _writes = 0;
        if (s->readMemory()) {
            s->data = _mems.read(s->addr);
        } else {
            ;  // inject data from s->data
        }
        xtal1_cycle();
        s->outData();
    }
    while (signal_ds() == LOW) {
        xtal1_cycle();
    }
    // #DS is high
    Signals::nextCycle();
    busMode(DATA, INPUT);
    return s;
}

Signals *PinsZ8::cycle() {
    return completeCycle(prepareCycle());
}

Signals *PinsZ8::cycle(uint8_t inst) {
    return completeCycle(prepareCycle()->inject(inst));
}

bool PinsZ8::rawStep() {
    auto s = prepareCycle();
    if (s->write()) {
        // interrupt acknowledge is ongoing
        // finsh saving PC and FLAGS
        while (s->write()) {
            completeCycle(s);
            s = prepareCycle();
        }
        if (fetchVectorAfterContextSave()) {
            completeCycle(s);
            s = cycle();
            intrAck(s->prev(6));
            s = prepareCycle();
        } else {
            intrAck(s->prev(7));
        }
    }
    const auto inst = _mems.raw_read(s->addr);
    const auto cycles = _inst.busCycles(inst);
    if (_inst.isBreak(inst) || cycles == 0) {
        completeCycle(s->inject(InstZ8::JR));
        cycle(InstZ8::JR_HERE);
        Signals::discard(s);
        return false;
    }
    auto fetch = s;
    completeCycle(s);
    for (auto c = 1; c < cycles; ++c) {
        s = cycle();
        if (_writes == 3) {
            if (fetchVectorAfterContextSave()) {
                cycle();
                s = cycle();
            }
            intrAck(s->prev(6));
            return true;
        }
    }
    fetch->fetch() = true;
    return true;
}

void PinsZ8::execInst(const uint8_t *inst, uint8_t len) {
    execute(inst, len, nullptr, nullptr, 0);
}

void PinsZ8::captureWrites(const uint8_t *inst, uint8_t len, uint16_t *addr,
        uint8_t *buf, uint8_t max) {
    execute(inst, len, addr, buf, max);
}

void PinsZ8::execute(const uint8_t *inst, uint8_t len, uint16_t *addr,
        uint8_t *buf, uint8_t max) {
    uint8_t inj = 0;
    uint8_t cap = 0;
    while (inj < len || cap < max) {
        Signals *s = prepareCycle();
        if (cap < max)
            s->capture();
        if (inj < len)
            s->inject(inst[inj]);
        completeCycle(s);
        if (s->read()) {
            if (inj < len) {
                inj++;
            }
        } else {
            if (cap == 0 && addr)
                *addr = s->addr;
            if (buf && cap < max) {
                buf[cap++] = s->data;
            }
        }
    }
}

void PinsZ8::idle() {
    const auto s = cycle(InstZ8::JR);
    cycle(InstZ8::JR_HERE);
    Signals::discard(s);
}

void PinsZ8::loop() {
    while (true) {
        _devs.loop();
        if (!rawStep() || haltSwitch())
            return;
    }
}

void PinsZ8::run() {
    _regs.restore();
    Signals::resetCycles();
    saveBreakInsts();
    loop();
    restoreBreakInsts();
    disassembleCycles();
    _regs.save();
}

void PinsZ8::intrAck(Signals *frame) const {
    for (auto i = 0; i < 7; ++i)
        frame->next(i)->fetch() = false;
}

bool PinsZ8::step(bool show) {
    Signals::resetCycles();
    _regs.restore();
    if (show)
        Signals::resetCycles();
    if (rawStep()) {
        if (show)
            printCycles();
        _regs.save();
        return true;
    }
    return false;
}

void PinsZ8::assertInt(uint8_t name) {
    switch (name) {
    default:
        break;
    case INTR_IRQ0:
        assert_irq0();
        break;
    case INTR_IRQ1:
        assert_irq1();
        break;
    case INTR_IRQ2:
        assert_irq2();
        break;
    }
}

void PinsZ8::negateInt(uint8_t name) {
    switch (name) {
    default:
        break;
    case INTR_IRQ0:
        negate_irq0();
        break;
    case INTR_IRQ1:
        negate_irq1();
        break;
    case INTR_IRQ2:
        negate_irq2();
        break;
    }
}

void PinsZ8::setBreakInst(uint32_t addr) const {
    _mems.put_inst(addr, _inst.breakInst());
}

void PinsZ8::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles; i++) {
        g->next(i)->print();
    }
}

void PinsZ8::disassembleCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles;) {
        const auto s = g->next(i);
        if (s->fetch()) {
            const auto len = _mems.disassemble(s->addr, 1) - s->addr;
            i += len;
        } else {
            s->print();
            ++i;
        }
    }
}

}  // namespace z8
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
