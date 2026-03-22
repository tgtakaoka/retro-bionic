#include "pins_z280.h"
#include "debugger.h"
#include "devs_z280.h"
#include "inst_z280.h"
#include "mems_z280.h"
#include "regs_z280.h"
#include "signals_z280.h"

namespace debugger {
namespace z280 {

// clang-format off
/**
 * Z280 Z-BUS memory cycle (Z280 Technical Manual, Chapter 13).
 *
 * This board drives #XTALI and only observes #CLK, which the Z280
 * derives as #XTALI/2 (Bus Clock Select = 1/1, see resetPins()).
 * xtali_cycle_hi() toggles #XTALI once, so two calls advance one
 * #CLK cycle.
 *
 *       |_____|     |_____|     |_____|     |_____|     |
 * XTALI |     |_____|     |_____|     |_____|     |_____|
 *       |___________|           |___________|           |
 * CLK   |           |___________|           |___________|
 *
 *      |      T1       |      T2       |      T3       |
 *      |_______|       |_______|       |_______|       |
 * CLK  |       |_______|       |_______|       |_______|
 *      |_|       |______________________________________
 * AS   | |_______|
 *      |_____________________|                         |
 * DS   |                     |_________________________|
 *                                              ^ #WAIT sampled here
 *
 * Rising edge of #AS latches address, status, #R//W and B//W. #DS
 * falls in the first half of T2 for reads, the second half for
 * writes, and rises at the T3/T1 boundary once data is sampled.
 * Halt and Refresh never strobe #DS; I/O and Interrupt-Acknowledge
 * add one automatic wait cycle before #DS falls.
 */
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

// #NMI is falling-edge activated: it must be negated before the
// next single step can assert it again.
void assert_nmi() {
    digitalWriteFast(PIN_NMI, LOW);
}

void negate_nmi() {
    digitalWriteFast(PIN_NMI, HIGH);
}

inline auto signal_as() {
    return digitalReadFast(PIN_AS);
}

inline auto signal_ds() {
    return digitalReadFast(PIN_DS);
}

// A word rides the bus byte-swapped: its low (even-address) byte on
// AD8-15, its high byte on AD0-7.
inline uint16_t swapBytes(uint16_t v) {
    return static_cast<uint16_t>((v << 8) | (v >> 8));
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
    Cycles::reset();
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
    // The register has latched; release AD before the CPU drives the
    // address of its first fetch onto it.
    s->inputMode();
    _regs->reset();
    _regs->save();
}

void PinsZ280::idle() {
    xtali_cycle();
    xtali_cycle_hi();
}

Signals *PinsZ280::prepareCycle() {
    auto s = Signals::put();
    negate_wait();
    // #AS rising latches address, status, #R//W and B//W. Waiting for
    // it to fall is unbounded, so only the edge and the sampling that
    // must closely follow it run with interrupts off.
    while (signal_as() != LOW)
        xtali_cycle_hi();
    noInterrupts();
    while (signal_as() != HIGH)
        xtali_cycle_hi();
    s->getAddr();
    s->getControl();
    interrupts();
    return s;
}

Signals *PinsZ280::completeCycle(Signals *s) {
    if (s->halt() || s->refresh()) {
        // Neither strobes #DS, and #WAIT is not sampled for them.
        idle();
        idle();
        s->inputMode();
        Cycles::next();
        return s;
    }

    // I/O and Interrupt-Acknowledge insert one automatic wait cycle.
    if (!s->memReq())
        idle();

    // T2
    while (signal_ds() != LOW)
        xtali_cycle_hi();

    // An I/O port address is 16 bits; A16-A23 only carry the I/O Page
    // register, which the emulated devices take no part in.
    const uint16_t ioaddr = s->addr;

    if (s->read()) {
        if (s->memReq()) {
            if (s->readMemory()) {
                if (s->wordAccess()) {
                    s->data = mems<MemsZ280>()->read_zbus(s->addr);
                } else {
                    // Drive both halves; the CPU samples the one
                    // matching the address parity.
                    const uint16_t v = _mems->read_byte(s->addr) & 0xFF;
                    s->data = v | (v << 8);
                }
            }
        } else if (s->intAck()) {
            // No address is generated; the vector is the low byte.
            s->data = swapBytes(_devs->vector());
        } else if (s->readMemory() && _devs->isSelected(ioaddr)) {
            if (s->wordAccess()) {
                s->data = swapBytes(_devs->read(ioaddr));
            } else {
                const uint16_t v = _devs->read(ioaddr) & 0xFF;
                s->data = v | (v << 8);
            }
        }
        s->outData();
    }

    // T3. A write must be latched while #DS is still low: once the
    // rising edge is observed the AD bus may already be turning
    // around, so keep the most recent value sampled during the wait.
    while (signal_ds() != HIGH) {
        if (s->write())
            s->getData();
        xtali_cycle_hi();
    }

    if (s->write()) {
        if (s->memReq()) {
            if (s->writeMemory()) {
                if (s->wordAccess()) {
                    mems<MemsZ280>()->write_zbus(s->addr, s->data);
                } else {
                    // Driven on both halves; pick by address parity.
                    const uint8_t v = (s->addr & 1) ? lo(s->data) : hi(s->data);
                    _mems->write_byte(s->addr, v);
                }
            }
        } else if (s->writeMemory() && _devs->isSelected(ioaddr)) {
            if (s->wordAccess()) {
                _devs->write(ioaddr, swapBytes(s->data));
            } else {
                // Byte I/O always uses AD0-AD7.
                _devs->write(ioaddr, lo(s->data));
            }
        }
    }

    s->inputMode();
    Cycles::next();
    return s;
}

uint16_t PinsZ280::injectRead(uint16_t data) {
    auto s = prepareCycle();
    completeCycle(s->inject(data));
    return s->addr;
}

uint16_t PinsZ280::execute(
        const uint8_t *inst, uint_fast8_t len, uint8_t *buf, uint_fast8_t max) {
    uint16_t addr = 0;
    uint_fast8_t inj = 0;
    uint_fast8_t cap = 0;
    // Bound the damage if the CPU stops producing the transactions we
    // expect: a wrong dump beats a hung debugger.
    auto guard = Cycles::MAX_CYCLES;
    while ((inj < len || cap < max) && guard--) {
        auto s = prepareCycle();
        const auto injecting = s->memReq() && s->read() && inj < len;
        const auto capturing = s->memReq() && s->write() && cap < max;
        if (injecting) {
            // Every memory read -- fetch, operand or stack alike --
            // is answered from |inst| in program order.
            if (s->wordAccess()) {
                const uint_fast8_t next = inj + 1;
                const uint8_t b0 = inst[inj];
                const uint8_t b1 = (next < len) ? inst[next] : 0;
                s->inject(static_cast<uint16_t>(b0) << 8 | b1);
                inj = (next < len) ? next + 1 : next;
            } else {
                s->inject(static_cast<uint16_t>(inst[inj]) << 8 | inst[inj]);
                ++inj;
            }
        }
        if (capturing) {
            if (cap == 0)
                addr = s->addr;
            s->capture();
        }
        completeCycle(s);
        if (capturing && buf) {
            if (s->wordAccess()) {
                buf[cap++] = hi(s->data);
                if (cap < max)
                    buf[cap++] = lo(s->data);
            } else {
                buf[cap++] = (s->addr & 1) ? lo(s->data) : hi(s->data);
            }
        }
    }
    return addr;
}

void PinsZ280::execInst(const uint8_t *inst, uint_fast8_t len) {
    execute(inst, len, nullptr, 0);
}

uint16_t PinsZ280::captureWrites(
        const uint8_t *inst, uint_fast8_t len, uint8_t *buf, uint_fast8_t max) {
    return execute(inst, len, buf, max);
}

// Step one instruction: #NMI is accepted between instructions, so the
// CPU finishes the current one and vectors. That vector fetch is
// aborted with an injected RETN before any handler code runs. Lacking
// an #M1 equivalent, the fetch is recognized by its address plus the
// PC push that must immediately precede it.
uint16_t PinsZ280::suspend() {
    assert_nmi();
    while (true) {
        auto s = prepareCycle();
        if (s->memReq() && s->read() && s->addr == InstZ280::ORG_NMI &&
                s->prev()->memReq() && s->prev()->write()) {
            negate_nmi();
            const auto pushedPc = s->prev()->data;
            completeCycle(s->inject(InstZ280::RETN_WORD));
            // RETN pops the PC just pushed; hand back that same value.
            injectRead(pushedPc);
            Cycles::discard(s->prev());
            // Freeze in a wait state until the next prepareCycle()
            // negates #WAIT; otherwise stray idle() calls (eg. from
            // printCycles()) would run further instructions.
            assert_wait();
            return swapBytes(pushedPc);
        }
        completeCycle(s);
    }
}

bool PinsZ280::rawStep() {
    const auto pc = _regs->nextIp();
    if (_mems->read_byte(pc) == InstZ280::HALT)
        return false;
    _regs->setIp(suspend());
    return true;
}

bool PinsZ280::step(bool show) {
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

void PinsZ280::loop() {
    while (true) {
        if (!rawStep() || isBreakPoint(_regs->nextIp()) || haltSwitch()) {
            auto s = Signals::put();
            _regs->save();
            Cycles::discard(s);
            return;
        }
        _devs->loop();
    }
}

void PinsZ280::run() {
    _regs->restore();
    Cycles::reset();
    saveBreakInsts();
    loop();
    restoreBreakInsts();
    disassembleCycles();
}

void PinsZ280::setBreakInst(uint32_t addr) const {
    // Nothing to patch: loop() matches the resumed PC against the
    // breakpoint list. Patching would clobber a whole word, taking the
    // adjacent byte with it for breakpoints on odd addresses.
    (void)addr;
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
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

void PinsZ280::disassembleCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles;) {
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
