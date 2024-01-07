#include "pins_z88.h"
#include "debugger.h"
#include "devs_z88.h"
#include "digital_bus.h"
#include "inst_z88.h"
#include "mems_z88.h"
#include "regs_z88.h"
#include "signals_z88.h"
#include "z88_uart_handler.h"

namespace debugger {
namespace z88 {

struct PinsZ88 Pins;

// clang-format off
/**
 * Z8 External Bus cycle
 *        -|-----T1----|-----T2----|----T3-----|-----T1----|-----T2----|-----T3----|---
 *        _    __    __    __    __    __    __    __    __    __    __    __    __
 * XTAL1   |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|
 *            \     \       \               \     \     \             \         \
 *        _____|     |_______|_______________|_____|     |_____________|_________|_____
 *   #AS       |_____|       |               |     |_____|             |         |
 *        _____|_____________|               |_____|___________________|         |_____
 *   #DS       |             |_______________|     |                   |_________|
 *        _____|___________________________________|
 *  R/#W  _____|                                   |___________________________________
 *        _____|___________________________________|___________________________________
 *    P0  _____X______________A15-18_______________X_______________A15-A8______________
 *             |________                _____      |________ __________________________
 *    P1  -----<__A7-A0_>--------------<D7-D0>-----<_A7-A0__X_____________D7-D0________
 *        _____|___________________________________|___________________________________
 *   #DM  _____X___________________________________X___________________________________
 */
// clang-format on

namespace {
//   TpC: min  50 ns; XTAL1 cycle (20MHz)
//   TwC: min  25 ns; XTAL1 width
//  TwAS: min  65 ns; #AS width
//   TdA: min  35 ns; Address valid to #AS+
//  TdAS: min  65 ns; #AS+ to Address hold
// TwDSR: min 125 ns; #DS read width
// TdDSR: max  80 ns; #DS- to read data
// TwDSW: min  65 ns; #DS write width
//  TdDW: min  10 ns; write data to #DS-
//  TdDS: min   0 ns; #DS+ to write data hold

constexpr auto xtal1_lo_ns = 4;
constexpr auto xtal1_hi_ns = 8;  // 25

inline uint8_t signal_as() {
    return digitalReadFast(PIN_AS);
}

inline uint8_t signal_ds() {
    return digitalReadFast(PIN_DS);
}

void assert_irq0() {
    digitalWriteFast(PIN_IRQ0, LOW);
}

void negate_irq0() {
    digitalWriteFast(PIN_IRQ0, HIGH);
}

void assert_irq1() {
    digitalWriteFast(PIN_IRQ1, LOW);
}

void negate_irq1() {
    digitalWriteFast(PIN_IRQ1, HIGH);
}

void assert_irq2() {
    digitalWriteFast(PIN_IRQ2, LOW);
}

void negate_irq2() {
    digitalWriteFast(PIN_IRQ2, HIGH);
}

void assert_reset() {
    digitalWriteFast(PIN_RESET, LOW);
    negate_irq0();
    negate_irq1();
    negate_irq2();
}

void negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
}

const uint8_t PINS_LOW[] = {
        PIN_XTAL1,
        PIN_RESET,
};

const uint8_t PINS_HIGH[] = {
        PIN_IRQ0,
        PIN_IRQ1,
        PIN_IRQ2,
};

const uint8_t PINS_PULLDOWN[] = {
        PIN_ADDR8,
        PIN_ADDR9,
        PIN_ADDR10,
        PIN_ADDR11,
        PIN_ADDR12,
        PIN_ADDR13,
        PIN_ADDR14,
        PIN_ADDR15,
};

const uint8_t PINS_INPUT[] = {
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

inline void xtal1_lo() {
    digitalWriteFast(PIN_XTAL1, LOW);
}

inline void xtal1_hi() {
    digitalWriteFast(PIN_XTAL1, HIGH);
    UartH.loop();
}

inline void xtal1_cycle_lo() {
    xtal1_hi();
    delayNanoseconds(xtal1_hi_ns);
    xtal1_lo();
}

inline void xtal1_cycle() {
    xtal1_cycle_lo();
    delayNanoseconds(xtal1_lo_ns);
}

}  // namespace

void PinsZ88::reset() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_PULLDOWN, sizeof(PINS_PULLDOWN), INPUT_PULLDOWN);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    assert_reset();
    // #RESET must remain low for a minimum of 18 clocks.
    for (auto i = 0; i < 18 * 2; i++) {
        xtal1_cycle();
    }
    negate_reset();
    // Wait for #AS pulse with #DS high
    while (signal_ds() == LOW) {
        while (signal_as() == LOW)
            xtal1_cycle();
        while (signal_as() != LOW)
            xtal1_cycle();
    }

    Regs.reset();
    Regs.save();
}

