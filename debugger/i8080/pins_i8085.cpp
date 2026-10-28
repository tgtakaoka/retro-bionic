#include "pins_i8085.h"
#include "debugger.h"
#include "devs_i8085.h"
#include "digital_bus.h"
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
//  tAC: min 270 ns              ; A8-15 valid to leading control
// tLDR: max 460 ns              ; ALE to valid data for read
// tLDW: max 200 ns              ; ALE to valid data for write
//  tRD: max 300 ns              ; #RD to valid data
//  tWD: min 100 ns              ; data valid after trailing #WR
//  tCC: min 400 ns              ; width of #RD/#WR/#INTA
//  tCA: min 120 ns              ; A8-15 valid after control
//  tCL: min  50 ns              ; trailing control to leading ALE

// X1 runs free from the PWM, and the CPU waits in each cycle that
// samples READY low. READY goes low as ALE starts a cycle and high once
// the cycle is served, and stays high until the next ALE: the bus idle
// cycles of DAD have no ALE, so they never wait.

// The 8085AH-2's rated X1; a plain 8085A is rated to 6.25 MHz.
constexpr uint32_t x1_hz = 10000000;  // 150 MHz / 15: halves of 47/53 ns
// tCYC max 2,000 ns; CLK is X1/2.
constexpr uint32_t x1_min_hz = 1000000;
#ifdef TUNE_CLOCK
// K may overclock X1 past tCYC min, down to halves of 40 ns.
constexpr auto x1_half_min_ns = 40;
#else
// tCYC min 200 ns (8085AH-2): X1 at most 10 MHz.
constexpr auto x1_half_min_ns = 46;
#endif
// Teensy cycles in an X1 clock at the slowest rate, so the waits below
// hold at any rate.
constexpr uint32_t x1_max_cycles = F_CPU / x1_min_hz;
// The halt switch polls USB through yield(), longer than a cycle's
// service; once in this many Teensy cycles is soon enough.
constexpr uint32_t halt_poll_cycles = F_CPU / 100000;
// #RESET_IN should be kept low for a minimum of three clock periods.
constexpr auto reset_ns = 2 * 10 * (1000000000 / x1_min_hz);
// The gap between ALEs, at most a fetch's T4-T6 and DAD's two bus idle
// cycles, is well within 32 CLK.
constexpr auto ale_cycles = 2 * 32 * x1_max_cycles;
// A strobe ends a CLK after READY is sampled high.
constexpr auto strobe_cycles = 2 * 4 * x1_max_cycles;
// X1 clocks from ALE falling to a strobe, past tLC, half a CLK; the next
// ALE is at least two CLK on.
constexpr auto strobe_x1s = 3;

// The strobes, #RD, #WR and #INTA, all high.
bool no_strobe() {
    return (busRead(CNTL) & (CNTL_RD | CNTL_WR | CNTL_INTA)) ==
           (CNTL_RD | CNTL_WR | CNTL_INTA);
}

// Waits up to |limit| Teensy cycles for PIN to read |level|.
template <uint8_t PIN>
inline bool wait_level(uint8_t level, uint32_t limit) {
    const uint32_t start = ARM_DWT_CYCCNT;
    while (digitalReadFast(PIN) != level) {
        if (ARM_DWT_CYCCNT - start >= limit)
            return false;
    }
    return true;
}

