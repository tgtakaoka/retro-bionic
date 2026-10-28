#include "pins_mc68008.h"
#include "debugger.h"
#include "devs_mc68000.h"
#include "digital_bus.h"
#include "inst_mc68000.h"
#include "mems_mc68000.h"
#include "regs_mc68000.h"
#include "signals_mc68000.h"

namespace debugger {
namespace mc68000 {

/**
 * MC68008 bus cycle (M68000UM 5.1, Figures 5-2 and 5-4). CLK never stops:
 * a half longer than 250 ns is out of spec (10.9), so CLK runs from the
 * PWM from reset on, and the CPU waits for the Teensy by #DTACK alone.
 * A cycle is eight half-clock states, S0 opening at a rising edge. The
 * address, FC2:FC0 and R/#W are valid in S1, and #AS asserts in S2, with
 * #DS on a read; a write drives data in S3 and asserts #DS in S4.
 * #DTACK is sampled at the fall of S4: until it is asserted, wait states
 * repeat there. Read data is latched at the fall of S6, and the strobes
 * negate in S7. So a cycle is served while the CPU waits: #DTACK is
 * asserted once it is, and negated as soon as #AS negates, well before
 * the next cycle's S4.
 */

namespace {

constexpr auto ADDR_MASK_P = 0x0FFFFF;   // MC68008P: A0-A19
constexpr auto ADDR_MASK_FN = 0x3FFFFF;  // MC68008FN: A0-A21

// ATTENTION: unmeasured. 74AHCT157 select to output, 7.5ns typical at 5V.
constexpr auto asel_delay_ns = 15;
// #RESET and #HALT held together this long: 100 ms at power-up (M68000UM
// 5.5), and a reset can come at any time.
constexpr auto reset_ms = 100;
// No #AS for this many clocks: the CPU stopped in STOP, or halted on a
// double bus fault. Long enough to clear the slowest instruction.
constexpr auto halt_clocks = 1024;
// Clocks between polls of a CPU already stopped: short enough to service
// the devices and the halt switch, and to tick the watchdog, often.
constexpr auto wait_poll_clocks = 16;
// Bus cycles between polls of the halt switch in a run: its yield()
// services the USB while the CPU waits on #DTACK.
constexpr auto halt_poll_cycles = 16;
// Bus cycles allowed for a level-7 acknowledge; generous.
constexpr auto irq_ack_cycles = 1024;
// ATTENTION: unverified. Cycles before an exception's vector read that may
// hold its frame: six byte writes, an interrupt's acknowledge between
// them, and the reads between.
constexpr auto frame_lookback = 16;
// A cycle #DTACK ends negates #AS within a few clocks; this bounds the
// wait, interrupts masked, when it doesn't. Bounds the wait for a
// write's #DS too.
constexpr auto dtack_hold_clocks = 16;
// ATTENTION: unmeasured. CLK at 150 MHz / 20: the 8 MHz parts take 2-8
// MHz with halves of at least 55 ns (M68000UM 10.9).
constexpr uint32_t clock_input_hz = 7500000;
constexpr uint32_t clock_input_min_hz = 2000000;
constexpr auto clock_half_min_ns = 55;

const uint8_t PINS_LOW[] = {
        PIN_RESET,
        PIN_HALT,
        PIN_ASEL,
};

const uint8_t PINS_HIGH[] = {
        PIN_DTACK,
        PIN_BERR,
        PIN_IPL20,
        PIN_IPL1,
};

const uint8_t PINS_INPUT[] = {
        PIN_AS,
        PIN_RW,
        PIN_DS,
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
        PIN_AM0,
        PIN_AM1,
        PIN_AM2,
        PIN_AM3,
        PIN_AH0,
        PIN_AH1,
        PIN_AH2,
        PIN_AH3,
        PIN_FC0,
        PIN_FC1,
        PIN_FC2,
};

// #RESET and #HALT are open drain both ways, with no pull-up on the board:
// drive them low or let the level shifters' pull-ups have them, never
// drive them high.
void assert_reset() {
    pinMode(PIN_RESET, OUTPUT);
    digitalWriteFast(PIN_RESET, LOW);
    pinMode(PIN_HALT, OUTPUT);
    digitalWriteFast(PIN_HALT, LOW);
}

void release_reset() {
    pinMode(PIN_HALT, INPUT_PULLUP);
    pinMode(PIN_RESET, INPUT_PULLUP);
}

inline void assert_dtack() {
    digitalWriteFast(PIN_DTACK, LOW);
}

inline void negate_dtack() {
    digitalWriteFast(PIN_DTACK, HIGH);
}

inline bool memRead(const Signals *s) {
    return s->read() && !s->iack();
}

inline bool memWrite(const Signals *s) {
    return s->write();
}

// The byte a read at |addr| moves from |inst| placed at |org|. Outside the
// window it is a NOP, so a prefetch past the end runs nothing.
uint8_t busByte(
        const uint8_t *inst, uint_fast8_t len, uint32_t org, uint32_t addr) {
    const auto off = addr - org;
    if (addr >= org && off < len)
        return inst[off];
    return (addr & 1) ? InstMc68000::NOP & 0xFF : InstMc68000::NOP >> 8;
}

}  // namespace

PinsMc68008::PinsMc68008()
    : _clock(clock_half_min_ns, clock_input_min_hz) {
    _devs = new DevsMc68000(ADDR_MASK_P);
    _mems = new MemsMc68000(ADDR_MASK_P);
    _regs = new RegsMc68000(this, mems<MemsMc68000>());
    _clockInput = clock_input_hz;
    _clockCycles = F_CPU / _clockInput;
}

bool PinsMc68008::setClockInput(uint32_t hz) {
    const auto rate = _clock.setRate(hz);
    if (rate == 0)
        return false;
    _clockInput = rate;
    _clockCycles = F_CPU / rate;
    return true;
}

uint32_t PinsMc68008::wrap(uint32_t addr) const {
    return mems<MemsMc68000>()->wrap(addr);
}

void PinsMc68008::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    _intr = _halting = false;
    // The CPU is held in reset: a glitch on CLK is harmless. From here on
    // it never stops.
    _clock.begin(_clockInput);
    _clock.start();

