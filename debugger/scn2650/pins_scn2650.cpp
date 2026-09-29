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
 *        ________________________    ________________________________    _______________
 * #OPACK                         \__/                                \__/
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

constexpr auto clock_hi_ns = 380;       // 400
constexpr auto clock_lo_ns = 380;       // 400
constexpr auto clock_lo_t0 = 390;       // 400
constexpr auto clock_hi_t0p = 250;      // 400
constexpr auto clock_hi_tcor = 100;     // 400
constexpr auto clock_hi_t1 = 100;       // 400
constexpr auto clock_lo_t1w = 350;      // 400
constexpr auto clock_hi_iow = 72;       // 400
constexpr auto clock_hi_write = 314;    // 400
constexpr auto clock_hi_capture = 346;  // 400
constexpr auto clock_lo_ior = 280;      // 400
constexpr auto clock_lo_read = 250;     // 400
constexpr auto clock_lo_inject = 290;   // 400
constexpr auto clock_lo_input = 360;    // 400
constexpr auto clock_hi_exec = 20;      // 400
constexpr auto clock_hi_idle = 268;     // 400
constexpr auto clock_lo_idle = 378;     // 400

#ifdef PROFILE_CYCLES
// An instruction leaves OPREQ low for at most two processor cycles; a CPU
// in the WAIT state, after a HALT, never raises it.
constexpr auto wait_clocks = 3 * 8;
#endif

inline void clock_hi() {
    digitalWriteFast(PIN_CLOCK, HIGH);
}

inline void clock_lo() {
    digitalWriteFast(PIN_CLOCK, LOW);
}

inline auto signal_clock() {
    return digitalReadFast(PIN_CLOCK);
}

inline auto signal_opreq() {
    return digitalReadFast(PIN_OPREQ);
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
        PIN_OPACK,
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_RESET,
        PIN_CLOCK,
        PIN_PAUSE,
        PIN_INTREQ,
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

inline void clock_cycle() {
    clock_hi();
    delayNanoseconds(clock_hi_ns);
    clock_lo();
    delayNanoseconds(clock_lo_ns);
}

}  // namespace

PinsScn2650::PinsScn2650() {
    _regs = new RegsScn2650(this);
    _mems = new MemsScn2650();
    _devs = new DevsScn2650();
}

void PinsScn2650::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    // RESET must remain high for at least 3 cycles
    for (auto i = 0; i < 5; ++i)
        clock_cycle();
    negate_reset();
    Cycles::reset();

    _regs->save();
}

// Clocks to wait for OPREQ, far past an instruction's two processor
// cycles without it; a CPU in the WAIT state never raises it.
constexpr auto opreq_clocks = 1000;

// rawStep() keeps HALT from the CPU only while the cycle table is exact:
// an overcount would feed it the next instruction. record-cycles checks
// the table against the chip.
Signals *PinsScn2650::prepareCycle() {
    auto s = Signals::put();
    // T0H
    for (auto n = 0; signal_opreq() == LOW; ++n) {
        if (n >= opreq_clocks) {
            // Give up rather than wedge the board, leaving the halt switch
            // to stop the run.
            cli.println("?halt: no bus cycle");
            s->noCycle();
            return s;
        }
        delayNanoseconds(clock_hi_t0p);
        clock_lo();  // T0L
        delayNanoseconds(clock_lo_t0);
        clock_hi();  // T1H
        delayNanoseconds(clock_hi_tcor);
    }
    // T1H, OPREQ=H
    s->getAddr();
    return s;
}

Signals *PinsScn2650::completeCycle(Signals *s) {
    // T1H, OPREQ=H
    delayNanoseconds(clock_hi_t1);
    clock_lo();  // T1L
    if (s->write()) {
        s->getData();
        assert_opack();
        delayNanoseconds(clock_lo_t1w);
        clock_hi();  // T2H
        if (s->io()) {
            if (s->addr & 0x2000) {
                s->addr &= 0xFF;
                if (_devs->isSelected(s->addr)) {
                    _devs->write(s->addr, s->data);
                }
            }
            delayNanoseconds(clock_hi_iow);
        } else if (s->writeMemory()) {
            _mems->write(s->addr, s->data);
            delayNanoseconds(clock_hi_write);
        } else {
            delayNanoseconds(clock_hi_capture);
        }
    } else {
        if (s->io()) {
            if (s->vector()) {
                s->data = _devs->vector();
            } else if (s->addr & 0x2000) {
                s->addr &= 0xFF;
                if (_devs->isSelected(s->addr)) {
                    s->data = _devs->read(s->addr);
                }
            }
            s->outData();
            assert_opack();
            delayNanoseconds(clock_lo_ior);
        } else if (s->readMemory()) {
            s->data = _mems->read(s->addr);
            s->outData();
            assert_opack();
            delayNanoseconds(clock_lo_read);
        } else {
            s->outData();
            assert_opack();
            delayNanoseconds(clock_lo_inject);
        }
        clock_hi();  // T2H
        delayNanoseconds(clock_hi_ns);
    }
    negate_opack();
    clock_lo();  // T2L
    delayNanoseconds(clock_lo_input);
    Signals::inputMode();
    clock_hi();  // T0H
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
        delayNanoseconds(clock_hi_exec);
    }
    return cap;
}

void PinsScn2650::idle() {
    clock_lo();
    delayNanoseconds(clock_lo_idle);
    clock_hi();
    delayNanoseconds(clock_hi_idle);
}

#ifdef PROFILE_CYCLES
// prepareCycle(), or nullptr once OPREQ stays low: the CPU is in the WAIT
// state.
Signals *PinsScn2650::awaitCycle() {
    uint8_t clocks = 0;
    // T0H
    while (signal_opreq() == LOW) {
        if (++clocks > wait_clocks)
            return nullptr;
        delayNanoseconds(clock_hi_t0p);
        clock_lo();  // T0L
        delayNanoseconds(clock_lo_t0);
        clock_hi();  // T1H
        delayNanoseconds(clock_hi_tcor);
    }
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
        _devs->loop();
        auto s = awaitCycle();
        if (s == nullptr)
            break;
        completeCycle(s);
        if (Cycles::cycles() >= MAX_CYCLES || haltSwitch()) {
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
        // HALT or unknown instruction. Just return.
        return false;
    }

    completeCycle(s);
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
