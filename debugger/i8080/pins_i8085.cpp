#include "pins_i8085.h"
#include "debugger.h"
#include "devs_i8085.h"
#include "inst_i8085.h"
#include "mems_i8080.h"
#include "regs_i8085.h"
#include "signals_i8085.h"

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

constexpr auto x1_hi_ns = 100;
constexpr auto x1_lo_ns = 100;
constexpr auto clk_hi_x1_lo = 44;    // 80 ns
constexpr auto clk_hi_x1_hi = 60;    // 80 ns
constexpr auto clk_lo_x1_lo = 60;    // 80 ns
constexpr auto clk_lo_x1_hi = 60;    // 80 ns
constexpr auto t1b_hi_ns = 50;       // 80 ns
constexpr auto t2a_hi_ns = 58;       // 80 ns
constexpr auto t2b_hi_ns = 40;       // 80 ns
constexpr auto t3a_hi_capture = 25;  // 80 ns;
constexpr auto t1_lo_ns = 100;

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
        PIN_RESOUT,
};

inline void x1_cycle() {
    x1_lo();
    delayNanoseconds(x1_lo_ns);
    x1_hi();
    delayNanoseconds(x1_hi_ns);
}

inline void clk_hi_nowait() {
    x1_lo();
    delayNanoseconds(clk_lo_x1_lo);
    x1_hi();
}

inline void clk_hi() {
    clk_hi_nowait();
    delayNanoseconds(clk_lo_x1_hi);
}

}  // namespace

PinsI8085::PinsI8085() {
    auto regs = new RegsI8085(this);
    _regs = regs;
    _mems = new i8080::MemsI8080(regs);
    _devs = new DevsI8085();
}

void PinsI8085::clk_lo_nowait() const {
    x1_lo();
    delayNanoseconds(clk_hi_x1_lo);
    devs<DevsI8085>()->sioLoop();
    x1_hi();
}

void PinsI8085::clk_lo() const {
    clk_lo_nowait();
    delayNanoseconds(clk_hi_x1_hi);
}

void PinsI8085::resetPins() {
    // Assert reset condition
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    // Syncronize X1 and CLK.
    while (signal_clk() == LOW)
        x1_cycle();
    while (signal_clk() != LOW)
        x1_cycle();
    // CLK=L
    // #RESET_IN should be kept low for a minimum of three clock
    // #periods to ensure proper synchronization of the CPU.
    for (auto i = 0; i < 5; i++) {
        clk_hi();
        clk_lo();
    }
    negate_reset();
    Signals::resetCycles();
    // #RESET_IN is sampled here falling transition of next CLK.
    cycleT1();
    _regs->save();
}

Signals *PinsI8085::cycleT1() const {
    auto s = Signals::put();
    // Do T4/T5 if any, and confirm T1AL by ALE=H
    // CLK=L
    while (signal_ale() == LOW) {
        clk_hi();
        clk_lo();
    }
    // T1AL
    x1_lo();
    s->getAddress();
    // T1BH
    x1_hi();
    delayNanoseconds(t1b_hi_ns);
    return s;
}

Signals *PinsI8085::cycleT2() const {
    // T2A
    clk_lo_nowait();
    auto s = Signals::put();
    delayNanoseconds(t2a_hi_ns);
    // T2B
    clk_hi_nowait();
    s->getDirection();
    delayNanoseconds(t2b_hi_ns);
    return s;
}

Signals *PinsI8085::cycleT2Pause() const {
    negate_ready();
    Signals::put()->getAddress();
    return cycleT2();
}

Signals *PinsI8085::cycleT2Ready(uint16_t pc) const {
    assert_ready();
    auto s = cycleT2();
    s->setAddress(pc);
    return s;
}

Signals *PinsI8085::cycleT3(Signals *s) const {
    // T3A
    clk_lo_nowait();
    if (s->write()) {
        s->getData();
        if (s->memory()) {
            if (s->writeMemory()) {
                _mems->write_byte(s->addr, s->data);
            } else {
                delayNanoseconds(t3a_hi_capture);
            }
        } else if (s->write()) {
            const uint8_t ioaddr = s->addr;
            if (_devs->isSelected(ioaddr)) {
                _devs->write(ioaddr, s->data);
            }
        }
    } else {
        if (s->memory()) {  // Memory access
            if (s->readMemory()) {
                s->data = _mems->read_byte(s->addr);
            }
        } else {  // I/O access
            const uint8_t ioaddr = s->addr;
            if (s->vector()) {
                s->data = InstI8085::vec2Inst(_devs->vector());
            } else if (_devs->isSelected(ioaddr)) {
                s->data = _devs->read(ioaddr);
            } else {
                s->data = 0;
            }
        }
        s->outData();
    }
    // T3B
    clk_hi_nowait();
    Signals::inputMode();
    Signals::nextCycle();
    // T1AL or T4/T5
    clk_lo();
    while (signal_ale() == LOW) {
        clk_hi();
        clk_lo();
    }
    return s;
}

Signals *PinsI8085::inject(uint8_t data) const {
    cycleT1();
    return cycleT3(cycleT2()->inject(data));
}

uint16_t PinsI8085::captureWrites(
        const uint8_t *inst, uint_fast8_t len, uint8_t *buf, uint_fast8_t max) {
    uint16_t addr = 0;
    uint_fast8_t inj = 0;
    uint_fast8_t cap = 0;
    auto s = cycleT2Ready(_regs->nextIp());
    while (true) {
        if (inj < len)
            s->inject(inst[inj]);
        if (cap < max)
            s->capture();
        cycleT3(s);
        if (s->memory()) {
            if (s->read()) {
                if (inj < len)
                    ++inj;
            } else if (s->write()) {
                if (cap == 0)
                    addr = s->addr;
                if (cap < max && buf)
                    buf[cap++] = s->data;
            }
        }
        delayNanoseconds(t1_lo_ns);
        cycleT1();
        if (inj >= len && cap >= max) {
            cycleT2Pause();
            return addr;
        }
        s = cycleT2();
    }
}

void PinsI8085::idle() {
    cycleT2Pause();
}

void PinsI8085::loop() {
    auto s = cycleT2Ready(_regs->nextIp());
    while (true) {
        cycleT3(s);
        _devs->loop();
        if (haltSwitch()) {
            suspend();
            return;
        }
        s = cycleT1();
        if (s->fetch() && _mems->read_byte(s->addr) == InstI8085::HLT) {
            cycleT2Pause();
            return;
        }
        cycleT2();
    }
}

void PinsI8085::run() {
    _regs->restore();
    Signals::resetCycles();
    saveBreakInsts();
    loop();
    restoreBreakInsts();
    disassembleCycles();
    _regs->save();
}

void PinsI8085::suspend() {
    assert_trap();
    while (true) {
        auto s = cycleT1();
        if (s->fetch() && s->addr == InstI8085::ORG_TRAP) {
            negate_trap();
            cycleT3(cycleT2()->inject(InstI8085::RET));
            inject(s->prev()->data);
            inject(s->prev(2)->data);
            cycleT1();
            Signals::discard(s->prev(3));
            cycleT2Pause();
            return;
        }
        cycleT3(cycleT2());
        delayNanoseconds(t1_lo_ns);
    }
}

bool PinsI8085::rawStep() {
    const auto pc = _regs->nextIp();
    if (_mems->read_byte(pc) == InstI8085::HLT)
        return false;
    assert_trap();
    cycleT3(cycleT2Ready(pc));
    suspend();
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
    case INTR_NONE:
        break;
    }
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

}  // namespace i8085
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
