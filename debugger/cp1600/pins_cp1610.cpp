#include "pins_cp1610.h"
#include "debugger.h"
#include "devs_cp1610.h"
#include "digital_bus.h"
#include "inst_cp1610.h"
#include "mems_cp1610.h"
#include "regs_cp1610.h"
#include "signals_cp1610.h"

// #define DEBUG(e) e
#define DEBUG(e)

namespace debugger {
namespace cp1600 {

// clang-format off
/**
 * CP1610 bus cycle; a microcycle is TS1~TS4, and a bus transaction is
 * BAR followed by DTB (read) or DW and DWS (write).
 *          | TS1 | TS2 | TS3 | TS4 | TS1 | TS2 | TS3 | TS4 | TS1 |
 *           __          __          __          __          __
 *   PHI1 __|  |________|  |________|  |________|  |________|  |___
 *                 __          __          __          __
 *   PHI2 ________|  |________|  |________|  |________|  |_________
 *          ____ _______________________ _______________________ __
 *   BCx  __\___X___________BAR_________X___________DTB_________X__
 *                   _______________             _________
 *  D0-15 ----------<__address______>-----------<__data___>--------
 */
// clang-format on
// tCY: min 0.5 us, max 2 us; PHI1/PHI2 period
// tPHI: min 250 ns; PHI1/PHI2 pulse width
// tDC: max 100 ns; PHI1 to BDIR/BC1/BC2 valid
// tBO: max 100 ns; PHI1 to D0-D15 output
// tB1: min 0 ns; D0-D15 input setup to PHI1
// tB2: min 10 ns; D0-D15 input hold from PHI1
// tms: max 30 ns; PHI1 to #MSYNC

namespace {

constexpr auto phi_hi_ns = 250;
constexpr auto phi_lo_ns = 10;
constexpr auto phi1_cntl_ns = 110;

// The interrupt address injected while suspended; nothing is fetched
// from it, every read is injected.
constexpr uint16_t VEC_DEBUG = 0x0000;
// Bus transactions to wait for the CPU to stop or to acknowledge the
// debugger's interrupt; an instruction takes far fewer.
constexpr auto halt_cycles = 256;
// The halt switch polls USB through yield(), longer than a transaction's
// service; once in this many Teensy cycles is soon enough.
constexpr uint32_t halt_poll_cycles = F_CPU / 100000;

const uint8_t PINS_LOW[] = {
        PIN_PHI1,
        PIN_PHI2,
        PIN_MSYNC,
        PIN_EBCI,
};

const uint8_t PINS_HIGH[] = {
        PIN_INTRM,
        PIN_STPST,
        PIN_BDRDY,
        PIN_BUSREQ,
        PIN_ENSWI,
};

const uint8_t PINS_INPUT[] = {
        PIN_INTR,  // open drain
        PIN_PCIT,
        PIN_BDIR,
        PIN_BC1,
        PIN_BC2,
        PIN_TCI,
        PIN_HALT,
        PIN_BUSACK,
        PIN_EBCA0,
        PIN_EBCA1,
        PIN_EBCA2,
        PIN_EBCA3,
        PIN_D0,
        PIN_D1,
        PIN_D2,
        PIN_D3,
        PIN_D4,
        PIN_D5,
        PIN_D6,
        PIN_D7,
        PIN_D8,
        PIN_D9,
        PIN_D10,
        PIN_D11,
        PIN_D12,
        PIN_D13,
        PIN_D14,
        PIN_D15,
};

inline void phi1_hi() {
    digitalWriteFast(PIN_PHI1, HIGH);
}

inline void phi1_lo() {
    digitalWriteFast(PIN_PHI1, LOW);
}

inline void phi2_hi() {
    digitalWriteFast(PIN_PHI2, HIGH);
}

inline void phi2_lo() {
    digitalWriteFast(PIN_PHI2, LOW);
}

void phi1_cycle() {
    phi1_hi();
    delayNanoseconds(phi_hi_ns);
    phi1_lo();
    delayNanoseconds(phi_lo_ns);
}

void phi2_cycle() {
    phi2_hi();
    delayNanoseconds(phi_hi_ns);
    phi2_lo();
    delayNanoseconds(phi_lo_ns);
}

void microcycle() {
    phi1_cycle();  // TS1
    phi2_cycle();  // TS2
    phi1_cycle();  // TS3
    phi2_cycle();  // TS4
}

auto halt_asserted() {
    return digitalReadFast(PIN_HALT) != LOW;
}

void assert_stpst() {
    digitalWriteFast(PIN_STPST, LOW);
}

void negate_stpst() {
    digitalWriteFast(PIN_STPST, HIGH);
}

// #INTR is shared with the SIN trap logic, and has a pull-up.
void assert_intr() {
    digitalWriteFast(PIN_INTR, LOW);
    pinMode(PIN_INTR, OUTPUT);
}

void negate_intr() {
    pinMode(PIN_INTR, INPUT);
}

void intrm(bool assert) {
    digitalWriteFast(PIN_INTRM, assert ? LOW : HIGH);
}

void enswi(bool assert) {
    digitalWriteFast(PIN_ENSWI, assert ? LOW : HIGH);
}

}  // namespace

PinsCp1610::PinsCp1610() : _addr(0), _remain(0), _sdbd(false), _intrm(false) {
    _devs = new DevsCp1610();
    auto mems = new MemsCp1610(_devs);
    _mems = mems;
    _regs = new RegsCp1610(this, mems);
}

void PinsCp1610::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    _intrm = false;