    Cycles::reset();
    assert_reset();
    delay(reset_ms);
    release_reset();

    // The reset vector: SSP, then PC, read from 000000H. ATTENTION:
    // unverified that it reads as supervisor program (FC 6). Anything
    // else means the board, most likely the address mux, is not what the
    // driver assumes.
    const auto s = prepareCycle();
    if (s->halt() || !memRead(s) || !s->program() ||
            s->addr != InstMc68000::VEC_RESET) {
        cli.print("?reset: first cycle ");
        if (s->halt()) {
            cli.println(digitalReadFast(PIN_HALT) == LOW ? "none, #HALT low"
                                                         : "none");
        } else {
            s->print();
        }
        return;
    }
    // Answered with the park origin as both SSP and PC: the save only
    // pushes below it, and those writes are captured.
    static constexpr uint8_t VECTOR[] = {
            uint8_t(InstMc68000::ORG_PARK >> 24),
            uint8_t(InstMc68000::ORG_PARK >> 16),
            uint8_t(InstMc68000::ORG_PARK >> 8),
            uint8_t(InstMc68000::ORG_PARK),  // SSP
            uint8_t(InstMc68000::ORG_PARK >> 24),
            uint8_t(InstMc68000::ORG_PARK >> 16),
            uint8_t(InstMc68000::ORG_PARK >> 8),
            uint8_t(InstMc68000::ORG_PARK),  // PC
    };
    uint32_t org = InstMc68000::VEC_RESET;
    execInst(VECTOR, sizeof(VECTOR), org, InstMc68000::ORG_PARK);
    if (_cutShort) {
        cli.println("?reset: no fetch from the reset vector");
        printCycles();
        return;
    }
    detectBoard();
    _regs->setIp(org);
    _regs->save();
    _regs->reset();
}

// Parked at the fetch of ORG_PARK, 0FF000H: A16-A17 are 11 and A20-A21
// 00. The P board's P34-P35 carry A16-A17 under either select; the FN
// board's switch to A20-A21. P36-P37 are left out: on the FN board the
// second 157 has no A22-A23 for them. Its 22 address lines then move the
// ACIA, which is the board's wiring: set only when the board changes.
void PinsMc68008::detectBoard() {
    const uint32_t sel0 = PIN(ADDR);
    digitalWriteFast(PIN_ASEL, HIGH);
    delayNanoseconds(asel_delay_ns);
    const uint32_t sel1 = PIN(ADDR);
    digitalWriteFast(PIN_ASEL, LOW);
    const auto fn = ((sel0 >> 16) & 3) != ((sel1 >> 16) & 3);
    if (fn == _fn)
        return;
    _fn = fn;
    const auto mask = fn ? ADDR_MASK_FN : ADDR_MASK_P;
    mems<MemsMc68000>()->setAddrMask(mask);
    devs<DevsMc68000>()->setAddrMask(mask);
}

// ASEL rests at 0. One read of GPIO6 gives A0-A7, and one of GPIO7 per
// select A8-A11 and A16-A19 with FC2:FC0, then A12-A15 and, on the FN
// board, A20-A21. The CPU holds the address until #DTACK.
void PinsMc68008::getAddr(Signals *s) const {
    const uint32_t al = busRead(AL);
    const uint32_t sel0 = PIN(ADDR);
    digitalWriteFast(PIN_ASEL, HIGH);
    delayNanoseconds(asel_delay_ns);
    const uint32_t sel1 = PIN(ADDR);
    digitalWriteFast(PIN_ASEL, LOW);
    uint32_t addr = al | (sel0 & 0xF) << 8 | (sel1 & 0xF) << 12 |
                    ((sel0 >> 16) & 0xF) << 16;
    if (_fn)
        addr |= ((sel1 >> 16) & 3) << 20;
    s->addr = addr;
    s->setFc((sel0 >> 10) & 7, false);
}

Signals *PinsMc68008::prepareCycle() {
    return prepareCycle(halt_clocks);
}

Signals *PinsMc68008::prepareCycle(uint_fast16_t clocks) {
    auto s = Signals::put();
    s->clearMark();  // Cycles::next() keeps the slot's old mark
    const uint32_t start = ARM_DWT_CYCCNT;
    const uint32_t limit = clocks * _clockCycles;
    uint8_t cntl;
    while ((cntl = busRead(CNTL)) & Signals::AS) {
        if (ARM_DWT_CYCCNT - start >= limit) {
            s->markHalt();
            return s;
        }
    }
    s->cntl() = cntl;
    getAddr(s);
    return s;
}

// The CPU waits in the cycle it is parked in, so its address and status
// are read again from the bus.
Signals *PinsMc68008::resumeCycle() {
    auto s = Signals::put();
    s->clearMark();
    s->cntl() = busRead(CNTL);
    if (!s->strobe()) {
        s->markHalt();
        return s;
    }
    getAddr(s);
    return s;
}

Signals *PinsMc68008::completeCycle(Signals *s) {
    if (s->halt()) {
        // Nothing moves; not recorded, so s->prev() stays the previous
        // real cycle.
        busMode(D, INPUT);
        return s;
    }
    const uint32_t limit = dtack_hold_clocks * _clockCycles;
    if (s->read()) {
        serveRead(s);
        busWrite(D, s->data);
        busMode(D, OUTPUT);
    } else {
        // ATTENTION: unmeasured. Write data is driven from S3, #DS asserted
        // in S4, so the data is read as #DS is first seen low.
        const uint32_t start = ARM_DWT_CYCCNT;
        while (busRead(CNTL) & Signals::LDS) {
            if (ARM_DWT_CYCCNT - start >= limit)
                break;
        }
        s->data = busRead(D);
        storeWrite(s);
    }
    // Masked: an interrupt here can outlast #AS high between cycles, and
    // the next one would end on this #DTACK unserved. ATTENTION:
    // unmeasured. The next cycle's S4 samples #DTACK about two clocks
    // after #AS negates, which bounds how late it may be negated.
    noInterrupts();
    assert_dtack();
    const uint32_t start = ARM_DWT_CYCCNT;
    while ((busRead(CNTL) & Signals::AS) == 0) {
        if (ARM_DWT_CYCCNT - start >= limit)
            break;
    }
    negate_dtack();
    interrupts();
    if (s->read())
        busMode(D, INPUT);  // only a read drove the bus
    Cycles::next();
    return s;
}

// An acknowledge takes a vector number: the debugger's for level 7, else
// the interrupting device's.
void PinsMc68008::serveRead(Signals *s) {
    if (!s->readMemory())
        return;
    if (s->iack()) {
        const auto vec = _devs->vector();
        s->data = vec ? vec : InstMc68000::VECNUM_SPURIOUS;
        return;
    }
    const auto addr = wrap(s->addr);
    if (_devs->isSelected(addr)) {
        s->data = _devs->read(addr);
    } else {
        s->data = _mems->read_byte(addr);
    }
}

void PinsMc68008::storeWrite(Signals *s) {
    if (!s->writeMemory())
        return;
    const auto addr = wrap(s->addr);
    if (_devs->isSelected(addr)) {
        _devs->write(addr, s->data);
    } else {
        _mems->write_byte(addr, s->data);
    }
}

uint_fast8_t PinsMc68008::execute(const uint8_t *inst, uint_fast8_t len,
        uint8_t *buf, uint32_t *addrs, uint_fast8_t max, uint32_t &org,
        uint32_t exit) {
    uint_fast8_t cap = 0;
    // |inst| is answered by address. |exit| is where the sequence ends --
    // a prefetch past the window looks just like falling through, so the
    // caller must say.
    const uint32_t leaves = exit == EXIT_ORG ? org : wrap(exit);
    bool started = false;
    // The exit may lie inside the window: it counts only once the last
    // byte has been read.
    bool fetchedAll = false;
    bool exited = false;
    auto s = resumeCycle();
    // Bound the damage: a wrong dump beats a hung debugger.
    auto guard = Cycles::MAX_CYCLES;
    while (guard--) {
        if (s->halt())
            break;
        const auto reading = memRead(s);
        const auto atExit = fetchedAll && reading && s->program() &&
                            (s->addr & ~1) == (leaves & ~1);
        if (started && cap >= max && atExit) {
            exited = true;
            break;
        }
        if (reading)
            started = true;
        const auto inWindow = reading && s->addr >= org && s->addr < org + len;
        if (inWindow && s->addr == org + len - 1)
            fetchedAll = true;
        // Program reads outside the window are prefetch: NOPs. Data reads
        // there, an RTE's frame, come from memory.
        if (inWindow || (reading && s->program())) {
            s->inject(busByte(inst, len, org, s->addr));
        } else if (memWrite(s)) {
            s->capture();
        }
        completeCycle(s);
        if (memWrite(s) && cap < max) {
            if (buf)
                buf[cap] = s->data;
            if (addrs)
                addrs[cap] = wrap(s->addr);
            ++cap;
        }
        s = prepareCycle();
    }
    // The CPU is parked in |s|: the caller resumes from its address, which
    // is |leaves| unless the guard ran out.
    _cutShort = !exited;
    org = s->addr;
    return cap;
}

void PinsMc68008::execInst(
        const uint8_t *inst, uint_fast8_t len, uint32_t &org, uint32_t exit) {
    execute(inst, len, nullptr, nullptr, 0, org, exit);
}

uint_fast8_t PinsMc68008::captureWrites(const uint8_t *inst, uint_fast8_t len,
        uint8_t *buf, uint32_t *addrs, uint_fast8_t max, uint32_t &org,
        uint32_t exit) {
    return execute(inst, len, buf, addrs, max, org, exit);
}

bool PinsMc68008::answerVector(uint32_t vecAddr, uint32_t handler) {
    const uint8_t vector[] = {
            uint8_t(handler >> 24),
            uint8_t(handler >> 16),
            uint8_t(handler >> 8),
            uint8_t(handler),
    };
    auto org = vecAddr;
    execute(vector, sizeof(vector), nullptr, nullptr, 0, org, handler);
    return !_cutShort;
}

// The frame an exception stacked right before its vector read |s|: SR,
// then PC (M68000UM Figure 6-5), as the writes left them in memory. Six
// byte writes, with an interrupt's acknowledge among them (6.3.2), and no
// other read: the first one back ends the frame, so a push of the
// program's own, or the frame of a TRAP stepped before a trace, is never
// taken for it. |from| becomes the first of them.
bool PinsMc68008::findFrame(const Signals *s, RegsMc68000::Frame &frame,
        const Signals *&from) const {
    uint_fast8_t bytes = 0;
    uint32_t low = UINT32_MAX;
    for (auto i = 1; i <= frame_lookback && bytes < 6; ++i) {
        const auto t = s->prev(i);
        if (t->iack())
            continue;
        if (!memWrite(t))
            break;
        ++bytes;
        if (t->addr < low)
            low = t->addr;
        from = t;
    }
    if (bytes < 6)
        return false;
    const auto mems = this->mems<MemsMc68000>();
    frame.sr = mems->read16(low);
    frame.pc = mems->read32(low + 2);
    frame.ssp = wrap(low + 6);
    return true;
}

// The level on #IPL2-#IPL0, active low: 7 for a halt, 5 for a device.
// #IPL2 and #IPL0 are tied, so levels 2, 5 and 7 are all there is.
void PinsMc68008::setIpl() {
    const uint8_t level = _halting ? 7 : (_intr ? 5 : 0);
    digitalWriteFast(PIN_IPL20, (level & 5) ? LOW : HIGH);
    digitalWriteFast(PIN_IPL1, (level & 2) ? LOW : HIGH);
}

void PinsMc68008::assertInt(uint8_t) {
    _intr = true;
    setIpl();
}

void PinsMc68008::negateInt(uint8_t) {
    _intr = false;
    setIpl();
}

// Stop at an instruction boundary: level 7 is taken there, on its change
// from a lower level, and the CPU parks in its vector read with PC and SR
// stacked. With |s| null the CPU has no bus cycle, stopped in STOP, and
// level 7 wakes it.
bool PinsMc68008::suspend(Signals *s) {
    _halting = true;
    setIpl();
    if (s == nullptr) {
        s = prepareCycle();
    } else {
        completeCycle(s);
        s = prepareCycle();
    }
    const auto vecAddr = InstMc68000::vecAddr(InstMc68000::VECNUM_HALT);
    auto guard = irq_ack_cycles;
    while (guard-- && !s->halt()) {
        if (s->iack() && s->level() == 7) {
            s->inject(InstMc68000::VECNUM_HALT);
            // Back to the device level: the next halt is a new change to 7.
            _halting = false;
            setIpl();
        }
        if (memRead(s) && s->dataSpace() && s->addr == vecAddr) {
            RegsMc68000::Frame frame;
            const Signals *from = nullptr;
            if (findFrame(s, frame, from)) {
                // The frame and this read are the debugger's.
                Cycles::discard(from);
                regs<RegsMc68000>()->parkInVector(vecAddr, frame);
                return true;
            }
        }
        completeCycle(s);
        s = prepareCycle();
    }
    _halting = false;
    setIpl();
    cli.println("?halt: no level 7 acknowledge");
    return false;
}

// One instruction: T set in the restored SR takes the trace exception after
// it, or before the first instruction of any exception it raised, and its
// vector read parks the CPU.
bool PinsMc68008::step(bool show) {
    Cycles::reset();
    regs<RegsMc68000>()->restore(true);
    if (_cutShort) {
        cli.println("?step: restore cut short");
        return false;
    }
    if (show)
        Cycles::reset();
    auto s = resumeCycle();
    auto parked = false;
    for (auto guard = Cycles::MAX_CYCLES; guard && !parked; --guard) {
        if (s->halt()) {
            // STOP: level 7 wakes it.
            if (!suspend(nullptr))
                return false;
            parked = true;
            break;
        }
        if (memRead(s) && s->dataSpace() &&
                s->addr == InstMc68000::VEC_TRACE) {
            RegsMc68000::Frame frame;
            const Signals *from = nullptr;
            if (findFrame(s, frame, from)) {
                frame.sr &= ~InstMc68000::SR_T;
                regs<RegsMc68000>()->parkInVector(s->addr, frame);
                Cycles::discard(from);
                parked = true;
                break;
            }
        }
        completeCycle(s);
        s = prepareCycle();
    }
    if (!parked) {
        cli.println("?step: no trace vector read");
        return false;
    }
    Cycles::Hold hold;  // keep the step's cycles for printCycles()
    _regs->save();
    if (show)
        printCycles();
    return true;
}

// Free-run. Returns whether the CPU stopped at a boundary its registers can
// be saved from: an ILLEGAL, the debugger's breakpoint, or a halt. The
// ring is left holding the program's cycles only.
bool PinsMc68008::loop() {
    auto s = resumeCycle();
    uint_fast8_t poll = 0;
    while (true) {
        // First: a slot that timed out holds the status and address of
        // its use a ring ago.
        if (s->halt()) {
            // STOP: no bus cycle until an interrupt.
            _devs->loop();
            if (haltSwitch())
                return suspend(nullptr);
            s = prepareCycle(wait_poll_clocks);
            continue;
        }
        if (memRead(s) && s->dataSpace() &&
                s->addr == InstMc68000::VEC_ILLEGAL) {
            RegsMc68000::Frame frame;
            const Signals *from = nullptr;
            if (findFrame(s, frame, from)) {
                // ILLEGAL stacks its own address.
                regs<RegsMc68000>()->parkInVector(s->addr, frame);
                Cycles::discard(from);
                return true;
            }
        }
        completeCycle(s);
        _devs->loop();
        s = prepareCycle();
        // Only now, with a cycle recorded: suspend() completes it first.
        // Not at an acknowledge: the program's own interrupt is under way.
        if (++poll < halt_poll_cycles || s->iack())
            continue;
        poll = 0;
        if (haltSwitch())
            return suspend(s->halt() ? nullptr : s);
    }
}

void PinsMc68008::run() {
    _regs->restore();
    Cycles::reset();
    saveBreakInsts();
    startRunTimer();
    const auto stopped = loop();
    stopRunTimer();
    restoreBreakInsts();
    if (stopped) {
        Cycles::Hold hold;
        _regs->save();
    }
    disassembleCycles();
}

void PinsMc68008::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr & ~1, InstMc68000::ILLEGAL);
}

void PinsMc68008::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

// No pin marks an instruction start: every cycle.
void PinsMc68008::printBacktrace() {
    printCycles();
}

}  // namespace mc68000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
