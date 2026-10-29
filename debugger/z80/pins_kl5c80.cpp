#include "pins_kl5c80.h"
#include "debugger.h"
#include "devs_z80.h"
#include "inst_z80.h"
#include "mems_z80.h"
#include "regs_z80.h"
#include "signals_kl5c80.h"

namespace debugger {
namespace kl5c80 {

using z80::InstZ80;
using z80::MemsZ80;
using z80::waitUntil;
using z80::RegsZ80;

// clang-format off
/**
 * KL5C80 bus cycle.  0 wait / 1 wait
 *       __    __    __    __    __    __    __    __    __    __    __    __    __
 *  XIN    |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__
 *       __\     \ ____\     \ ____\     \ ____\     \ ____\      \____\     \ ____\
 *  CLK     |_____|     |_____|     |_____|     |_____|     |_____|     |_____|     |__
 *       ___\ __________\___________\ ________________v_____\ ________________v_____\ _
 * ADDR  ____X___________X___________X_______________________X_______________________X_
 *       __________      _________________                    _________________________
 * #EMRD ____/     \____/                 \__________________/
 *       ______________________      ______________________________                  __
 * #EMWR                       \____/                              \________________/
 *                     __        ____                       ___      _______________
 * DATA  -------------<__>------<____>---------------------<___>----<_______________>--
 *       _____________________________________________v_______________________v________
 * ERDY  ____________________________________________/ \_____________________/ \_______
 */
// clang-format on

namespace {

// XIN runs free from the PWM, and CLK is XIN/2. ERDY, low except to end a
// cycle, holds the CPU in each external cycle: it is honoured only with
// the wait states SCR1 inserts, which stay on (SCR1 resets to them).

constexpr uint32_t xin_hz = 18750000;  // 150 MHz / 8; CLK 9.375 MHz
// The clock stays at least 1 MHz.
constexpr uint32_t xin_min_hz = 2000000;
#ifdef TUNE_CLOCK
// K may overclock the CPU.
constexpr auto xin_half_min_ns = 20;
#else
constexpr auto xin_half_min_ns = 25;  // CLK at most 10 MHz
#endif
// Teensy cycles in a CLK clock at the slowest rate, so the waits below
// hold at any rate.
constexpr uint32_t clk_max_cycles = 2 * (F_CPU / xin_min_hz);
// RESET_ must stay low for at least three clocks.
constexpr auto reset_ns = 5 * 2 * (1000000000 / xin_min_hz);
// Waits for a bus cycle last 1000 clocks, far past an instruction's
// internal states.
constexpr auto cycle_cycles = 1000 * clk_max_cycles;
// A strobe ends within a few clocks of ERDY being raised.
constexpr auto strobe_cycles = 4 * clk_max_cycles;
// The address mux settles after getAddr() switches ASEL.
constexpr auto asel_ns = 20;
// Bus cycles execute() takes before giving up on its injection.
constexpr auto inject_cycles = 256;
// Bus cycles between polls of the halt switch in a run: its yield()
// services the USB while ERDY holds the CPU, which would otherwise
// stretch every cycle. The devices ration their own polling.
constexpr auto halt_poll_cycles = 16;

inline void assert_nmi() {
    digitalWriteFast(PIN_NMI, LOW);
}

inline void negate_nmi() {
    digitalWriteFast(PIN_NMI, HIGH);
}

// IR0 is active high.
inline void assert_int() {
    digitalWriteFast(PIN_IR0, HIGH);
}

inline void negate_int() {
    digitalWriteFast(PIN_IR0, LOW);
}

// ERDY low inserts wait states, holding the CPU in its bus cycle.
inline void assert_ready() {
    digitalWriteFast(PIN_ERDY, HIGH);
}

inline void negate_ready() {
    digitalWriteFast(PIN_ERDY, LOW);
}

void negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
}

constexpr uint8_t PINS_LOW[] = {
        PIN_RESET,
        PIN_IR0,
        PIN_ASEL,
        PIN_ERDY,
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_BREQ,
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
        PIN_CLK,
        PIN_M1,
        PIN_EMRD,
        PIN_EMWR,
        PIN_EIORD,
        PIN_EIOWR,
        PIN_BACK,
};

}  // namespace

PinsKl5c80::PinsKl5c80()
    : PinsZ80Base(), _clock(xin_half_min_ns, xin_min_hz) {
    _devs = new DevsZ80();
    _regs = new RegsZ80(this);
    _mems = new MemsZ80();
    mems<MemsZ80>()->setMaxAddr(0x7FFFF);  // 512kB
#ifdef TUNE_CLOCK
    _clockInput = xin_hz;
#endif
}

void PinsKl5c80::resetPins() {
    // RESET_ and ERDY are asserted first: a glitch on XIN is harmless.
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
    Cycles::reset();
    // ERDY holds the first fetch.
    prepareWait();
    enableExternalReady();
    _regs->setIp(InstZ80::ORG_RESET);
    _regs->save();
}

void PinsKl5c80::enableExternalReady() {
    static constexpr uint8_t PUSH_AF[] = {
            0xF5,  // PUSH AF
    };
    struct {
        uint8_t a;
        uint8_t f;
    } reg;
    captureWrites(PUSH_AF, sizeof(PUSH_AF), (uint8_t *)&reg, sizeof(reg));
    uint8_t config[] = {
            // Enable #M1,#NMI,#BREQ,#BACK
            // memory 1-wait, I/O 2-wait, wide write option
            0x3E, 0x4F,          // LD A, <SCR1>
            0xD3, 0x3B,          // OUT (SCR1), A
            0xF1, reg.f, reg.a,  // POP_AF
            0x18, 0xF8,          // JR $-6
    };
    execInst(config, sizeof(config));
}

// Returns the cycle a strobe starts, held by ERDY.
Signals *PinsKl5c80::prepareCycle() const {
    const auto s = Signals::put();
    if (!waitUntil([s] { return s->getControl(); }, cycle_cycles)) {
        // Give up rather than wedge the board, leaving the halt switch to
        // stop the run.
        cli.println("?halt: no bus cycle");
        s->noCycle();
        return s;
    }
    s->getAddr();
    delayNanoseconds(asel_ns);
    s->getAddrHigh();
    return s;
}

Signals *PinsKl5c80::completeCycle(Signals *s) const {
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
    // An interrupt here would leave ERDY high into the next cycle, which
    // then ends unserved.
    noInterrupts();
    // assert_debug();
    assert_ready();
    waitUntil([] { return !Signals::busy(); }, strobe_cycles);
    negate_ready();
    // negate_debug();
    interrupts();
    // Kept off the release: the CPU is already held in its next cycle.
    Cycles::next();
    s->inputMode();  // data bus hold is acting
    return s;
}

// The CPU comes to its next cycle by itself and waits there.
Signals *PinsKl5c80::prepareWait() const {
    return prepareCycle();
}

Signals *PinsKl5c80::resumeCycle(uint16_t pc) const {
    const auto s = Signals::put();
    waitUntil([s] { return s->getControl(); }, cycle_cycles);
    s->setAddress(pc);
    return completeCycle(s);
}

Signals *PinsKl5c80::inject(uint8_t data) const {
    return completeCycle(prepareCycle()->inject(data));
}

uint16_t PinsKl5c80::execute(
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

bool PinsKl5c80::isRst38Break(const Signals *org_rst) const {
    // The RST 38H fetch, its two pushes, then this vector fetch.
    const auto rst38h = org_rst->prev(3);
    if (rst38h->data == InstZ80::RST38H) {
        if (org_rst->prev(1)->mwr() && org_rst->prev(2)->mwr()) {
            if (isBreakPoint(rst38h->addr))
                return true;  // break point
            if (_mems->read(InstZ80::ORG_RST38H) == InstZ80::RST38H)
                return true;  // halt to system
        }
    }
    return false;
}

Signals *PinsKl5c80::loop() {
    resumeCycle(_regs->nextIp());
    auto poll = 0;
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
        _devs->loop();
        if (++poll >= halt_poll_cycles) {
            poll = 0;
            if (haltSwitch())
                return suspend();
        }
    }
}

void PinsKl5c80::run() {
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

Signals *PinsKl5c80::suspend() {
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
            // Park at the fetch RET returns to: what follows injects from
            // there, and with no wait states nothing else holds the CPU.
            prepareWait();
            return s->prev(3);
        }
        completeCycle(s);
    }
    negate_nmi();
    prepareWait();
    cli.println("?halt: no NMI acknowledge");
    return nullptr;
}

bool PinsKl5c80::rawStep() {
    const auto pc = _regs->nextIp();
    if (_mems->read_byte(pc) == InstZ80::HALT)
        return false;
    resumeCycle(pc);
    const auto s = suspend();
    if (s == nullptr)
        return false;
    Cycles::discard(s);
    return true;
}

bool PinsKl5c80::step(bool show) {
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

void PinsKl5c80::assertInt(uint8_t) {
    assert_int();
}

void PinsKl5c80::negateInt(uint8_t) {
    negate_int();
}

void PinsKl5c80::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

const SignalsImpl *PinsKl5c80::findBacktraceStart() {
    return backtraceStartByFetchCount<Signals>(_lineLimit);
}

void PinsKl5c80::printBacktrace() {
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

}  // namespace kl5c80
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
