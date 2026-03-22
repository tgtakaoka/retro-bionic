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
 *           _   _   _   _
 *  XTALI __| |_| |_| |_| |_
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

constexpr auto xtali_lo_ns = 100;  // 25 ns
constexpr auto xtali_hi_ns = 100;  // 25 ns

const uint8_t PINS_LOW[] = {
        PIN_XTALI,
        PIN_RESET,
};

const uint8_t PINS_HIGH[] = {
        PIN_INTA,
        PIN_NMI,
        PIN_WAIT,

};

const uint8_t PINS_INPUT[] = {
        PIN_CLK,
        PIN_DS,
        PIN_AS,
        PIN_RW,
        PIN_BW,
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
        PIN_ADR16,
        PIN_ADR17,
        PIN_ADR18,
        PIN_ADR19,
        PIN_ADR20,
        PIN_ADR21,
        PIN_ADR22,
        PIN_ADR23,
};

inline void xtali_lo() {
    digitalWriteFast(PIN_XTALI, LOW);
}

inline void xtali_hi() {
    digitalWriteFast(PIN_XTALI, HIGH);
}

void xtali_cycle_hi() {
    xtali_lo();
    delayNanoseconds(xtali_lo_ns);
    xtali_hi();
}

void xtali_cycle() {
    xtali_cycle_hi();
    delayNanoseconds(xtali_hi_ns);
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

}  // namespace

PinsZ280::PinsZ280() {
    _devs = new DevsZ280();
    _regs = new RegsZ280(this);
    _mems = new MemsZ280();
}

void PinsZ280::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    // The #RESET input must be asserted for a minimum of 128
    // processor clock cycles.  The frequency of processor clock is
    // one-half on the frequency of the external clock source.
    for (auto i = 0; i < (128 + 8) * 2; i++)
        xtali_cycle();
    Signals::resetCycles();
    // When #RESET is sampled high (deasserted), the state od the
    // #WAIT input sampled. if #WAIT is asserted, the contents of the
    // AD0~AD7 lines are sampled on the falling edge of the processor
    // clock and loaded into the Bus Timing and Initialization
    // register.
    // |7| 6| 5|4|32|10| CS: 00=1/2 01=1/1 10=1/4 (Bus Clock Select)
    // |1|BS|MP|0|LM|CS| LM: lower 8MB memory wait states
    // MP: multiprocessor mode, BS: bootstrap mode
    assert_wait();
    auto s = Signals::put();
    s->data = 0x80 | 1;  // Bus clock = Processor Clock
    s->outData();
    negate_reset();
    // #WAIT must be be asserted for at least two processor click
    // #cycles after #RESET is deasserted in order for the Bus Timing
    // #and Initialization register.
    for (auto i = 0; i < 4 * 2; i++)
        xtali_cycle();
    _regs->reset();
    _regs->save();
}

void PinsZ280::idle() {
    xtali_cycle();
    xtali_cycle_hi();
}

Signals *PinsZ280::prepareCycle() {
    auto s = Signals::put();
    return s;
}

Signals *PinsZ280::completeCycle(Signals *s) {
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
    digitalWriteFast(PIN_INTA, LOW);
}

void PinsZ280::negateInt(uint8_t) {
    digitalWriteFast(PIN_INTA, HIGH);
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
        s->print();
        ++i;
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
