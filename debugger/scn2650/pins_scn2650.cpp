#include "pins_scn2650.h"
#include "debugger.h"
#include "devs_scn2650.h"
#include "inst_scn2650.h"
#include "mems_scn2650.h"
#include "regs_scn2650.h"

namespace debugger {
namespace scn2650 {

// clang-format off
/**
 * SCN2650 Bus cycle
 *            _____       _____       _____       _____       _____       _____       __
 *  CLOCK ___|    T|0____|    T|1____|    T|2____|    T|0____|    T|1____|    T|2____|
 *                        \__________________\                 \_________________\
 *  OPREQ ________________|                  |_________________|                 |_______
 *        _______________ /__________________\__ _____________ /__________________\_ ____
 * A0~A14 _______________X______________________X_____________X_____________________X____
 *  M/#IO _______________                     |  _________________________________|_
 *   #R/W                \____________________|_/                                 | \____
 *        ______________________________ _____ ___________________________________ ______
 *   DATA ______________________________X__in_X________________________out________X______
 *        _______________________              _______________________             ______
 * #OPACK                        \____________/                       \___________/
 *        ____________________________________________________       _______       ______
 *   #WRP                                                     \_____/       \_____/
 */
// clang-format on

namespace {
//  tCP: min 800 ns; CLOCK period
//  tCH: min 400 ns; CLOCK high width
//  tCL: min 400 ns; CLOCK low width
// tCOR: max 300 ns; CLOCK+ to OPREQ+
//  tOR: max tCP+tCL+75 ns; OPREQ width
//  tAS: min  50 ns; Address stable to OPREQ+
// tCCS: min  50 ns; Control stable to OPREQ+
// tOAD: max tCP-350 ns; #OPACK delay from OPREQ+
// tOAH: min tCP ns; #OPACK hold time from OPREQ+
// tDIA: min tcp+tCL-300 ns; Data in from OPREQ+
// tWPD: max tCH+100 ns; WRP delay from OPREQ+
// tWPW: min tCL-50 ns; WRP width

// CLOCK runs free from the PWM, and the CPU waits in each operation, with
// OPREQ high, until #OPACK is asserted: #OPACK is sampled on each CLOCK
// fall from T2 on. #OPACK is negated once OPREQ falls.

constexpr uint32_t clock_hz = 1250000;  // 150 MHz / 120: halves of 400 ns
// The 2650 is static: any rate K takes down to this one.
constexpr uint32_t clock_min_hz = 100000;
#ifdef TUNE_CLOCK
// K may overclock the CPU, down to halves of 200 ns.
constexpr auto clock_half_min_ns = 200;
#else
constexpr auto clock_half_min_ns = 400;
#endif
// Teensy cycles in a CLOCK clock at the slowest rate, so the waits below
// hold at any rate.
constexpr uint32_t clock_max_cycles = F_CPU / clock_min_hz;
// RESET must remain high for at least 3 clocks.
constexpr auto reset_ns = 5 * (1000000000 / clock_min_hz);
// Waits for OPREQ last 1000 clocks, far past an instruction's two
// processor cycles without it; a CPU in the WAIT state never raises it.
constexpr auto opreq_cycles = 1000 * clock_max_cycles;
// OPREQ falls on the clock that samples #OPACK.
constexpr auto opack_cycles = 4 * clock_max_cycles;
// Data and #OPACK are held after OPREQ falls.
constexpr auto opack_hold_ns = 50;

#ifdef PROFILE_CYCLES
// An instruction leaves OPREQ low for at most two processor cycles; a CPU
// in the WAIT state, after a HALT, never raises it.
constexpr auto wait_clocks = 3 * 8;
#endif

inline auto signal_opreq() {
    return digitalReadFast(PIN_OPREQ);
}

// Waits up to |limit| Teensy cycles for OPREQ to read |level|.
bool wait_opreq(uint8_t level, uint32_t limit) {
    const uint32_t start = ARM_DWT_CYCCNT;
    while (signal_opreq() != level) {
        if (ARM_DWT_CYCCNT - start >= limit)
            return false;
    }
    return true;
}

void assert_intreq() {
    digitalWriteFast(PIN_INTREQ, LOW);
}

void negate_intreq() {
    digitalWriteFast(PIN_INTREQ, HIGH);
}

void assert_opack() {
    digitalWriteFast(PIN_OPACK, LOW);
}

void negate_opack() {
    digitalWriteFast(PIN_OPACK, HIGH);
}

void negate_reset() {
    digitalWriteFast(PIN_RESET, LOW);
}

constexpr uint8_t PINS_LOW[] = {
        PIN_ADREN,
        PIN_DBUSEN,
        PIN_SENSE,
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_RESET,
        PIN_PAUSE,
        PIN_INTREQ,
        PIN_OPACK,
};

constexpr uint8_t PINS_INPUT[] = {
        PIN_DBUS0,
        PIN_DBUS1,
        PIN_DBUS2,
        PIN_DBUS3,
        PIN_DBUS4,
        PIN_DBUS5,
        PIN_DBUS6,
        PIN_DBUS7,
        PIN_ADR0,
        PIN_ADR1,
        PIN_ADR2,
        PIN_ADR3,
        PIN_ADR4,
        PIN_ADR5,
        PIN_ADR6,
        PIN_ADR7,
        PIN_ADR8,
        PIN_ADR9,
        PIN_ADR10,
        PIN_ADR11,
        PIN_ADR12,
        PIN_ADR13,
        PIN_ADR14,
        PIN_OPREQ,
        PIN_MIO,
        PIN_RW,
        PIN_WRP,
        PIN_INTACK,
        PIN_RUNWAIT,
        PIN_FLAG,
};

}  // namespace

PinsScn2650::PinsScn2650() : _clock(clock_half_min_ns, clock_min_hz) {
    _regs = new RegsScn2650(this);
    _mems = new MemsScn2650();
    _devs = new DevsScn2650();
#ifdef TUNE_CLOCK
    _clockInput = clock_hz;
#endif
#ifdef PROFILE_CYCLES
    _opreqLow = 0;
#endif
}

#ifdef TUNE_CLOCK
bool PinsScn2650::setClockInput(uint32_t hz) {
    // The rate reloads at a period start, so the running clock never
    // glitches.
    const auto rate = _clock.setRate(hz);
    if (rate == 0)
        return false;
    _clockInput = rate;
    return true;
}
#endif

void PinsScn2650::resetPins() {
    // RESET is asserted first: a glitch on CLOCK is harmless.
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
#ifdef TUNE_CLOCK
    _clock.begin(_clockInput);
#else
    _clock.begin(clock_hz);
#endif
    _clock.start();
    delayNanoseconds(reset_ns);
    negate_reset();
    Cycles::reset();

    _regs->save();
}

// rawStep() keeps HALT from the CPU only while the cycle table is exact:
// an overcount would feed it the next instruction. record-cycles checks
// the table against the chip.
Signals *PinsScn2650::prepareCycle() {
    auto s = Signals::put();
    if (!wait_opreq(HIGH, opreq_cycles)) {
        // Give up rather than wedge the board, leaving the halt switch
        // to stop the run.
        cli.println("?halt: no bus cycle");
        s->noCycle();
        return s;
    }
    s->getAddr();
    return s;
}

// The CPU is held in the cycle prepared until #OPACK is asserted here.
// Address, control and write data are valid while OPREQ is high.
Signals *PinsScn2650::completeCycle(Signals *s) {
    const auto read = s->read();
    // Extended I/O (A13 high) carries its port on A0-A7.
    const auto extended =
            s->io() && !s->vector() && (s->addr & 0x2000) != 0;
    if (extended)
        s->addr &= 0xFF;
    if (read) {
        if (s->io()) {
            if (s->vector()) {
                s->data = _devs->vector();
            } else if (extended && _devs->isSelected(s->addr)) {
                s->data = _devs->read(s->addr);
            }
        } else if (s->readMemory()) {
            s->data = _mems->read(s->addr);
        }
        s->outData();
    } else {
        s->getData();
    }
    // An interrupt here would leave #OPACK asserted into the next cycle,
    // which then ends unserved.
    noInterrupts();
    assert_opack();
    wait_opreq(LOW, opack_cycles);
#ifdef PROFILE_CYCLES
    _opreqLow = ARM_DWT_CYCCNT;
#endif
    delayNanoseconds(opack_hold_ns);
    negate_opack();
    interrupts();
    if (read) {
        Signals::inputMode();
    } else if (s->io()) {
        if (extended && _devs->isSelected(s->addr))
            _devs->write(s->addr, s->data);
    } else if (s->writeMemory()) {
        _mems->write(s->addr, s->data);
    }
    Cycles::next();
    return s;
}

void PinsScn2650::execInst(const uint8_t *inst, uint8_t len) {
    execute(inst, len, nullptr, nullptr, 0);
}

uint8_t PinsScn2650::captureWrites(const uint8_t *inst, uint8_t len,
        uint16_t *addr, uint8_t *buf, uint8_t max) {
    return execute(inst, len, addr, buf, max);
}

uint8_t PinsScn2650::execute(const uint8_t *inst, uint8_t len, uint16_t *addr,
        uint8_t *buf, uint8_t max) {
    uint8_t inj = 0;
    uint8_t cap = 0;
    while (inj < len || cap < max) {
        auto s = prepareCycle();
        if (cap < max)
            s->capture();
        if (inj < len)
            s->inject(inst[inj]);
        completeCycle(s);
        if (inj == 0 && addr)
            *addr = s->addr;
        if (s->read()) {
            if (inj < len)
                inj++;
        } else {
            if (buf && cap < max)
                buf[cap++] = s->data;
        }
    }
    return cap;
}

#ifdef PROFILE_CYCLES
uint32_t PinsScn2650::clockCycles() const {
#ifdef TUNE_CLOCK
    return F_CPU / _clockInput;
#else
    return F_CPU / clock_hz;
#endif
}

// prepareCycle(), or nullptr once OPREQ stays low: the CPU is in the WAIT
// state. OPREQ is low for half a clock more than the clocks counted.
Signals *PinsScn2650::awaitCycle() {
    const auto cycles = clockCycles();
    if (!wait_opreq(HIGH, wait_clocks * cycles))
        return nullptr;
    const uint8_t clocks = (ARM_DWT_CYCCNT - _opreqLow) / cycles;
    auto s = prepareCycle();
    s->clocks() = clocks;
    return s;
}

// For scripts/record-cycles.py: every bus cycle as the chip makes it, no
// cycle table, until the CPU executes a HALT and enters the WAIT state, so
// only a real opcode fetch ends a pattern; give up well before the ring
// wraps. The debug pin frames the run, to trigger a capture on.
void PinsScn2650::loop() {
    constexpr auto MAX_CYCLES = 96;
    assert_debug();
    while (true) {
        auto s = awaitCycle();
        if (s == nullptr)
            break;
        // The CPU waits on #OPACK meanwhile, so the clocks awaitCycle()
        // counts until the next cycle stay exact.
        _devs->loop();
        const auto halt = haltSwitch();
        completeCycle(s);
        if (Cycles::cycles() >= MAX_CYCLES || halt) {
            cli.println(Cycles::cycles() >= MAX_CYCLES ? "?cycles" : "?halt");
            break;
        }
    }
    negate_debug();
}
#else
void PinsScn2650::loop() {
    while (true) {
        _devs->loop();
        if (!rawStep() || haltSwitch())
            return;
    }
}
#endif

void PinsScn2650::run() {
    _regs->restore();
    Cycles::reset();
    saveBreakInsts();
    startRunTimer();
    loop();
    stopRunTimer();
    restoreBreakInsts();
    disassembleCycles();
#ifdef PROFILE_CYCLES
    // Only a reset brings the CPU out of the WAIT state, or back from
    // wherever the run gave up.
    resetPins();
#else
    _regs->save();
#endif
}

bool PinsScn2650::rawStep() {
    auto s = prepareCycle();
    const auto inst = _mems->read(s->addr);
    const auto len = InstScn2650::instLen(inst);
    if (inst == InstScn2650::HALT || len == 0) {
        // A HALT or unknown instruction is left unexecuted.
        return false;
    }

    // The fetch is served the opcode just read.
    completeCycle(s->inject(inst));
    const auto opr = _mems->read(s->addr + 1);
    const auto busCycles = len + InstScn2650::busCycles(inst, opr);
    const auto indirect = busCycles > len + InstScn2650::busCycles(inst, 0);
    const auto branch = indirect && InstScn2650::isBranch(inst);
    const auto fetch = s->addr;
    s->markFetch();
    for (auto i = 1; i < busCycles; ++i) {
        auto s = prepareCycle();
        if (branch && i == len && InstScn2650::notTaken(fetch, len, s->addr))
            return true;  // the next opcode's fetch, left for the next step
        if (s->vector()) {
            s->clearFetch();
            completeCycle(s);
            if (InstScn2650::busCycles(InstScn2650::ZBSR, s->data)) {
                // Indirect vector
                completeCycle(prepareCycle());
                completeCycle(prepareCycle());
            }
            break;
        }
        completeCycle(s);
    }
    return true;
}

bool PinsScn2650::step(bool show) {
    Cycles::reset();
    _regs->restore();
    if (show)
        Cycles::reset();
    if (rawStep()) {
        if (show)
            printCycles();
        _regs->save();
        return true;
    }
    return false;
}

void PinsScn2650::assertInt(uint8_t) {
    assert_intreq();
}

void PinsScn2650::negateInt(uint8_t) {
    negate_intreq();
}

void PinsScn2650::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, InstScn2650::HALT);
}

