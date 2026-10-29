#include "pins_nsc800.h"
#include "debugger.h"
#include "devs_z80.h"
#include "inst_z80.h"
#include "mems_z80.h"
#include "regs_z80.h"
#include "signals_nsc800.h"

namespace debugger {
namespace nsc800 {

using z80::InstZ80;
using z80::MemsZ80;
using z80::waitUntil;

// clang-format off
/**
 * NSC800 Opcode fetch
 *           |----T1-----|----T2-----|----T3-----|----T4-----|----T1-----|
 *           |-T1A-|-T1B-|-T2A-|-T2B-|-T3A-|-T3B-|-T4A-|-T4B-|-T21-|-T1B-|
 *         __    __    __    __    __    __    __    __    __    __    __
 *    X1 _|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |
 *           \ ____\     \ ____\     \ ____\     \ ____\     \ ____\     \
 *   CLK _____|    T|1____|    T|2____|    T|3____|    T|4____|    T|1____
 *            \ ____\     \     |     \ ____\     \           \ ____\
 *   ALE ______|     |_____v____v______|     |_____v___________|     |____
 * AH/Sx ______\ __________|___________\ __________|___________\ _________
 * #IO/M _______X_________AH____________X________regI___________X_________
 *       _______|________  |          __|__________|            |________
 *    AD _______X___AL___>-----------<inX___regR___>------------<___AL___>
 * #INTA __________________|            ________________________|_________
 *   #RD                   |___________|
 *       ______________________________\                        |_________
 * #RFSH                                |_______________________|
 *       _______________________v_________________________________________
 * #WAIT ______________________/ \________________________________________
 *
 * NSC800 Memory read write
 *           |----T1-----|----T2-----|----T3-----|----T1-----|
 *           |-T1A-|-T1B-|-T2A-|-T2B-|-T3A-|-T3B-|-T21-|-T1B-|
 *         __    __    __    __    __    __    __    __    __
 *    X1 _|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |
 *           \ ____\     \ ____\     \ ____\     \ ____\     \
 *   CLK _____|    T|1____|    T|2____|    T|3____|    T|1____
 *            \ ____\     \     \           \     \ ____\
 *   ALE ______|     |_____v____vv___________v____|      |____
 * AH/Sx _______ __________|_____|___________|____\ __________
 * #IO/M _______X__________________________________X__________
 *       _______|________  |     |          _|     |__________
 *    AD _______X___AL___>-------_---------<in>---<___________
 *       __________________|     |           |_____|__________
 *   #RD                   |_________________|
 *       _______ ________ _______|___________|_____|__________
 *    AD _______X___AL___X__________out____________X__________
 *       ________________________|           |________________
 *   #WR                         |___________|
 *       _______________________v_____________________________
 * #WAIT ______________________/ \____________________________
 */
// clang-format on

namespace {
//   tx: min 125 ns, max 3,333 ns; XIN period (~8MHz, NSC800-4)
// tCYC: min 320 ns, max 2,000 ns; CLK cycle period
// tXCF: min   5 ns, max    80 ns; XIN falling to CLK falling
// tXCR: min   5 ns, max    80 ns; XIN falling to CLK rising
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

// XIN runs free from the PWM, and CLK is XIN/2. #WAIT, asserted except to
// end a cycle, holds the CPU from T2 of each cycle with #RD or #INTA; a
// write is held before #WR, and the cycle after an accepted interrupt
// holds with no strobe at all. ALE latches nothing on the board, so the
// Teensy reads A0-A7 off AD0-AD7 as ALE falls.

constexpr uint32_t xin_hz = 8333333;  // 150 MHz / 18; CLK 4.17 MHz
// T at most 6,667 ns.
constexpr uint32_t xin_min_hz = 300000;
#ifdef TUNE_CLOCK
// K may overclock the CPU.
constexpr auto xin_half_min_ns = 50;
#else
// tx min 125 ns; an odd count leaves the high half at 60 ns.
constexpr auto xin_half_min_ns = 60;
#endif
// Teensy cycles in a CLK clock at the slowest rate, so the waits below
// hold at any rate.
constexpr uint32_t clk_max_cycles = 2 * (F_CPU / xin_min_hz);
// #RESET_IN must be low for at least three clocks.
constexpr auto reset_ns = 5 * 2 * (1000000000 / xin_min_hz);
// Waits for a bus cycle last 1000 clocks, far past an instruction's
// internal states.
constexpr auto cycle_cycles = 1000 * clk_max_cycles;
// A strobe follows ALE, and ends, within a few clocks.
constexpr auto strobe_cycles = 4 * clk_max_cycles;
// ALE pulses prepareCycle() skips before giving up: a refresh, or the
// strobeless cycle after an accepted interrupt.
constexpr auto skip_cycles = 16;
// Bus cycles execute() takes before giving up on its injection.
constexpr auto inject_cycles = 256;
// Bus cycles between polls of the halt switch in a run: its yield()
// services the USB while #WAIT holds the CPU, which would otherwise
// stretch every cycle. The devices ration their own polling.
constexpr auto halt_poll_cycles = 16;

inline auto signal_clk() {
    return digitalReadFast(PIN_CLK);
}

inline void assert_nmi() {
    digitalWriteFast(PIN_NMI, LOW);
}

inline void negate_nmi() {
    digitalWriteFast(PIN_NMI, HIGH);
}

inline void assert_intr() {
    digitalWriteFast(PIN_INTR, LOW);
}

inline void negate_intr() {
    digitalWriteFast(PIN_INTR, HIGH);
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

inline void assert_wait() {
    digitalWriteFast(PIN_WAIT, LOW);
}

inline void negate_wait() {
    digitalWriteFast(PIN_WAIT, HIGH);
}

inline void negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
}

const uint8_t PINS_LOW[] = {
        PIN_RESET,
};

const uint8_t PINS_HIGH[] = {
        PIN_RSTC,
        PIN_RSTB,
        PIN_RSTA,
        PIN_INTR,
        PIN_NMI,
        PIN_PS,
        PIN_BREQ,
        PIN_WAIT,
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
        PIN_BACK,
        PIN_RFSH,
        PIN_INTA,
        PIN_RD,
        PIN_WR,
        PIN_RESOUT,
};

}  // namespace

PinsNsc800::PinsNsc800()
    : PinsZ80Base(), _clock(xin_half_min_ns, xin_min_hz) {
    _devs = new DevsZ80();
    _regs = new z80::RegsZ80(this);
    _mems = new MemsZ80();
#ifdef TUNE_CLOCK
    _clockInput = xin_hz;
#endif
}

void PinsNsc800::resetPins() {
    // #RESET_IN is asserted first: a glitch on XIN is harmless.
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
#ifdef TUNE_CLOCK
    _clock.begin(_clockInput);
#else
    _clock.begin(xin_hz);
#endif
    _clock.start();
    delayNanoseconds(reset_ns);
    negate_reset();
    // #WAIT holds the first fetch.
    assert_wait();
    Cycles::reset();
    prepareWait();
    _regs->setIp(InstZ80::ORG_RESET);
    _regs->save();
}

// Returns the cycle ALE starts, held by #WAIT on its #RD or #INTA, or
// before #WR. completeCycle() leaves interrupts masked until this has the
// next address: an interrupt could outlast ALE, and the address with it.
Signals *PinsNsc800::prepareCycle() const {
    const auto s = Signals::put();
    for (auto skip = 0;; ++skip) {
        if (skip >= skip_cycles) {
            interrupts();
            cli.println("?halt: no data cycle");
            s->noCycle();
            return s;
        }
        if (!waitUntil([] { return signal_ale() == HIGH; }, cycle_cycles) ||
                !waitUntil([] { return signal_ale() == LOW; }, strobe_cycles)) {
            interrupts();
            // Give up rather than wedge the board, leaving the halt switch
            // to stop the run.
            cli.println("?halt: no bus cycle");
            s->noCycle();
            return s;
        }
        // tLA: A0-A7 stay 100 ns past ALE falling.
        s->getAddress();
        if (s->writing())
            break;
        // A refresh ALE is soon followed by the next cycle's.
        waitUntil(
                [s] {
                    s->getControl();
                    return !s->nobus() || signal_ale() == HIGH;
                },
                strobe_cycles);
        if (!s->nobus())
            break;
        if (signal_ale() == LOW) {
            // The strobeless cycle after an accepted interrupt: let it end.
            negate_wait();
            waitUntil([] { return signal_ale() == HIGH; }, strobe_cycles);
            assert_wait();
        }
    }
    interrupts();
    return s;
}

Signals *PinsNsc800::completeCycle(Signals *s) const {
    if (s->writing()) {
        // Held before #WR, with the data out on AD0-AD7 from the CLK rise
        // after ALE. Write it meanwhile: the next ALE follows #WR by
        // less than a clock, too soon for a device write.
        waitUntil([] { return signal_clk() == HIGH; }, clk_max_cycles);
        waitUntil([] { return signal_clk() == LOW; }, clk_max_cycles);
        s->getData();
        if (s->mreq()) {
            if (s->writeMemory())
                _mems->write_byte(s->addr, s->data);
        } else {
            const uint8_t ioaddr = s->addr;
            if (_devs->isSelected(ioaddr))
                _devs->write(ioaddr, s->data);
        }
        // Left masked for the next prepareCycle(), which every caller
        // runs next; it lasts through the wait for ALE, up to
        // cycle_cycles if the CPU makes no cycle.
        noInterrupts();
        negate_wait();
        // The control lines with #WR low mark the cycle a write.
        waitUntil(
                [s] {
                    s->getControl();
                    return s->write();
                },
                strobe_cycles);
        // Done while #WR lasts, as in a read below.
        Cycles::next();
        waitUntil([] { return signal_wr() == HIGH; }, strobe_cycles);
        assert_wait();
        return s;
    }
    if (s->nobus())
        return s;
    if (s->read()) {
        if (s->mreq()) {  // Memory access
            if (s->readMemory())
                s->data = _mems->read_byte(s->addr);
        } else {  // I/O access
            const uint8_t ioaddr = s->addr;
            if (_devs->isSelected(ioaddr))
                s->data = _devs->read(ioaddr);
        }
    } else {  // Interrupt acknowledge
        s->data = _devs->vector();
    }
    s->outData();
    // Left masked for the next prepareCycle(), which every caller runs
    // next; it lasts through the wait for ALE, up to cycle_cycles if the CPU
    // makes no cycle.
    noInterrupts();
    // assert_debug();
    negate_wait();
    // Kept off the release, and done while the strobe lasts: the next ALE
    // may follow its end by only tCL.
    Cycles::next();
    waitUntil([] { return !Signals::busy(); }, strobe_cycles);
    assert_wait();
    // negate_debug();
    return s;
}

// The CPU comes to its next cycle by itself and waits there.
Signals *PinsNsc800::prepareWait() const {
    return prepareCycle();
}

Signals *PinsNsc800::resumeCycle(uint16_t pc) const {
    const auto s = Signals::put();
    // The CPU comes to the fetch of |pc| by itself, past its ALE, and
    // waits there with #RD low; serve it by that status, not a stale one.
    waitUntil(
            [s] {
                s->getAddress();
                return !s->nobus();
            },
            cycle_cycles);
    s->setAddress(pc);
    return completeCycle(s);
}

Signals *PinsNsc800::inject(uint8_t data) const {
    return completeCycle(prepareCycle()->inject(data));
}

uint16_t PinsNsc800::execute(
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
        if (s->mreq()) {
            if (s->read()) {
                if (inj < len)
                    ++inj;
            } else {
                if (cap == 0)
                    addr = s->addr;
                if (cap < max && buf)
                    buf[cap++] = s->data;
            }
        }
        if (inj >= len && cap >= max) {
            prepareWait();
            return addr;
        }
        s = prepareCycle();
    }
}

bool PinsNsc800::isRst38Break(const Signals *org_rst) const {
    // The RST 38H fetch, its two pushes, then this vector fetch.
    const auto rst38h = org_rst->prev(3);
    if (rst38h->data == InstZ80::RST38H) {
        if (org_rst->prev(1)->mwrite() && org_rst->prev(2)->mwrite()) {
            if (isBreakPoint(rst38h->addr))
                return true;  // break point
            if (_mems->read(InstZ80::ORG_RST38H) == InstZ80::RST38H)
                return true;  // halt to system
        }
    }
    return false;
}

bool PinsNsc800::loop() {
    resumeCycle(_regs->nextIp());
    auto poll = 0;
    while (true) {
        const auto s = prepareCycle();
        // #WAIT holds the CPU meanwhile, with the address taken.
        _devs->loop();
        if (++poll >= halt_poll_cycles) {
            poll = 0;
            if (haltSwitch()) {
                // Serve the cycle held, then stop at an instruction.
                completeCycle(s);
                return suspend();
            }
        }
        if (s->addr == InstZ80::ORG_RST38H && s->fetch()) {
            if (isRst38Break(s)) {
                const auto rst38h = s->prev(3);
                // restore PC to the break point
                completeCycle(s->inject(InstZ80::RET));
                inject(lo(rst38h->addr));
                inject(hi(rst38h->addr));
                Cycles::discard(rst38h);
                prepareWait();
                return true;
            }
        }
        completeCycle(s);
    }
}

void PinsNsc800::run() {
    _regs->restore();
    Cycles::reset();
    saveBreakInsts();
    startRunTimer();
    const auto stopped = loop();
    stopRunTimer();
    restoreBreakInsts();
    disassembleCycles();
    // A failed halt never got the NMI acknowledge, so there is no saved
    // context: keep the registers from the last good save.
    if (stopped)
        _regs->save();
}

// Bus cycles to wait for the NMI acknowledge; an instruction takes far fewer.
constexpr auto nmi_ack_cycles = 1024;

bool PinsNsc800::suspend() {
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
            // Unlike the Z80's, the NMI's dummy opcode fetch has no RD, so
            // prepareCycle() never records it: only the two pushes go.
            Cycles::discard(s->prev(2));
            prepareWait();
            return true;
        }
        completeCycle(s);
    }
    negate_nmi();
    prepareWait();
    cli.println("?halt: no NMI acknowledge");
    return false;
}

