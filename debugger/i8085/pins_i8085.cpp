#include "pins_i8085.h"
#include "debugger.h"
#include "devs_i8085.h"
#include "digital_bus.h"
#include "i8085_sio_handler.h"
#include "inst_i8085.h"
#include "mems_i8085.h"
#include "regs_i8085.h"
#include "signals_i8085.h"

namespace debugger {
namespace i8085 {

struct PinsI8085 Pins;

// clang-format off
/**
 * P8085 bus cycle.
 *        __    __    __    __    __    __    __    __    __    __    __    __    __
 *   X1 _|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |
 *      __      \____\      \_____\     \_____\     \_____\     \_____\     \_____\
 *  CLK   |____T|1    |____T|2    |____T|3    |____T|1    |____T|2    |____T|3    |__
 *        _____       \                 \     _____       \                  \    ___
 *  ALE _|     |_______|_________________|___|     |_______|_________________|___|
 *         ___________ |            _____|    _____________|_________________|     __
 *   AD --<___________>|-----------<_____|>--<_____________X_________________>----<__
 *      _______________|                 |_________________|_________________|_______
 *  #RD                |_________________|                 |                 |
 *      ___________________________________________________|                 |_______
 *  #WR                                                    |_________________|
 */
// clang-format on

namespace {

// tCYC: min 320 ns, max 2,000 ns; CLK cycle period
// tXKR: min  20 ns, max   150 ns; X1 rising to CLK rising
// tXKF: min  20 ns, max   150 ns; X1 rising to CLK faling
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

constexpr auto x1_hi_ns = 64;      // 80 ns
constexpr auto x1_lo_ns = 64;      // 80 ns
constexpr auto clk_hi_x1_hi = 42;  // 80 ns
constexpr auto clk_hi_x1_lo = 62;  // 80 ns
constexpr auto clk_lo_x1_hi = 62;  // 80 ns
constexpr auto clk_lo_x1_lo = 50;  // 80 ns
constexpr auto t1_lo_ns = 80;
constexpr auto t2_lo_ns = 4;
constexpr auto t2_hi_ns = 2;

inline void x1_hi() {
    digitalWriteFast(PIN_X1, HIGH);
}

inline void x1_lo() {
    digitalWriteFast(PIN_X1, LOW);
}

inline auto signal_clk() {
    return digitalReadFast(PIN_CLK);
}

inline void assert_trap() {
    digitalWriteFast(PIN_TRAP, HIGH);
}

inline void negate_trap() {
    digitalWriteFast(PIN_TRAP, LOW);
}

inline void assert_intr() {
    digitalWriteFast(PIN_INTR, HIGH);
}

inline void negate_intr() {
    digitalWriteFast(PIN_INTR, LOW);
}

inline auto signal_ale() {
    return digitalReadFast(PIN_ALE);
}

inline auto signal_rd() {
    return digitalReadFast(PIN_RD);
}

inline auto signal_wr() {
    return digitalReadFast(PIN_WR);
}

inline auto signal_inta() {
    return digitalReadFast(PIN_INTA);
}

inline void assert_ready() {
    digitalWriteFast(PIN_READY, HIGH);
}

inline void negate_ready() {
    digitalWriteFast(PIN_READY, LOW);
}

inline auto signal_ready() {
    return digitalReadFast(PIN_READY);
}

inline void negate_hold() {
    digitalWriteFast(PIN_HOLD, LOW);
}

void assert_reset() {
    // Drive RESET condition
    x1_lo();
    digitalWriteFast(PIN_RESET, LOW);
    negate_hold();
    negate_trap();
    negate_intr();
    assert_ready();
    digitalWriteFast(PIN_RST55, LOW);
    digitalWriteFast(PIN_RST65, LOW);
    digitalWriteFast(PIN_RST75, LOW);
}

void negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
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
};

const uint8_t PINS_HIGH[] = {
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
};

inline void x1_cycle() {
    x1_hi();
    delayNanoseconds(x1_hi_ns);
    x1_lo();
    delayNanoseconds(x1_lo_ns);
}

inline void clk_hi() {
    x1_hi();
    delayNanoseconds(clk_hi_x1_hi);
    SioH.loop();
    x1_lo();
}

inline void clk_lo() {
    x1_hi();
    delayNanoseconds(clk_lo_x1_hi);
    x1_lo();
}

inline void clk_cycle() {
    clk_hi();
    delayNanoseconds(clk_hi_x1_lo);
    clk_lo();
    delayNanoseconds(clk_lo_x1_lo);
}

}  // namespace

void PinsI8085::reset() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    Signals::resetCycles();
    assert_reset();
    // Syncronize X1 and CLK.
    while (signal_clk() == LOW)
        x1_cycle();
    while (signal_clk() != LOW)
        x1_cycle();
    // #RESET_IN should be kept low for a minimum of three clock
    // #periods to ensure proper synchronization of the CPU.
    for (auto i = 0; i < 3; i++)
        clk_cycle();
    negate_reset();
    // #RESET_IN is sampled here falling transition of next CLK.
    clk_cycle();
    cycleT1();
    cycleT2Pause();

    _inta = InstI8085::NOP;
    Regs.save();
}

Signals *PinsI8085::cycleT1() const {
    auto signals = Signals::put();
    // Do T4/T5 if any, and confirm T1L by ALE=H
    while (signal_ale() == LOW)
        clk_cycle();
    // T1H
    clk_hi();
    while (signal_ale() == HIGH)
        ;
    signals->getAddress();
    // T2L
    clk_lo();
    return signals;
}