void PinsScn2650::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; i++) {
        g->next(i)->print();
    }
}

const SignalsImpl *PinsScn2650::findBacktraceStart() {
    return backtraceStartByFetchCount<Signals>(_lineLimit);
}

void PinsScn2650::printBacktrace() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
#ifdef PROFILE_CYCLES
    // Every cycle, with the marks rawStep() would give them, up to the
    // HALT or unknown instruction it stops at; a HALT is marked too.
    for (auto i = 0u; i < cycles;) {
        const auto s = g->next(i);
        const auto inst = _mems->read(s->addr);
        const auto len = InstScn2650::instLen(inst);
        if (len == 0)
            break;
        const auto opr = _mems->read(s->addr + 1);
        auto busCycles = len + InstScn2650::busCycles(inst, opr);
        if (busCycles > len + InstScn2650::busCycles(inst, 0) &&
                InstScn2650::isBranch(inst) &&
                InstScn2650::notTaken(s->addr, len, g->next(i + len)->addr))
            busCycles -= 2;
        s->markFetch(busCycles);
        if (inst == InstScn2650::HALT)
            break;
        i += busCycles;
    }
    printCycles();
#else
    for (auto i = 0u; i < cycles;) {
        const auto s = g->next(i);
        if (s->fetch()) {
            const auto len = _mems->disassemble(s->addr, 1) - s->addr;
            i += len;
        } else {
            s->print();
            ++i;
        }
    }
#endif
}

}  // namespace scn2650
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