Signals *PinsZ88::prepareCycle() {
    auto s = Signals::put();
    s->fetch() = false;
    // Wait for bus cycle
    while (signal_ds() != LOW) {
        if (signal_as() == LOW) {
            // Read address and bus status
            s->getAddr();
        }
        xtal1_cycle_lo();
    }
    s->getDirection();
    // #DS is low
    return s;
}

Signals *PinsZ88::completeCycle(Signals *s) {
    // #DS is low
    if (s->write()) {
        s->getData();
        xtal1_hi();
        if (s->writeMemory()) {
            Memory.write(s->addr, s->data);
        } else {
            ;  // capture data to s->data
        }
        xtal1_lo();
        ++_writes;
    } else {
        if (s->readMemory()) {
            s->data = Memory.read(s->addr);
        } else {
            ;  // inject data from s->data
        }
        xtal1_hi();
        s->outData();
        xtal1_lo();
        _writes = 0;
    }
    delayNanoseconds(xtal1_lo_ns);
    while (signal_ds() == LOW) {
        xtal1_cycle();
    }
    // #DS is high
    Signals::nextCycle();
    busMode(DATA, INPUT);
    return s;
}

Signals *PinsZ88::cycle() {
    return completeCycle(prepareCycle());
}

Signals *PinsZ88::cycle(uint8_t inst) {
    return completeCycle(prepareCycle()->inject(inst));
}

void PinsZ88::execInst(const uint8_t *inst, uint8_t len) {
    execute(inst, len, nullptr, nullptr, 0);
}

uint8_t PinsZ88::captureWrites(const uint8_t *inst, uint8_t len, uint16_t *addr,
        uint8_t *buf, uint8_t max) {
    return execute(inst, len, addr, buf, max);
}

uint8_t PinsZ88::execute(const uint8_t *inst, uint8_t len, uint16_t *addr,
        uint8_t *buf, uint8_t max) {
    uint8_t inj = 0;
    uint8_t cap = 0;
    while (inj < len || cap < max) {
        auto s = prepareCycle();
        if (inj == 0)
            s->fetch() = true;
        if (cap < max)
            s->capture();
        if (inj < len)
            s->inject(inst[inj]);
        completeCycle(s);
        if (s->read()) {
            if (inj < len)
                inj++;
        } else {
            if (cap == 0 && addr)
                *addr = s->addr;
            if (buf && cap < max)
                buf[cap++] = s->data;
        }
    }
    return cap;
}

void PinsZ88::idle() {
    const auto s = cycle(InstZ88::JR);
    cycle(InstZ88::JR_HERE);
    Signals::discard(s);
}

void PinsZ88::loop() {
    while (true) {
        Devs.loop();
        if (!rawStep() || haltSwitch())
            return;
    }
}

void PinsZ88::run() {
    Regs.restore();
    Signals::resetCycles();
    saveBreakInsts();
    loop();
    restoreBreakInsts();
    disassembleCycles();
    Regs.save();
}

void PinsZ88::intrAck(Signals *frame) const {
    for (auto i = 0; i < 7; ++i)
        frame->next(i)->fetch() = false;
}

bool PinsZ88::rawStep() {
    auto s = prepareCycle();
    if (s->write()) {
        // interrupt acknowledge is ongoing
        // finsh saving PC and FLAGS
        while (s->write()) {
            completeCycle(s);
            s = prepareCycle();
        }
        // vector was already fetched before context saving
        intrAck(s->prev(7));
    }
    const auto inst = Memory.raw_read(s->addr);
    const auto cycles = InstZ88::busCycles(inst);
    if (inst == InstZ88::WFI || cycles == 0) {
        completeCycle(s->inject(InstZ88::JR));
        cycle(InstZ88::JR_HERE);
        Signals::discard(s);
        return false;
    }
    auto fetch = s;
    completeCycle(s);
    for (auto c = 1; c < cycles; ++c) {
        s = cycle();
        if (_writes == 3) {
            // interruput is acknowlegded, and vector was fetched.
            intrAck(s->prev(6));
            return true;
        }
    }
    fetch->fetch() = true;
    return true;
}

bool PinsZ88::step(bool show) {
    Regs.restore();
    Signals::resetCycles();
    if (rawStep()) {
        if (show)
            printCycles();
        Regs.save();
        return true;
    }
    return false;
}

void PinsZ88::assertInt(uint8_t name) {
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

void PinsZ88::negateInt(uint8_t name) {
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

void PinsZ88::setBreakInst(uint32_t addr) const {
    Memory.put_inst(addr, InstZ88::WFI);
}

void PinsZ88::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles; i++) {
        g->next(i)->print();
    }
}

void PinsZ88::disassembleCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles;) {
        const auto s = g->next(i);
        if (s->fetch()) {
            const auto len = Memory.disassemble(s->addr, 1) - s->addr;
            i += len;
        } else {
            s->print();
            ++i;
        }
    }
}

}  // namespace z88
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
