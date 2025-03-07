#include "pins_i8085.h"
#include "debugger.h"
#include "devs_i8085.h"
#include "inst_i8085.h"
#include "mems_i8080.h"
#include "regs_i8085.h"
#include "signals_i8085.h"

// #define DEBUG(e) e
#define DEBUG(e)

namespace debugger {
namespace i8085 {
// clang-format off
/**
 * P8085 bus cycle.
 *       |----T1-----|----T2-----|----T3-----|----T1-----|----T2-----|----T3-----|
 *       |-T1A-|-T1B-|-T2A-|-T2B-|-T3A-|-T3B-|-T1A-|-T1B-|-T2A-|-T2B-|-T3A-|-T3B-|
 *        __    __    __    __    __    __    __    __    __    __    __    __    __
 *   X1 _|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |
 *      _\      \____\      \_____\     \_____\     \_____\     \_____\     \_____\
 *  CLK   |____T|1    |____T|2    |____T|3    |____T|1    |____T|2    |____T|3    |__
 *        |_____|      \                |     |_____|      \                |    ___
 *  ALE __|     |_______|_______________|_____|     |_______|_______________|___|
 *          ___________ |          _____|    _______________|_______________|     __
 *   AD ---<___________>|---------<_____|>--<_______________X_______________>----<__
 *      ________________|               |___________________|_______________|_______
 *  #RD                 |_______________|                   |               |
 *      ____________________________________________________|               |_______
 *  #WR                                                     |_______________|
 */
// clang-format on

namespace {
//   X1: min 1MHz, max 6MHz
// tCYC: min 320 ns, max 2,000 ns; CLK cycle period
//  tLL: min 140 ns              ; ALE width
//  tLA: min 100 ns              ; Address hold time after ALE
//  tAL: min 115 ns              ; Address valid before trailing ALE
//  tLC: min 130 ns              ; Trailing ALE to leading control
// tLCK: min 100 ns              ; ALE low during CLK high
//  tAC: min 270 ns              ; A8-15 balid to leading control
// tLDR: max 460 ns              ; ALE to valid data for read
// tLDW: max 200 ns              ; ALE to valid data for write
//  tRD: max 300 ns              ; #RD to valid data
//  tWD: min 100 ns              ; data valid after trailing #WR
//  tCC: min 400 ns              ; width of #RD/#WR/#INTA
//  tCA: min 120 ns              ; A8-15 valid after control
//  tCL: min  50 ns              ; trailing control to leading ALE

constexpr auto x1_hi_ns = 57;       // 85
constexpr auto x1_lo_ns = 60;       // 85
constexpr auto x1_lo_ale = 27;      // 85
constexpr auto x1_lo_status = 10;   // 85
constexpr auto x1_hi_inject = 33;   // 85
constexpr auto x1_lo_output = 0;    // 85
constexpr auto x1_hi_get = 17;      // 85
constexpr auto x1_lo_capture = 50;  // 85
constexpr auto x1_hi_next = 10;     // 85

auto signal_ale() {
    return digitalReadFast(PIN_ALE);
}

void assert_ready() {
    digitalWriteFast(PIN_READY, HIGH);
}

void negate_ready() {
    digitalWriteFast(PIN_READY, LOW);
}

void negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
}

void assert_trap() {
    digitalWriteFast(PIN_TRAP, HIGH);
}

void negate_trap() {
    digitalWriteFast(PIN_TRAP, LOW);
}

const uint8_t PINS_LOW[] = {
        PIN_X1,
        PIN_RESET,
        PIN_RST55,
        PIN_RST65,
        PIN_RST75,
        PIN_INTR,
        PIN_TRAP,
        PIN_HOLD,
        PIN_READY,
};

const uint8_t PINS_INPUT[] = {
        PIN_AD0,
        PIN_AD1,
        PIN_AD2,
        PIN_AD3,
        PIN_AD4,
        PIN_AD5,
        PIN_AD6,
        PIN_AD7,
        PIN_ADDR8,
        PIN_ADDR9,
        PIN_ADDR10,
        PIN_ADDR11,
        PIN_ADDR12,
        PIN_ADDR13,
        PIN_ADDR14,
        PIN_ADDR15,
        PIN_S0,
        PIN_S1,
        PIN_ALE,
        PIN_IOM,
        PIN_HLDA,
        PIN_INTA,
        PIN_RD,
        PIN_WR,
        PIN_RESOUT,
};

}  // namespace

PinsI8085::PinsI8085() {
    auto regs = new RegsI8085(this);
    _regs = regs;
    _mems = new i8080::MemsI8080(regs);
    _devs = new DevsI8085();
}

void PinsI8085::x1_hi() const {
    digitalWriteFast(PIN_X1, HIGH);
}

void PinsI8085::x1_lo() const {
    digitalWriteFast(PIN_X1, LOW);
    devs<DevsI8085>()->sioLoop();
}

void PinsI8085::x1_cycle_lo() const {
    x1_hi();
    delayNanoseconds(x1_hi_ns);
    x1_lo();
}

void PinsI8085::x1_cycle() const {
    x1_cycle_lo();
    delayNanoseconds(x1_lo_ns);
}

void PinsI8085::resetPins() {
    // Assert reset condition
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    // #RESET_IN should be kept low for a minimum of three clock
    // #periods to ensure proper synchronization of the CPU.
    for (auto i = 0; i < 3 * 2; i++)
        x1_cycle();
    // #RESET_IN is sampled here falling transition of next CLK.
    negate_reset();
    Signals::resetCycles();
    do {
        delayNanoseconds(x1_lo_ale);
        x1_cycle_lo();
    } while (signal_ale() == LOW);
    prepareCycle();
    _regs->save();
}

Signals *PinsI8085::prepareCycle() const {
    auto s = Signals::put();
    // ALE=H
    // assert_debug();
    s->getAddress();
    // negate_debug();
    x1_cycle_lo();
    s->getStatus();
    delayNanoseconds(x1_lo_status);
    x1_cycle_lo();
    // assert_debug();
    s->getDirection();
    // negate_debug();
    return s;
}

Signals *PinsI8085::resumeCycle(uint16_t pc) const {
    auto s = Signals::put();
    s->addr = pc;
    s->getStatus();
    assert_ready();
    return s;
}

Signals *PinsI8085::completeCycle(Signals *s) const {
    // X1=L
    x1_hi();
    if (s->readEnable()) {
        if (s->memory()) {  // Memory access
            if (s->readMemory()) {
                s->data = _mems->read(s->addr);
            } else {
                delayNanoseconds(x1_hi_inject);
            }
        } else {  // I/O access
            const uint8_t ioaddr = s->addr;
            if (_devs->isSelected(ioaddr)) {
                s->data = _devs->read(ioaddr);
            } else {
                s->data = 0;
            }
        }
        x1_lo();
        // assert_debug();
        s->outData();
        // negate_debug();
        delayNanoseconds(x1_lo_output);
    } else if (s->writeEnable()) {
        delayNanoseconds(x1_hi_get);
        // assert_debug();
        s->getData();
        // negate_debug();
        x1_lo();
        if (s->memory()) {
            if (s->writeMemory()) {
                _mems->write(s->addr, s->data);
            } else {
                delayNanoseconds(x1_lo_capture);
            }
        } else {
            const uint8_t ioaddr = s->addr;
            if (_devs->isSelected(ioaddr)) {
                _devs->write(ioaddr, s->data);
            }
        }
    } else if (s->intAck()) {
        s->data = InstI8085::vec2Inst(_devs->vector());
        x1_lo();
        // assert_debug();
        s->outData();
        // negate_debug();
        delayNanoseconds(x1_lo_output);
    }
    x1_hi();
    Signals::nextCycle();
    delayNanoseconds(x1_hi_next);
    x1_lo();
    Signals::inputMode();
    do {
        delayNanoseconds(x1_lo_ale);
        x1_cycle_lo();
    } while (signal_ale() == LOW);
    // ALE=H
    return s;
}

Signals *PinsI8085::inject(uint8_t data) const {
    return completeCycle(prepareCycle()->inject(data));
}

uint16_t PinsI8085::captureWrites(
        const uint8_t *inst, uint_fast8_t len, uint8_t *buf, uint_fast8_t max) {
    DEBUG(cli.print("@@ captureWrites: len="));
    DEBUG(cli.printDec(len));
    DEBUG(cli.print(" max="));
    DEBUG(cli.printlnDec(max));
    uint16_t addr = 0;
    uint_fast8_t inj = 0;
    uint_fast8_t cap = 0;
    auto s = resumeCycle();
    while (inj < len || cap < max) {
        if (inj < len)
            s->inject(inst[inj]);
        if (cap < max)
            s->capture();
        completeCycle(s);
        if (s->readEnable()) {
            if (inj < len)
                ++inj;
        } else if (s->writeEnable()) {
            if (cap == 0)
                addr = s->addr;
            if (cap < max && buf)
                buf[cap++] = s->data;
        }
        DEBUG(cli.print("@@ captureWrites: inj="));
        DEBUG(cli.printDec(inj));
        DEBUG(cli.print(" cap="));
        DEBUG(cli.printDec(cap));
        DEBUG(cli.print(' '));
        DEBUG(s->print());
        s = prepareCycle();
    }
    negate_ready();
    return addr;
}

void PinsI8085::idle() {
    // READY=L
    x1_cycle_lo();
}

Signals *PinsI8085::loop() const {
    auto s = resumeCycle(_regs->nextIp());
    while (true) {
        completeCycle(s);
        _devs->loop();
        s = prepareCycle();
        if (s->halt()) {
            const auto halt = s->prev();
            Signals::discard(halt);
            return halt;
        }
        if (haltSwitch() && s->fetch()) {
            negate_ready();
            return nullptr;
        }
    }
}

void PinsI8085::run() {
    _regs->restore();
    Signals::resetCycles();
    saveBreakInsts();
    const auto halt = loop();
    restoreBreakInsts();
    disassembleCycles();
    if (halt) {
        const auto pc = halt->addr;
        assert_trap();  // resume from HALT
        do {
            delayNanoseconds(x1_lo_ale);
            x1_cycle_lo();
        } while (signal_ale() == LOW);
        negate_trap();
        regs<RegsI8085>()->saveContext(pc);
    } else {
        _regs->save();
    }
}

bool PinsI8085::rawStep() const {
    auto s = resumeCycle(_regs->nextIp());
    const auto insn = _mems->read(s->addr);
    if (insn == InstI8085::HLT) {
        negate_ready();
        return false;
    }
    s->inject(insn);
    do {
        completeCycle(s);
        s = prepareCycle();
    } while (!s->fetch());
    negate_ready();
    return true;
}

bool PinsI8085::step(bool show) {
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

void PinsI8085::assertInt(uint8_t name) {
    switch (name) {
    default:
        digitalWriteFast(PIN_INTR, HIGH);
        break;
    case INTR_RST55:
        digitalWriteFast(PIN_RST55, HIGH);
        break;
    case INTR_RST65:
        digitalWriteFast(PIN_RST65, HIGH);
        break;
    case INTR_RST75:
        digitalWriteFast(PIN_RST75, HIGH);
        break;
    case INTR_NONE:
        break;
    }
}

void PinsI8085::negateInt(uint8_t name) {
    switch (name) {
    default:
        digitalWriteFast(PIN_INTR, LOW);
        break;
    case INTR_RST55:
        digitalWriteFast(PIN_RST55, LOW);
        break;
    case INTR_RST65:
        digitalWriteFast(PIN_RST65, LOW);
        break;
    case INTR_RST75:
        digitalWriteFast(PIN_RST75, LOW);
        break;
    case INTR_NONE:
        break;
    }
}

void PinsI8085::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

void PinsI8085::disassembleCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles;) {
        const auto s = g->next(i);
        if (s->fetch() && !s->intAck()) {
            const auto next = _mems->disassemble(s->addr, 1);
            i += next - s->addr;
        } else {
            s->print();
            ++i;
        }
        idle();
    }
}

}  // namespace i8085
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
