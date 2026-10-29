#include "pins_z180.h"
#include "debugger.h"
#include "devs_z80.h"
#include "inst_z80.h"
#include "mems_z80.h"
#include "regs_z180.h"
#include "signals_z180.h"

#define USART_Z180 0x0140

namespace debugger {
namespace z180 {

using z80::InstZ80;
using z80::MemsZ80;
using z80::waitUntil;

// clang-format off
/**
 * Z180 bus cycle.  0 wait / 1 wait
 *        __    __    __    __    __    __    __    __    __    __    __    __    __
 * EXTAL |  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__
 *          \ ____\     \ ____\     \____\     \ ____\      \____\     \ ____\       __
 *   PHI ___|    T|1____|    T|2____|    T|3____|    T|1____|    T|2____|    T|3____|
 *       ___\ ____\_____\_____v_____\ __________\ ____\_____\ ____v_________________\ _
 * ADDR  ____X___________________________________X____________X______________________X_
 *       __________                             _______________________________________
 * #MREQ           \___________________________/
 *       ______________________________________________                             ___
 * #IORQ                                               \___________________________/
 *       __________                             _______                             ___
 *   #RD           \___________________________/       \___________________________/
 *       _______________                        ____________                        ___
 *   #WR                \______________________/            \______________________/
 *                                  ____________           ________________________
 *  DATA  -------------------------<____________>---------<________________________>----
 *       _____________________v___________________________________v____________________
 * #WAIT  ___________________/ \_________________________________/ \___________________
 */
// clang-format on

namespace {

// EXTAL runs free from the PWM, and PHI is EXTAL/2. #WAIT, asserted except
// to end a cycle, holds the CPU in a wait state from T2 of each external
// memory and I/O cycle; it is sampled on each PHI fall from T2 on. An
// internal I/O cycle ignores #WAIT but repeats on the bus.

// The clock must remain active: PHI periods of at most 2 us.
constexpr uint32_t extal_min_hz = 1000000;
#ifdef TUNE_CLOCK
// K may overclock the CPU.
constexpr auto extal_half_min_ns = 20;
#else
constexpr auto extal_half_min_ns = 25;
#endif
// Teensy cycles in a PHI clock at the slowest rate, so the waits below
// hold at any rate.
constexpr uint32_t phi_max_cycles = 2 * (F_CPU / extal_min_hz);
// #RESET must be active for at least six clocks.
constexpr auto reset_ns = 10 * 2 * (1000000000 / extal_min_hz);
// Waits for a bus cycle last 1000 clocks, far past an instruction's
// internal states; SLEEP makes none.
constexpr auto cycle_cycles = 1000 * phi_max_cycles;
// A strobe ends within a TW and T3 of #WAIT being negated.
constexpr auto strobe_cycles = 4 * phi_max_cycles;
// Refreshes prepareCycle() skips before giving up; one follows each
// instruction fetch at most.
constexpr auto skip_cycles = 16;
// Bus cycles execute() takes before giving up on its injection.
constexpr auto inject_cycles = 256;
// Bus cycles between polls of the halt switch in a run: its yield()
// services the USB while #WAIT holds the CPU, which would otherwise
// stretch every cycle. The devices ration their own polling.
constexpr auto halt_poll_cycles = 16;

inline void assert_nmi() {
    digitalWriteFast(PIN_NMI, LOW);
}

inline void negate_nmi() {
    digitalWriteFast(PIN_NMI, HIGH);
}

inline void assert_int0() {
    digitalWriteFast(PIN_INT0, LOW);
}

inline void negate_int0() {
    digitalWriteFast(PIN_INT0, HIGH);
}

inline void negate_wait() {
    digitalWriteFast(PIN_WAIT, HIGH);
}

inline void assert_wait() {
    digitalWriteFast(PIN_WAIT, LOW);
}

void negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
}

constexpr uint8_t PINS_LOW[] = {
        PIN_RESET,
        PIN_ASEL,
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_WAIT,
        PIN_BUSREQ,
        PIN_NMI,
        PIN_INT0,
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
        PIN_PHI,
        PIN_M1,
        PIN_MREQ,
        PIN_IORQ,
        PIN_RD,
        PIN_WR,
        PIN_BUSACK,
};

// Set 0-wait for memory and 1-wait for I/O (DCNTL)
// Disable DRAM refresh (RCR)
constexpr uint8_t Z180_CONFIG[] = {
        0xAF,              // XOR A
        0xED, 0x39, 0x32,  // OUT0 (DCNTL), A
        0xED, 0x39, 0x36,  // OUT0 (RCR), A
        0x18, 0xF7         // JR $-7
};
}  // namespace

PinsZ180::PinsZ180(const uint8_t *config, uint_fast8_t len, uint32_t extalHz,
        uint16_t internalIo, bool holdsEarly)
    : PinsZ80Base(),
      _clock(extal_half_min_ns, extal_min_hz),
      _config(config ? config : Z180_CONFIG),
      _configLen(config ? len : sizeof(Z180_CONFIG)),
      _internalIo(internalIo),
      _extalHz(extalHz),
      _holdsEarly(holdsEarly) {
    _devs = new DevsZ80(USART_Z180);
    _regs = new RegsZ180(this);
    _mems = new MemsZ80();
    mems<MemsZ80>()->setMaxAddr(0xFFFFF);  // 1MB
#ifdef TUNE_CLOCK
    _clockInput = extalHz;
#endif
}

void PinsZ180::resetPins() {
    // #RESET is asserted first: a glitch on EXTAL is harmless.
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
#ifdef TUNE_CLOCK
    _clock.begin(_clockInput);
#else
    _clock.begin(_extalHz);
#endif
    _clock.start();
    delayNanoseconds(reset_ns);
    negate_reset();
    // #WAIT holds the first fetch, where configureCpu() resumes.
    assert_wait();
    Cycles::reset();
    prepareCycle();
    _regs->setIp(InstZ80::ORG_RESET);
    configureCpu();
    _regs->save();
}

void PinsZ180::configureCpu() {
    execInst(_config, _configLen);
}

// Waits for the strobe of the cycle |s| requests: #RD and #WR follow #MREQ
// and #IORQ.
void PinsZ180::awaitStrobe(Signals *s) const {
    const auto strobe = [s] {
        return !s->getControl() || s->read() || s->write() || s->intack();
    };
    // #WR falls part of a clock after #MREQ: releasing #WAIT before it
    // lets the write run unheld.
    if (_holdsEarly && !waitUntil(strobe, phi_max_cycles)) {
        // Held before its strobe: let it on to the strobe, then hold it
        // again from its next #WAIT sample.
        noInterrupts();
        negate_wait();
        waitUntil(strobe, strobe_cycles);
        assert_wait();
        interrupts();
    } else {
        waitUntil(strobe, strobe_cycles);
    }
}

// Returns the cycle a strobe starts, held by #WAIT unless an internal I/O
// cycle; a refresh is skipped.
Signals *PinsZ180::prepareCycle() const {
    const auto s = Signals::put();
    for (auto skip = 0;; ++skip) {
        if (skip >= skip_cycles) {
            // Give up on a request that never moves data.
            cli.print("?halt: no data cycle ");
            s->getAddr();
            s->print();
            s->noCycle();
            return s;
        }
        if (!waitUntil([s] { return s->getControl(); }, cycle_cycles)) {
            // Give up rather than wedge the board, leaving the halt switch
            // to stop the run.
            cli.println("?halt: no bus cycle");
            s->noCycle();
            return s;
        }
        awaitStrobe(s);
        if (!s->nobus()) {
            s->getAddr();
            s->getAddrHigh();
            return s;
        }
        // A refresh moves no data; wait past its end.
        waitUntil([s] { return !s->getControl(); }, strobe_cycles);
    }
}

Signals *PinsZ180::completeCycle(Signals *s) const {
    if (s->nobus())
        return s;
    if (s->mreq()) {      // Memory request
        if (s->read()) {  // Memory read
            if (s->readMemory())
                s->data = _mems->read(s->addr);
            s->outData();
        } else if (s->write()) {  // Memory write
            s->getData();
            if (s->writeMemory())
                _mems->write(s->addr, s->data);
        }
    } else {  // I/O request
        // using 16-bit for I/O device addressing.
        if (s->read() && _devs->isSelected(s->addr)) {
            s->data = _devs->read(s->addr);
            s->outData();
        } else if (s->write()) {  // I/O write
            s->getData();
            if (_devs->isSelected(s->addr))
                _devs->write(s->addr, s->data);
        } else if (s->intack()) {
            s->data = _devs->vector();
            s->outData();
        }
    }
    if (internalIo(s)) {
        // Not held: a pulse on #WAIT could release the next cycle.
        waitUntil([] { return !Signals::busy(); }, strobe_cycles);
        Cycles::next();
        s->inputMode();
        return s;
    }
    // An interrupt here would leave #WAIT negated into the next cycle,
    // which then ends unserved.
    noInterrupts();
    // assert_debug();
    negate_wait();
    waitUntil([] { return !Signals::busy(); }, strobe_cycles);
    assert_wait();
    // negate_debug();
    interrupts();
    // Kept off the release: the CPU is already held in its next cycle.
    Cycles::next();
    s->inputMode();  // data bus hold is acting
    return s;
}

bool PinsZ180::internalIo(const Signals *s) const {
    return s->iorq() && !s->intack() && (s->addr & 0xFFFF) < _internalIo;
}

Signals *PinsZ180::resumeCycle(uint16_t pc) {
    const auto s = Signals::put();
    // execute() may have stopped past an interrupt acknowledge, at its
    // handler's fetch rather than at |pc|.
    s->setAddress(_heldFetch ? _heldAddr : pc);
    _heldFetch = false;
    // The CPU comes to the fetch by itself and waits there.
    waitUntil([s] { return s->getControl(); }, cycle_cycles);
    awaitStrobe(s);
    return completeCycle(s);
}

Signals *PinsZ180::inject(uint8_t data) const {
    return completeCycle(prepareCycle()->inject(data));
}

uint16_t PinsZ180::execute(
        const uint8_t *inst, uint_fast8_t len, uint8_t *buf, uint_fast8_t max) {
    uint16_t addr = 0;
    uint_fast8_t inj = 0;
    uint_fast8_t cap = 0;
    auto s = Signals::put();
    for (auto n = 0; inj < len || cap < max; ++n) {
        if (n >= inject_cycles) {
            cli.println("?halt: injection stalled");
            break;
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
        if (!s->nobus()) {
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
        s = prepareCycle();
    }
    // Park on the next fetch, which may be past an interrupt acknowledge;
    // resumeCycle() starts there.
    auto guard = Cycles::MAX_CYCLES;
    while (guard-- && !s->fetch()) {
        completeCycle(s);
        s = prepareCycle();
    }
    _heldFetch = true;
    _heldAddr = s->addr;
    return addr;
}

bool PinsZ180::isRst38Break(const Signals *org_rst) const {
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

Signals *PinsZ180::loop() {
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

void PinsZ180::run() {
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

Signals *PinsZ180::suspend() {
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
            return s->prev(3);
        }
        completeCycle(s);
    }
    negate_nmi();
    cli.println("?halt: no NMI acknowledge");
    return nullptr;
}

bool PinsZ180::rawStep() {
    const auto pc = _regs->nextIp();
    if (_mems->read_byte(pc) == InstZ80::HALT)
        return false;
    assert_nmi();  // Before the fetch, to stop after this instruction
    resumeCycle(pc);
    const auto s = suspend();
    if (s == nullptr)
        return false;
    Cycles::discard(s);
    return true;
}

bool PinsZ180::step(bool show) {
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

void PinsZ180::assertInt(uint8_t) {
    assert_int0();
}

void PinsZ180::negateInt(uint8_t) {
    negate_int0();
}

void PinsZ180::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

const SignalsImpl *PinsZ180::findBacktraceStart() {
    return backtraceStartByFetchCount<Signals>(_lineLimit);
}

void PinsZ180::printBacktrace() {
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

}  // namespace z180
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
