#include "pins_kl5c8400.h"
#include "debugger.h"
#include "devs_z80.h"
#include "inst_z80.h"
#include "mems_z80.h"
#include "regs_z80.h"
#include "signals_kl5c8400.h"

namespace debugger {
namespace kl5c8400 {

using z80::InstZ80;
using z80::MemsZ80;
using z80::RegsZ80;

// clang-format off
/**
 * KL5C8400 bus cycles in KC80 mode (CNFG low), 0 wait.
 *           |  T1   |  T2   |  T3   |  T1   |  T2   |  T3   |  T4   |
 *            ___     ___     ___     ___     ___     ___     ___     ___
 *  CLK  ____|   |___|   |___|   |___|   |___|   |___|   |___|   |___|   |___
 *       ____ ______________________ _______________________________ ______
 * ADDR  ____X______________________X_______________________________X______
 *       __________               __________                   _____________
 * #MRD            \_____________/         ...#MWR, #IORD, #IOWR \______/ (I/O)
 *       ____________________________________________________________________
 *  #M1  (high in an opcode fetch; low from mid T2 in a memory read/write,
 *        from T3 in an I/O cycle)
 *
 * #MRD falls in T1 low, #MWR, #IORD and #IOWR just after T2 rises. The CPU
 * latches read data at the last falling edge, T3 in a memory cycle and T4
 * in an I/O cycle, and the strobe rises after it. #WAIT is sampled at T3
 * (memory) or T4 (I/O) rising.
 */
// clang-format on

namespace {

// The data sheet rates the clock 0-33 MHz but gives no AC timing, so each
// clock phase holds 100 ns (5 MHz) until a bench capture sets them.
constexpr auto clk_hi_ns = 100;
constexpr auto clk_lo_ns = 100;
// Strobes and #M1 settle after a clock edge.
constexpr auto strobe_ns = 60;
// RESET_ must stay low for at least three clocks.
constexpr auto reset_clocks = 5;
// Clock edges to wait for a bus cycle, far past an instruction's internal
// states.
constexpr auto cycle_edges = 2000;
// Clock edges a strobe lasts: four clocks for an I/O cycle.
constexpr auto strobe_edges = 16;
// Bus cycles execute() takes before giving up on its injection.
constexpr auto inject_cycles = 256;

bool clk_high;

void clk_hi() {
    digitalWriteFast(PIN_CLK, HIGH);
    clk_high = true;
    delayNanoseconds(clk_hi_ns);
}

void clk_lo() {
    digitalWriteFast(PIN_CLK, LOW);
    clk_high = false;
    delayNanoseconds(clk_lo_ns);
}

void clk_edge() {
    if (clk_high) {
        clk_lo();
    } else {
        clk_hi();
    }
}

void clk_cycle() {
    clk_hi();
    clk_lo();
}

inline void assert_nmi() {
    digitalWriteFast(PIN_NMI, LOW);
}

inline void negate_nmi() {
    digitalWriteFast(PIN_NMI, HIGH);
}

inline void assert_int() {
    digitalWriteFast(PIN_INT, LOW);
}

inline void negate_int() {
    digitalWriteFast(PIN_INT, HIGH);
}

void negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
}

constexpr uint8_t PINS_LOW[] = {
        PIN_CLK,
        PIN_RESET,
        PIN_CNFG,  // KC80 mode
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_BUSREQ,
        PIN_WAIT,
        PIN_INT,
        PIN_NMI,
};

constexpr uint8_t PINS_INPUT[] = {
        PIN_D0,
        PIN_D1,
        PIN_D2,
        PIN_D3,
        PIN_D4,
        PIN_D5,
        PIN_D6,
        PIN_D7,
        PIN_AL0,
        PIN_AL1,
        PIN_AL2,
        PIN_AL3,
        PIN_AL4,
        PIN_AL5,
        PIN_AL6,
        PIN_AL7,
        PIN_AM8,
        PIN_AM9,
        PIN_AM10,
        PIN_AM11,
        PIN_AH12,
        PIN_AH13,
        PIN_AH14,
        PIN_AH15,
        PIN_M1,
        PIN_MRD,
        PIN_MWR,
        PIN_IORD,
        PIN_IOWR,
        PIN_HALT,
        PIN_BUSACK,
};

}  // namespace

PinsKl5c8400::PinsKl5c8400() : PinsZ80Base() {
    _devs = new DevsZ80();
    _regs = new RegsZ80(this);
    _mems = new MemsZ80();
}

void PinsKl5c8400::resetPins() {
    // CNFG is set before RESET_ rises and never changes.
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    clk_high = false;

    for (auto i = 0; i < reset_clocks; i++)
        clk_cycle();
    negate_reset();
    Cycles::reset();
    // The first fetch, from 0000H.
    prepareWait();
    _regs->setIp(InstZ80::ORG_RESET);
    _regs->save();
}

// Clocks the CPU to its next strobe and stops there, in T2 high: #M1
// has settled by then, telling an opcode fetch from a memory read.
Signals *PinsKl5c8400::prepareCycle() const {
    const auto s = Signals::put();
    for (auto n = 0;; ++n) {
        if (n >= cycle_edges) {
            // Give up rather than wedge the board, leaving the halt switch
            // to stop the run.
            cli.println("?halt: no bus cycle");
            s->noCycle();
            return s;
        }
        clk_edge();
        delayNanoseconds(strobe_ns);
        if (s->getControl())
            break;
    }
    s->getAddress();
    if (!clk_high) {
        // #MRD came in T1 low; #M1 falls in T2 high.
        clk_hi();
        s->getControl();
    }
    return s;
}

Signals *PinsKl5c8400::completeCycle(Signals *s) const {
    if (s->nobus())
        return s;
    if (s->mrd()) {  // Memory read
        if (s->readMemory())
            s->data = _mems->read(s->addr);
        s->outData();
    } else if (s->mwr()) {  // Memory write
        s->getData();
        if (s->writeMemory())
            _mems->write(s->addr, s->data);
    } else {  // I/O cycles
        const uint8_t ioaddr = s->addr;
        if (s->iord()) {  // I/O read
            if (_devs->isSelected(ioaddr))
                s->data = _devs->read(ioaddr);
            s->outData();
        } else if (s->iowr()) {  // I/O write
            s->getData();
            if (_devs->isSelected(ioaddr))
                _devs->write(ioaddr, s->data);
        }
    }
    Cycles::next();
    // Clock on until the strobe rises, after the CPU latches read data.
    for (auto n = 0; n < strobe_edges && Signals::busy(); ++n) {
        clk_edge();
        delayNanoseconds(strobe_ns);
    }
    s->inputMode();
    return s;
}

// The core is static, so a CPU parked at its next strobe with the clock
// stopped just waits there.
Signals *PinsKl5c8400::prepareWait() const {
    return prepareCycle();
}

Signals *PinsKl5c8400::resumeCycle(uint16_t pc) const {
    const auto s = Signals::put();
    s->getControl();
    s->setAddress(pc);
    return completeCycle(s);
}

Signals *PinsKl5c8400::inject(uint8_t data) const {
    return completeCycle(prepareCycle()->inject(data));
}

uint16_t PinsKl5c8400::execute(
        const uint8_t *inst, uint_fast8_t len, uint8_t *buf, uint_fast8_t max) {
    uint16_t addr = 0;
    uint8_t inj = 0;
    uint8_t cap = 0;
    auto s = Signals::put();
    for (auto n = 0;; ++n) {
        if (n >= inject_cycles) {
            cli.println("?halt: injection stalled");
            prepareWait();
            return addr;
        }
        if (inj < len)
            s->inject(inst[inj]);
        if (cap < max)
            s->capture();
        if (inj == 0) {
            resumeCycle(_regs->nextIp());
        } else {
            completeCycle(s);
        }
        if (s->mrd()) {
            if (inj < len)
                ++inj;
        } else if (s->mwr()) {
            if (cap == 0)
                addr = s->addr;
            if (cap < max && buf)
                buf[cap++] = s->data;
        }
        if (inj >= len && cap >= max) {
            prepareWait();
            return addr;
        }
        s = prepareCycle();
    }
}

bool PinsKl5c8400::isRst38Break(const Signals *org_rst) const {
    // The RST 38H fetch, its two pushes, then this vector fetch.
    const auto rst38h = org_rst->prev(3);
    if (rst38h->data != InstZ80::RST38H)
        return false;
    const auto push_lo = org_rst->prev(1);
    const auto push_hi = org_rst->prev(2);
    if (!push_lo->mwr() || !push_hi->mwr())
        return false;
    // Mode 1, the only interrupt in KC80 mode, also enters at 0038H. It
    // discards the prefetched opcode, which may be a break point's RST
    // 38H, and pushes that opcode's address; RST 38H pushes the address
    // after it.
    const uint16_t pushed = push_lo->data | (push_hi->data << 8);
    if (pushed != static_cast<uint16_t>(rst38h->addr + 1))
        return false;
    if (isBreakPoint(rst38h->addr))
        return true;  // break point
    if (_mems->read(InstZ80::ORG_RST38H) == InstZ80::RST38H)
        return true;  // halt to system
    return false;
}

Signals *PinsKl5c8400::loop() {
    resumeCycle(_regs->nextIp());
    while (true) {
        const auto s = prepareCycle();
        if (s->addr == InstZ80::ORG_RST38H && s->fetch()) {
            if (isRst38Break(s)) {
                const auto rst38h = s->prev(3);
                // restore PC to the break point
                completeCycle(s->inject(InstZ80::RET));
                inject(lo(rst38h->addr));
                inject(hi(rst38h->addr));
                prepareWait();
                return rst38h;
            }
        }
        completeCycle(s);
        if (_devs->loop() && haltSwitch())
            return suspend();
    }
}

void PinsKl5c8400::run() {
    _regs->restore();
    Cycles::reset();
    saveBreakInsts();
    startRunTimer();
    const auto s = loop();
    stopRunTimer();
    if (s)
        Cycles::discard(s);
    restoreBreakInsts();
    disassembleCycles();
    // A failed halt never got the NMI acknowledge, so there is no saved
    // context: keep the registers from the last good save.
    if (s)
        _regs->save();
}

// Bus cycles to wait for the NMI acknowledge; an instruction takes far fewer.
constexpr auto nmi_ack_cycles = 1024;

Signals *PinsKl5c8400::suspend() {
    assert_nmi();
    // Bound the wait; loop() polls the halt switch only between steps.
    for (auto n = 0; n < nmi_ack_cycles; ++n) {
        const auto s = prepareCycle();
        if (s->fetch() && s->addr == InstZ80::ORG_NMI) {
            negate_nmi();
            // RET, not RETN, keeps IFF1 clear through the save
            completeCycle(s->inject(InstZ80::RET));
            // Feed RET the PC the NMI pushed, low byte first.
            inject(s->prev()->data);
            inject(s->prev(2)->data);
            // Park at the fetch RET returns to, where what follows injects.
            prepareWait();
            // The NMI discards the prefetched opcode and pushes its address.
            return s->prev(3);
        }
        completeCycle(s);
    }
    negate_nmi();
    prepareWait();
    cli.println("?halt: no NMI acknowledge");
    return nullptr;
}

bool PinsKl5c8400::rawStep() {
    const auto pc = _regs->nextIp();
    if (_mems->read_byte(pc) == InstZ80::HALT)
        return false;
    // The parked fetch is this instruction's prefetch; the NMI is taken at
    // the end of the instruction it starts.
    resumeCycle(pc);
    const auto s = suspend();
    if (s == nullptr)
        return false;
    Cycles::discard(s);
    return true;
}

bool PinsKl5c8400::step(bool show) {
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

void PinsKl5c8400::assertInt(uint8_t) {
    assert_int();
}

void PinsKl5c8400::negateInt(uint8_t) {
    negate_int();
}

void PinsKl5c8400::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

const SignalsImpl *PinsKl5c8400::findBacktraceStart() {
    return backtraceStartByFetchCount<Signals>(_lineLimit);
}

void PinsKl5c8400::printBacktrace() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles;) {
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

}  // namespace kl5c8400
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