// Waits up to |limit| Teensy cycles for a strobe to be |active| or not.
inline bool wait_strobe(bool active, uint32_t limit) {
    const uint32_t start = ARM_DWT_CYCCNT;
    while (no_strobe() == active) {
        if (ARM_DWT_CYCCNT - start >= limit)
            return false;
    }
    return true;
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

PinsI8085::PinsI8085() : _clock(x1_half_min_ns, x1_min_hz) {
    _regs = new RegsI8085(this);
    _mems = new i8080::MemsI8080();
    _devs = new DevsI8085();
#ifdef TUNE_CLOCK
    _clockInput = x1_hz;
#endif
}

#ifdef TUNE_CLOCK
bool PinsI8085::setClockInput(uint32_t hz) {
    // The rate reloads at a period start, so the running clock never
    // glitches.
    const auto rate = _clock.setRate(hz);
    if (rate == 0)
        return false;
    _clockInput = rate;
    devs<DevsI8085>()->sioClock(rate);
    return true;
}
#endif

void PinsI8085::resetPins() {
    // Assert reset condition
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
#ifdef TUNE_CLOCK
    const auto hz = _clockInput;
#else
    const auto hz = x1_hz;
#endif
    _clock.begin(hz);
    _clock.start();
    devs<DevsI8085>()->sioClock(hz);

    delayNanoseconds(reset_ns);
    negate_reset();
    Cycles::reset();
    noInterrupts();
    latchCycle();
    interrupts();
    _regs->save();
}

// Waits for the next ALE, lowers READY for its cycle and latches its
// address and status into Signals::put(). AD0-AD7 carry the address only
// until tLA after ALE falls, so the caller masks interrupts. A cycle with
// no strobe, the acknowledge of TRAP or RST 5.5-7.5, doesn't wait for
// READY: it is recorded as it passes, and the next one latched. Returns
// false when no ALE comes, as in HALT, where the status reads S1=S0=0.
bool PinsI8085::latchCycle() const {
    while (true) {
        auto s = Signals::put();
        if (!wait_level<PIN_ALE>(HIGH, ale_cycles)) {
            s->getStatus();
            return false;
        }
        negate_ready();
        wait_level<PIN_ALE>(LOW, ale_cycles);
        // assert_debug();
        s->getAddress();
        s->getStatus();
        // negate_debug();
        if (s->halt() || wait_strobe(true, strobe_x1s * x1Cycles()))
            return true;
        assert_ready();
        Cycles::next();
    }
}

// The cycle latchCycle() caught, with its strobe, as the CPU waits in it.
Signals *PinsI8085::prepareCycle() const {
    auto s = Signals::put();
    devs<DevsI8085>()->sioLoop();
    s->getDirection();
    return s;
}

// The cycle the CPU waits in, as latched.
Signals *PinsI8085::resumeCycle() const {
    auto s = Signals::put();
    s->getStatus();
    return s;
}

Signals *PinsI8085::resumeCycle(uint16_t pc) const {
    auto s = resumeCycle();
    s->addr = pc;
    return s;
}

uint32_t PinsI8085::x1Cycles() const {
#ifdef TUNE_CLOCK
    return F_CPU / _clockInput;
#else
    return F_CPU / x1_hz;
#endif
}

// Serves the cycle the CPU waits in, raises READY, and latches the next
// cycle once the strobe ends. Write data is valid by the strobe; read
// data is driven once the strobe has floated AD and released once it
// rises (tRDH 0).
Signals *PinsI8085::completeCycle(Signals *s) const {
    if (s->readEnable()) {
        if (s->memory()) {  // Memory access
            if (s->readMemory())
                s->data = _mems->read(s->addr);
        } else {  // I/O access
            const uint8_t ioaddr = s->addr;
            if (_devs->isSelected(ioaddr)) {
                s->data = _devs->read(ioaddr);
            } else {
                s->data = 0;
            }
        }
        s->outData();
    } else if (s->writeEnable()) {
        s->getData();
        if (s->memory()) {
            if (s->writeMemory())
                _mems->write(s->addr, s->data);
        } else {
            const uint8_t ioaddr = s->addr;
            if (_devs->isSelected(ioaddr))
                _devs->write(ioaddr, s->data);
        }
    } else if (s->intAck()) {
        s->data = InstI8085::vec2Inst(_devs->vector());
        s->outData();
    }
    // An interrupt from here to the next ALE could miss its address.
    noInterrupts();
    assert_ready();
    wait_strobe(false, strobe_cycles);
    Signals::inputMode();
    Cycles::next();
    latchCycle();
    interrupts();
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

Signals *PinsI8085::loop(Trap &trap) const {
    auto s = resumeCycle(_regs->nextIp());
    uint32_t polled = ARM_DWT_CYCCNT;
    while (true) {
        completeCycle(s);
        _devs->loop();
        s = prepareCycle();
        if (s->halt()) {
            const auto halt = s->prev();
            Cycles::discard(halt);
            return halt;
        }
        if (s->fetch() && ARM_DWT_CYCCNT - polled >= halt_poll_cycles) {
            polled = ARM_DWT_CYCCNT;
            if (haltSwitch()) {
                suspend(s, trap);
                return nullptr;
            }
        }
    }
}

// Cycles to take a TRAP: the instruction under way, an interrupt entry
// before it, then the TRAP's pushes.
constexpr auto trap_cycles = 64;

// Stop through TRAP: run from |s| until the TRAP is taken, which clears
// IE and pushes the PC, and stop at its fetch from 0024H with READY low.
// RIM there still reads the IE the TRAP cleared.
bool PinsI8085::suspend(Signals *s, Trap &trap) const {
    assert_trap();
    trap = {};
    uint8_t writes = 0;
    for (auto n = 0; n < trap_cycles; ++n) {
        completeCycle(s);
        s = prepareCycle();
        if (s->fetch() && s->addr == InstI8085::ORG_TRAP && writes >= 2) {
            negate_trap();
            negate_ready();
            const auto pch = s->prev(2);
            trap.pc = uint16(pch->data, s->prev(1)->data);
            trap.sp = pch->addr + 1;
            // The TRAP's machine cycles start with a fetch it ignores
            const auto m1 = pch->prev();
            Cycles::discard(m1->fetch() && m1->addr == trap.pc ? m1 : pch);
            return true;
        }
        writes = s->writeEnable() ? writes + 1 : 0;
    }
    negate_trap();
    negate_ready();
    cli.println("?halt: no TRAP");
    return false;
}

void PinsI8085::run() {
    _regs->restore();
    Cycles::reset();
    saveBreakInsts();
    startRunTimer();
    Trap trap = {};
    const auto halt = loop(trap);
    stopRunTimer();
    restoreBreakInsts();
    disassembleCycles();
    if (halt) {
        const auto pc = halt->addr;
        assert_trap();  // resume from HALT
        noInterrupts();
        latchCycle();
        interrupts();
        negate_trap();
        regs<RegsI8085>()->saveContext(pc);
    } else if (trap.sp) {
        regs<RegsI8085>()->saveTrap(trap.pc, trap.sp);
    } else {
        _regs->save();
    }
}

bool PinsI8085::rawStep(Trap &trap) const {
    auto s = resumeCycle(_regs->nextIp());
    const auto insn = _mems->read(s->addr);
    if (insn == InstI8085::HLT) {
        // Leave HLT unexecuted; run() resumes from HALT only by a TRAP.
        negate_ready();
        return false;
    }
    s->inject(insn);
    // Taken at the end of this instruction, or of the interrupt entry
    // the restore let in
    return suspend(s, trap);
}

bool PinsI8085::step(bool show) {
    Cycles::reset();
    _regs->restore();
    if (show)
        Cycles::reset();
    Trap trap;
    if (rawStep(trap)) {
        if (show)
            printCycles();
        regs<RegsI8085>()->saveTrap(trap.pc, trap.sp);
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
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

const SignalsImpl *PinsI8085::findBacktraceStart() {
    return backtraceStartByFetchCount<Signals>(_lineLimit);
}

void PinsI8085::printBacktrace() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles;) {
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