bool PinsNsc800::rawStep() {
    const auto pc = _regs->nextIp();
    if (_mems->read_byte(pc) == InstZ80::HALT)
        return false;
    assert_nmi();
    resumeCycle(pc);
    return suspend();
}

bool PinsNsc800::step(bool show) {
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

// RSTA/B/C and INTR are all active low.
void PinsNsc800::assertInt(uint8_t name) {
    switch (name) {
    default:
        assert_intr();
        break;
    case INTR_RSTC:
        digitalWriteFast(PIN_RSTC, LOW);
        break;
    case INTR_RSTB:
        digitalWriteFast(PIN_RSTB, LOW);
        break;
    case INTR_RSTA:
        digitalWriteFast(PIN_RSTA, LOW);
        break;
    case INTR_NONE:
        break;
    }
}

void PinsNsc800::negateInt(uint8_t name) {
    switch (name) {
    default:
        negate_intr();
        break;
    case INTR_RSTC:
        digitalWriteFast(PIN_RSTC, HIGH);
        break;
    case INTR_RSTB:
        digitalWriteFast(PIN_RSTB, HIGH);
        break;
    case INTR_RSTA:
        digitalWriteFast(PIN_RSTA, HIGH);
        break;
    case INTR_NONE:
        break;
    }
}

void PinsNsc800::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

const SignalsImpl *PinsNsc800::findBacktraceStart() {
    return backtraceStartByFetchCount<Signals>(_lineLimit);
}

void PinsNsc800::printBacktrace() {
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

}  // namespace nsc800
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