Signals *PinsI8085::cycleT2() const {
    delayNanoseconds(t2_lo_ns);
    auto signals = Signals::put();
    // T2H
    clk_hi();
    delayNanoseconds(t2_hi_ns);
    signals->getDirection();
    // T3L
    clk_lo();
    return signals;
}

Signals *PinsI8085::cycleT2Pause() const {
    negate_ready();
    return cycleT2();
}

Signals *PinsI8085::cycleT2Ready(uint16_t pc) const {
    assert_ready();
    auto signals = cycleT2();
    signals->setAddress(pc);
    return signals;
}

Signals *PinsI8085::cycleT3(Signals *signals) {
    // T3L
    if (signals->memory()) {  // Memory access
        if (signals->read()) {
            if (signals->readMemory()) {
                signals->data = Memory.raw_read(signals->addr);
            }
            busMode(AD, OUTPUT);
            busWrite(AD, signals->data);
        } else if (signals->write()) {
            signals->getData();
            if (signals->writeMemory()) {
                Memory.raw_write(signals->addr, signals->data);
            }
        }
    } else {  // I/O access
        const uint8_t ioaddr = signals->addr;
        if (signals->read()) {
            if (Devs.isSelected(ioaddr)) {
                signals->data = Devs.read(ioaddr);
            } else {
                signals->data = 0;
            }
            busMode(AD, OUTPUT);
            busWrite(AD, signals->data);
        } else if (signals->write()) {
            signals->getData();
            if (Devs.isSelected(ioaddr)) {
                Devs.write(ioaddr, signals->data);
            }
        } else if (signals->vector()) {
            signals->data = _inta;
            busMode(AD, OUTPUT);
            busWrite(AD, signals->data);
            _inta = InstI8085::NOP;
        }
    }
    // T3H
    clk_hi();
    Signals::nextCycle();
    busMode(AD, INPUT);
    // T1L or T4L/T5L
    clk_lo();
    return signals;
}

void PinsI8085::execInst(const uint8_t *inst, uint8_t len) {
    execute(inst, len, nullptr, nullptr, 0);
}

uint8_t PinsI8085::captureWrites(const uint8_t *inst, uint8_t len,
        uint16_t *addr, uint8_t *buf, uint8_t max) {
    return execute(inst, len, addr, buf, max);
}

uint8_t PinsI8085::execute(const uint8_t *inst, uint8_t len, uint16_t *addr,
        uint8_t *buf, uint8_t max) {
    uint8_t inj = 0;
    uint8_t cap = 0;
    while (inj < len || cap < max) {
        auto signals = (inj == 0) ? cycleT2Ready(Regs.nextIp()) : cycleT2();
        if (inj < len)
            signals->inject(inst[inj]);
        if (cap < max)
            signals->capture();
        cycleT3(signals);
        if (signals->read())
            ++inj;
        if (signals->write()) {
            if (cap == 0 && addr)
                *addr = signals->addr;
            if (cap < max && buf)
                buf[cap++] = signals->data;
        }
        delayNanoseconds(t1_lo_ns);
        cycleT1();
    }
    cycleT2Pause();
    return cap;
}

void PinsI8085::idle() {
    Signals::put()->inject(InstI8085::NOP);
    cycleT2Pause();
}

void PinsI8085::loop() {
    while (true) {
        Devs.loop();
        const auto &s = Signals::put();
        if (!rawStep(s->addr, false) || haltSwitch()) {
            cycleT2Pause();
            restoreBreakInsts();
            disassembleCycles();
            Regs.save();
            return;
        }
    }
}

void PinsI8085::run() {
    Regs.restore();
    Signals::resetCycles();
    rawStep(Regs.nextIp(), false);
    assert_ready();
    saveBreakInsts();
    loop();
}

bool PinsI8085::rawStep(uint16_t pc, bool step) {
    if (Memory.raw_read(pc) == InstI8085::HLT) {
        cycleT2Pause();
        return false;
    }
    cycleT3(cycleT2Ready(pc));
    while (true) {
        auto *signals = cycleT1();
        if (signals->fetch())
            break;
        cycleT3(cycleT2());
        delayNanoseconds(t1_lo_ns);
    }
    if (step)
        cycleT2Pause();
    return true;
}

bool PinsI8085::step(bool show) {
    Regs.restore();
    Signals::resetCycles();
    if (rawStep(Regs.nextIp(), true)) {
        if (show)
            printCycles();
        Regs.save();
        return true;
    }
    return false;
}

uint8_t PinsI8085::intrToInta(uint8_t name) {
    switch (name) {
    default:
        return InstI8085::RST0 | (name & 0x38);
    case INTR_RST55:
    case INTR_RST65:
    case INTR_RST75:
    case INTR_TRAP:
        return InstI8085::NOP;
    }
}

void PinsI8085::assertInt(uint8_t name) {
    switch (name) {
    default:
        assert_intr();
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
    case INTR_TRAP:
        assert_trap();
        break;
    case INTR_NONE:
        break;
    }
    _inta = intrToInta(name);
}

void PinsI8085::negateInt(uint8_t name) {
    switch (name) {
    default:
        negate_intr();
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
    case INTR_TRAP:
        negate_trap();
        break;
    case INTR_NONE:
        break;
    }
}

void PinsI8085::setBreakInst(uint32_t addr) const {
    Memory.put_inst(addr, InstI8085::HLT);
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
        if (s->fetch()) {
            const auto next = Memory.disassemble(s->addr, 1);
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