    // #MSYNC low for at least 10 ms.
    for (auto i = 0; i < 10000; ++i)
        microcycle();
    phi1_cycle();
    phi2_cycle();
    // #MSYNC goes high at the rising edge of PHI1, which becomes TS3.
    phi1_hi();
    digitalWriteFast(PIN_MSYNC, HIGH);
    delayNanoseconds(phi_hi_ns);
    phi1_lo();
    delayNanoseconds(phi_lo_ns);
    phi2_cycle();

    Cycles::reset();
    _remain = 0;
    _sdbd = false;
    // The CPU inputs its starting address with IAB.
    auto s = prepareCycle(halt_cycles);
    if (s == nullptr || s->bus() != IAB) {
        cli.println("?reset: no IAB");
        return;
    }
    completeCycle(s->inject(InstCp1610::RESET_PC));
    _regs->reset();
    _regs->save();
}

/**
 * Clock the CPU until a transaction that carries data, and return it
 * with PHI1 of its TS1 high; BAR and DW are handled here. Returns
 * nullptr if the CPU halted or nothing came within |limit| microcycles.
 */
Signals *PinsCp1610::prepareCycle(uint_fast16_t limit) {
    auto s = Signals::put();
    for (uint_fast16_t n = 0; n < limit; ++n) {
        // TS1
        phi1_hi();
        delayNanoseconds(phi1_cntl_ns);
        s->getControl();
        const auto bus = s->bus();
        if (bus == ADAR || bus == IAB || bus == DTB || bus == DWS ||
                bus == INTAK) {
            s->addr = _addr;
            return s;
        }
        delayNanoseconds(phi_hi_ns - phi1_cntl_ns);
        phi1_lo();
        delayNanoseconds(phi_lo_ns);
        phi2_cycle();  // TS2
        phi1_cycle();  // TS3
        // TS4
        phi2_hi();
        if (bus == BAR) {
            s->getAddr();
            _addr = s->addr;
        }
        delayNanoseconds(phi_hi_ns);
        phi2_lo();
        delayNanoseconds(phi_lo_ns);
        if (bus == NACT && halt_asserted())
            return nullptr;
    }
    return nullptr;
}

/**
 * Complete the transaction |s| that prepareCycle() returned in its TS1.
 */
Signals *PinsCp1610::completeCycle(Signals *s) {
    const auto bus = s->bus();
    if (s->read()) {
        if (s->readMemory())
            s->data = (bus == IAB) ? _devs->vector() : _mems->read(s->addr);
        s->outData();
        delayNanoseconds(phi_hi_ns - phi1_cntl_ns);
        phi1_lo();
        delayNanoseconds(phi_lo_ns);
        phi2_cycle();  // TS2
        phi1_cycle();  // TS3; the CPU latches data
        // TS4
        phi2_hi();
        if (bus == ADAR)
            _addr = s->data;
        Cycles::next();
        delayNanoseconds(phi_hi_ns);
        phi2_lo();
        Signals::inputMode();
        delayNanoseconds(phi_lo_ns);
    } else {
        delayNanoseconds(phi_hi_ns - phi1_cntl_ns);
        phi1_lo();
        delayNanoseconds(phi_lo_ns);
        phi2_cycle();  // TS2
        phi1_cycle();  // TS3
        // TS4
        phi2_hi();
        if (bus == INTAK) {
            s->getAddr();  // R6
            s->data = _addr = s->addr;
        } else {
            s->getData();
        }
        Cycles::next();
        delayNanoseconds(phi_hi_ns);
        phi2_lo();
        delayNanoseconds(phi_lo_ns);
        if (bus == DWS && s->writeMemory())
            _mems->write(s->addr, s->data);
    }
    trackFetch(s);
    return s;
}

/**
 * The CP1610 has no fetch signal: count the transactions each fetched
 * instruction makes, and the next DTB after them is the next fetch.
 */
void PinsCp1610::trackFetch(Signals *s) {
    const auto bus = s->bus();
    if (bus == INTAK) {
        _remain = 2;  // DW/DWS of PC, then IAB
    } else if (bus == DTB && _remain == 0) {
        s->markFetch();
        _remain = InstCp1610::busCycles(s->data, _sdbd);
        _sdbd = (s->data & 0x3FF) == InstCp1610::SDBD;
    } else if (_remain) {
        --_remain;
    }
}

bool PinsCp1610::fetching(const Signals *s) const {
    return s->bus() == DTB && _remain == 0;
}

void PinsCp1610::idle() {
    // CPU is halted
    microcycle();
}

void PinsCp1610::unhalt() {
    // #STPST is edge triggered.
    assert_stpst();
    microcycle();
    negate_stpst();
    // Keep the address of a BAR that comes before HALT goes low.
    for (auto n = 0; n < halt_cycles && halt_asserted(); ++n) {
        phi1_hi();
        delayNanoseconds(phi1_cntl_ns);
        const auto bar = (busRead(CNTL) & 7) == BAR;
        delayNanoseconds(phi_hi_ns - phi1_cntl_ns);
        phi1_lo();
        delayNanoseconds(phi_lo_ns);
        phi2_cycle();  // TS2
        phi1_cycle();  // TS3
        phi2_hi();     // TS4
        if (bar)
            _addr = busRead(DATA);
        delayNanoseconds(phi_hi_ns);
        phi2_lo();
        delayNanoseconds(phi_lo_ns);
    }
}

Signals *PinsCp1610::resumeCycle() {
    auto s = Signals::put();
    *s = _held;
    s->clear();
    return s;
}

void PinsCp1610::startInsts() {
    intrm(false);
    if (halt_asserted())
        unhalt();
    // Halted, or past an IAB: at an instruction boundary.
    _remain = 0;
    _sdbd = false;
}

/**
 * The |i|-th read after injected instructions: HLT stops the CPU only after
 * the next interruptible instruction, so NOP follows it.
 */
uint16_t PinsCp1610::parking(uint_fast8_t i) {
    return i == 0 ? InstCp1610::HLT : InstCp1610::NOP;
}

uint16_t PinsCp1610::injectInsts(const uint16_t *inst, uint_fast8_t len,
        uint16_t *buf, uint_fast8_t max) {
    startInsts();
    uint16_t pc = 0;
    uint_fast8_t i = 0;
    uint_fast8_t w = 0;
    for (auto n = 0; n < halt_cycles; ++n) {
        auto s = prepareCycle();
        if (s == nullptr) {
            intrm(_intrm);
            return pc;  // halted
        }
        if (s->read()) {
            if (n == 0)
                pc = s->addr;
            s->inject(i < len ? inst[i] : parking(i - len));
            ++i;
        } else {
            s->capture();
        }
        completeCycle(s);
        if (s->bus() == DWS && w < max)
            buf[w++] = s->data;
        DEBUG(cli.print("@@  inject: "));
        DEBUG(s->print());
    }
    intrm(_intrm);
    cli.println("?inject: CPU did not halt");
    return pc;
}

bool PinsCp1610::probeIntff() {
    startInsts();
    intrm(true);
    auto intff = false;
    uint_fast8_t i = 0;
    for (auto n = 0; n < halt_cycles; ++n) {
        auto s = prepareCycle();
        if (s == nullptr)
            break;  // halted
        if (s->bus() == INTAK) {
            intff = true;
            intrm(false);
        } else if (s->bus() == IAB) {
            s->inject(VEC_DEBUG);
        } else if (s->read()) {
            // The first fetch is NOP, which is interruptible.
            s->inject(i == 0 ? InstCp1610::NOP : parking(i - 1));
            ++i;
        } else {
            s->capture();
        }
        completeCycle(s);
        DEBUG(cli.print("@@   probe: "));
        DEBUG(s->print());
    }
    intrm(_intrm);
    return intff;
}

void PinsCp1610::resumeInsts(const uint16_t *inst, uint_fast8_t len) {
    startInsts();
    uint_fast8_t i = 0;
    for (auto n = 0; n < halt_cycles; ++n) {
        auto s = prepareCycle();
        if (s == nullptr)
            break;
        if (i == len) {
            _held = *s;
            intrm(_intrm);
            return;
        }
        if (s->read()) {
            s->inject(inst[i++]);
        } else {
            s->capture();
        }
        completeCycle(s);
        DEBUG(cli.print("@@  resume: "));
        DEBUG(s->print());
    }
    intrm(_intrm);
    cli.println("?resume: CPU did not resume");
}

/**
 * The CPU acknowledged the debugger's interrupt at |intak|: capture the
 * PC it pushes, inject the interrupt address and save the registers.
 * The interrupted PC is offset by |offset|.
 */
bool PinsCp1610::suspend(Signals *intak, int_fast8_t offset) {
    negate_intr();
    completeCycle(intak);
    const auto sp = intak->addr;
    auto s = prepareCycle();
    if (s == nullptr || s->bus() != DWS) {
        cli.println("?suspend: no PC push");
        return false;
    }
    completeCycle(s->capture());
    const auto pc = s->data;
    s = prepareCycle();
    if (s == nullptr || s->bus() != IAB) {
        cli.println("?suspend: no IAB");
        return false;
    }
    completeCycle(s->inject(VEC_DEBUG));
    Cycles::discard(intak);
    Cycles::Hold hold;
    _regs->save();
    regs<RegsCp1610>()->interrupted(pc + offset, sp);
    return true;
}

/**
 * The CPU stopped after a HLT and the interruptible instruction following
 * it, and its next fetch is the PC: save the registers.
 */
void PinsCp1610::halted() {
    Cycles::Hold hold;
    _regs->save();
}

bool PinsCp1610::loop() {
    auto s = resumeCycle();
    const Signals *sin = nullptr;
    auto stopping = 0;
    uint32_t polled = ARM_DWT_CYCCNT;
    while (true) {
        if (s == nullptr) {
            if (halt_asserted()) {
                halted();
                return true;
            }
            cli.println("?run: no bus transaction");
            return false;
        }
        if (s->bus() == INTAK && (sin || stopping)) {
            // A SIN at a break point traps with PC past it.
            const auto brk = sin && isBreakPoint(sin->addr);
            const auto suspended = suspend(s, brk ? -1 : 0);
            if (brk)
                Cycles::discard(sin);
            return suspended;
        }
        completeCycle(s);
        if (s->fetch()) {
            sin = InstCp1610::sin(s->data) ? s : nullptr;
            // Backs up the #PCIT pulse the trap logic relies on.
            if (sin)
                assert_intr();
        }
        _devs->loop();
        if (stopping == 0 && ARM_DWT_CYCCNT - polled >= halt_poll_cycles) {
            polled = ARM_DWT_CYCCNT;
            if (haltSwitch()) {
                assert_intr();
                stopping = 1;
            }
        } else if (stopping && ++stopping > halt_cycles) {
            negate_intr();
            cli.println("?halt: no interrupt acknowledge");
            return false;
        }
        s = prepareCycle();
    }
}

void PinsCp1610::run() {
    _regs->restore();
    Cycles::reset();
    saveBreakInsts();
    enswi(true);
    startRunTimer();
    loop();
    stopRunTimer();
    enswi(false);
    // context has been saved in loop()
    restoreBreakInsts();
    disassembleCycles();
}

bool PinsCp1610::rawStep() {
    auto s = resumeCycle();
    assert_intr();
    auto fetches = 0;
    auto hlt = false;
    const Signals *nop = nullptr;
    for (auto n = 0; n < halt_cycles; ++n) {
        if (s == nullptr)
            break;
        if (s->bus() == INTAK) {
            const auto suspended = suspend(s, nop ? -1 : 0);
            if (nop)
                Cycles::discard(nop);
            return suspended;
        }
        if (fetching(s) && fetches++ && !nop && !_sdbd && !hlt) {
            // The stepped instruction is not interruptible: let an
            // injected NOP take the interrupt.
            s->inject(InstCp1610::NOP);
            nop = s;
        }
        completeCycle(s);
        if (fetches == 1 && s->fetch() &&
                (s->data & 0x3FF) == InstCp1610::HLT) {
            // Let it stop after the next interruptible instruction.
            negate_intr();
            hlt = true;
        }
        s = prepareCycle();
    }
    negate_intr();
    if (s == nullptr && halt_asserted()) {
        halted();
        return true;
    }
    cli.println("?step: no interrupt acknowledge");
    return false;
}

bool PinsCp1610::step(bool show) {
    Cycles::reset();
    _regs->restore();
    if (show)
        Cycles::reset();
    enswi(true);
    const auto stepped = rawStep();
    enswi(false);
    if (stepped && show)
        printCycles();
    // context has been saved by rawStep()
    return stepped;
}

void PinsCp1610::assertInt(uint8_t) {
    _intrm = true;
    intrm(true);
}

void PinsCp1610::negateInt(uint8_t) {
    _intrm = false;
    intrm(false);
}

void PinsCp1610::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, InstCp1610::SIN);
}

void PinsCp1610::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

const SignalsImpl *PinsCp1610::findBacktraceStart() {
    return backtraceStartByFetchCount<Signals>(_lineLimit);
}

void PinsCp1610::printBacktrace() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles;) {
        const auto s = g->next(i);
        if (s->fetch()) {
            const auto len = _mems->disassemble(s->addr, 1) - s->addr;
            // print bus cycles other than instruction words
            for (uint_fast8_t j = 1; j < len && i + j < cycles; j++) {
                const auto t = s->next(j);
                if (t->addr < s->addr || t->addr >= s->addr + len) {
                    t->print();
                    idle();
                }
            }
            i += len;
        } else {
            s->print();
            ++i;
        }
        idle();
    }
}

}  // namespace cp1600
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
