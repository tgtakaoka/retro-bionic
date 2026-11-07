#include "pins_mc68000_base.h"
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
 * The 68000-family bus cycle (M68000UM 5.1, Figures 5-1 to 5-7). CLK never
 * stops: a half longer than 125 ns (250 ns on the MC68008) is out of spec
 * (10.8, 10.9), so CLK runs from the PWM from reset on, and the CPU waits
 * for the Teensy by #DTACK alone. A cycle is eight half-clock states, S0
 * opening at a rising edge. The address, FC2:FC0 and R/#W are valid in S1,
 * and #AS asserts in S2, with the data strobes on a read; a write drives
 * data in S3 and asserts its strobes in S4. #DTACK is sampled at the fall
 * of S4: until it is asserted, wait states repeat there. Read data is
 * latched at the fall of S6, and the strobes negate in S7. So a cycle is
 * served while the CPU waits: #DTACK is asserted once it is, and negated
 * as soon as #AS negates, well before the next cycle's S4.
 */

namespace {

// #RESET and #HALT held together this long: 100 ms at power-up (M68000UM
// 5.5), and a reset can come at any time.
constexpr auto reset_ms = 100;
// No #AS for this many clocks: the CPU stopped in STOP, or halted on a
// double bus fault. Long enough to clear the slowest instruction.
constexpr auto halt_clocks = 1024;
// Clocks between polls of a CPU already stopped: short enough to service
// the devices and the halt switch, and to tick the watchdog, often.
constexpr auto wait_poll_clocks = 16;
// Bus cycles allowed for a level-7 acknowledge; generous.
constexpr auto irq_ack_cycles = 1024;
// ATTENTION: unverified. Cycles before an exception's vector read that may
// hold its frame: up to eight byte writes, an interrupt's acknowledge
// between them, and the reads between.
constexpr auto frame_lookback = 16;
// A cycle #DTACK ends negates #AS within a few clocks; this bounds the
// wait, interrupts masked, when it doesn't. Bounds the wait for a
// write's data strobes too.
constexpr auto dtack_hold_clocks = 16;
// The 10 MHz parts take 2-10 MHz with halves of at least 45 ns (M68000UM
// 10.8, 10.9); K trusts the user with the grade.
constexpr uint32_t clock_input_min_hz = 2000000;
constexpr auto clock_half_min_ns = 45;
// Teensy cycles in a clock at the slowest rate: timeouts that hold at any.
constexpr uint32_t clock_cycles = F_CPU / clock_input_min_hz;

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

// The byte at |addr| of |inst| placed at |org|. Outside the window it is a
// NOP, so a prefetch past the end runs nothing.
uint8_t instByte(
        const uint8_t *inst, uint_fast8_t len, uint32_t org, uint32_t addr) {
    const auto off = addr - org;
    if (addr >= org && off < len)
        return inst[off];
    return (addr & 1) ? InstMc68000::NOP & 0xFF : InstMc68000::NOP >> 8;
}

// What a read |s| moves from |inst|: the aligned word, or its own byte.
uint16_t instData(const Signals *s, const uint8_t *inst, uint_fast8_t len,
        uint32_t org) {
    if (s->wordAccess()) {
        const auto even = s->addr & ~UINT32_C(1);
        return static_cast<uint16_t>(instByte(inst, len, org, even)) << 8 |
               instByte(inst, len, org, even + 1);
    }
    return instByte(inst, len, org, s->addr);
}

}  // namespace

PinsMc68000Base::PinsMc68000Base(uint8_t strobes, uint32_t clockInput)
    : _clockInput(clockInput),
      _clock(clock_half_min_ns, clock_input_min_hz),
      _strobes(strobes) {}

#ifdef TUNE_CLOCK
bool PinsMc68000Base::setClockInput(uint32_t hz) {
    const auto rate = _clock.setRate(hz);
    if (rate == 0)
        return false;
    _clockInput = rate;
    return true;
}
#endif

uint32_t PinsMc68000Base::wrap(uint32_t addr) const {
    return mems<MemsMc68000>()->wrap(addr);
}

void PinsMc68000Base::resetCpu() {
    _intr = _halting = false;
    setIpl();
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
    _regs->setIp(InstMc68000::ORG_PARK);
    _regs->save();
    _regs->reset();
}

Signals *PinsMc68000Base::prepareCycle() {
    return prepareCycle(halt_clocks);
}

Signals *PinsMc68000Base::prepareCycle(uint_fast16_t clocks) {
    auto s = Signals::put();
    s->clearMark();  // Cycles::next() keeps the slot's old mark
    const uint32_t start = ARM_DWT_CYCCNT;
    const uint32_t limit = clocks * clock_cycles;
#ifdef PROFILE_CYCLES
    assert_debug();  // polling for #AS
#endif
    uint8_t cntl;
    while ((cntl = busRead(CNTL)) & Signals::AS) {
        if (ARM_DWT_CYCCNT - start >= limit) {
            s->markHalt();
            return s;
        }
    }
#ifdef PROFILE_CYCLES
    negate_debug();  // #AS seen
#endif
    s->cntl() = cntl;
    getAddr(s);
    return s;
}

// The CPU waits in the cycle it is parked in, so its address and status
// are read again from the bus.
Signals *PinsMc68000Base::resumeCycle() {
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

Signals *PinsMc68000Base::completeCycle(Signals *s) {
    if (s->halt()) {
        // Nothing moves; not recorded, so s->prev() stays the previous
        // real cycle.
        inputMode();
        return s;
    }
    const uint32_t limit = dtack_hold_clocks * clock_cycles;
#ifdef PROFILE_CYCLES
    assert_debug();  // the cycle served
#endif
    if (s->read()) {
        serveRead(s);
#ifdef PROFILE_CYCLES
        negate_debug();  // looked up
#endif
        // The CPU latches read data a clock after it samples #DTACK, and
        // wants it no later than #DTACK plus M68000UM parameter #31.
        noInterrupts();
        assert_dtack();
        outData(s);
    } else {
        // ATTENTION: unmeasured. Write data is driven from S3, the data
        // strobes asserted in S4, so the data and its lanes are read as a
        // strobe is first seen low.
        const uint32_t start = ARM_DWT_CYCCNT;
        uint8_t cntl;
        while (((cntl = busRead(CNTL)) & _strobes) == _strobes) {
            if (ARM_DWT_CYCCNT - start >= limit)
                break;
        }
        s->cntl() = cntl;
        getLanes(s);
        getData(s);
        noInterrupts();
        assert_dtack();
    }
    // Masked from #DTACK on, or an interrupt could outlast #AS high and the
    // next cycle end on this #DTACK. #DTACK is negated about 50 ns after
    // #AS rises, well before the next cycle's S4. The store waits until
    // #DTACK is off: a device or an EXTMEM miss can outlast #AS.
    Cycles::next();
    const uint32_t start = ARM_DWT_CYCCNT;
    uint8_t cntl;
    while (((cntl = busRead(CNTL)) & Signals::AS) == 0) {
        if (ARM_DWT_CYCCNT - start >= limit)
            break;
        // TAS holds #AS from its read through its write (M68000UM Figure
        // 5-9): R/#W falls for the write, a clock before its data.
        if ((cntl & Signals::RW) == 0 && s->read()) {
            negate_dtack();
            inputMode();
            s = completeTasWrite(s, cntl);
            break;
        }
    }
    negate_dtack();
#ifdef PROFILE_CYCLES
    assert_debug();  // released
#endif
    interrupts();
    if (s->read()) {
        inputMode();  // only a read drove the bus
    } else {
        // Before the next cycle is looked up, so a read of the same
        // address sees it.
        storeWrite(s);
    }
#ifdef PROFILE_CYCLES
    negate_debug();  // back to the run loop
#endif
    return s;
}

// TAS's write, at the address its read left on the bus: served as its
// own cycle, from its strobes to #AS rising.
Signals *PinsMc68000Base::completeTasWrite(const Signals *r, uint8_t cntl) {
    const uint32_t limit = dtack_hold_clocks * clock_cycles;
    auto s = Signals::put();
    s->clearMark();
    s->addr = r->addr;
    s->setFc(r->fc(), false);
    const uint32_t start = ARM_DWT_CYCCNT;
    while ((cntl & _strobes) == _strobes) {
        if (ARM_DWT_CYCCNT - start >= limit)
            break;
        cntl = busRead(CNTL);
    }
    s->cntl() = cntl;
    getLanes(s);
    getData(s);
    assert_dtack();
    Cycles::next();
    while ((busRead(CNTL) & Signals::AS) == 0) {
        if (ARM_DWT_CYCCNT - start >= 2 * limit)
            break;
    }
    return s;
}

// An acknowledge takes a vector number: the debugger's for level 7, else
// the interrupting device's. A word read moves the aligned pair, the even
// byte high; a device takes a byte at a time.
void PinsMc68000Base::serveRead(Signals *s) {
    if (!s->readMemory())
        return;
    if (s->iack()) {
        const auto vec = _devs->vector();
        s->data = vec ? vec : InstMc68000::VECNUM_SPURIOUS;
        return;
    }
    // Through the final classes, so the hot path takes no virtual call.
    const auto mems = this->mems<MemsMc68000>();
    const auto devs = this->devs<DevsMc68000>();
    const auto addr = mems->wrap(s->addr);
    if (s->wordAccess()) {
        const auto even = addr & ~1;
        if (devs->isSelected(even)) {
            s->data = uint16(devs->read(even), devs->read(even + 1));
        } else {
            s->data = mems->read_bus(even);
        }
    } else if (devs->isSelected(addr)) {
        s->data = devs->read(addr);
    } else {
        s->data = mems->read_byte(addr);
    }
}

void PinsMc68000Base::storeWrite(Signals *s) {
    if (!s->writeMemory())
        return;
    const auto mems = this->mems<MemsMc68000>();
    const auto devs = this->devs<DevsMc68000>();
    const auto addr = mems->wrap(s->addr);
    if (s->wordAccess()) {
        const auto even = addr & ~1;
        if (devs->isSelected(even)) {
            devs->write(even, hi(s->data));
            devs->write(even + 1, lo(s->data));
        } else {
            mems->write_bus(even, s->data);
        }
    } else if (devs->isSelected(addr)) {
        devs->write(addr, s->data);
    } else {
        mems->write_byte(addr, s->data);
    }
}

uint_fast8_t PinsMc68000Base::execute(const uint8_t *inst, uint_fast8_t len,
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
        // A word read moves the aligned pair, so it is in the window when
        // either byte is.
        const auto first = s->wordAccess() ? s->addr & ~UINT32_C(1) : s->addr;
        const auto last = s->wordAccess() ? first + 1 : first;
        const auto inWindow = reading && last >= org && first < org + len;
        if (inWindow && last >= org + len - 1)
            fetchedAll = true;
        // Program reads outside the window are prefetch: NOPs. Data reads
        // there, an RTE's frame, come from memory.
        if (inWindow || (reading && s->program())) {
            s->inject(instData(s, inst, len, org));
        } else if (memWrite(s)) {
            s->capture();
        }
        completeCycle(s);
        if (memWrite(s)) {
            const auto put = [&](uint32_t a, uint8_t v) {
                if (cap < max) {
                    if (buf)
                        buf[cap] = v;
                    if (addrs)
                        addrs[cap] = wrap(a);
                    ++cap;
                }
            };
            if (s->wordAccess()) {
                put(s->addr & ~1, hi(s->data));
                put((s->addr & ~1) + 1, lo(s->data));
            } else {
                put(s->addr, s->data);
            }
        }
        s = prepareCycle();
    }
    // The CPU is parked in |s|: the caller resumes from its address, which
    // is |leaves| unless the guard ran out.
    _cutShort = !exited;
    org = s->addr;
    return cap;
}

void PinsMc68000Base::execInst(
        const uint8_t *inst, uint_fast8_t len, uint32_t &org, uint32_t exit) {
    execute(inst, len, nullptr, nullptr, 0, org, exit);
}

uint_fast8_t PinsMc68000Base::captureWrites(const uint8_t *inst, uint_fast8_t len,
        uint8_t *buf, uint32_t *addrs, uint_fast8_t max, uint32_t &org,
        uint32_t exit) {
    return execute(inst, len, buf, addrs, max, org, exit);
}

bool PinsMc68000Base::answerVector(uint32_t vecAddr, uint32_t handler) {
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
// then PC (M68000UM Figure 6-5), as the writes left them in memory. An
// interrupt's acknowledge comes among them (6.3.2), and no other read: the
// first one back ends the frame, so a push of the program's own, or the
// frame of a TRAP stepped before a trace, is never taken for it. |from|
// becomes the first of them.
bool PinsMc68000Base::findFrame(const Signals *s, RegsMc68000::Frame &frame,
        const Signals *&from) const {
    const auto size = regs<RegsMc68000>()->frameBytes();
    uint_fast8_t bytes = 0;
    uint32_t low = UINT32_MAX;
    for (auto i = 1; i <= frame_lookback && bytes < size; ++i) {
        const auto t = s->prev(i);
        if (t->iack())
            continue;
        if (!memWrite(t))
            break;
        bytes += t->wordAccess() ? 2 : 1;
        const auto a = t->wordAccess() ? t->addr & ~UINT32_C(1) : t->addr;
        if (a < low)
            low = a;
        from = t;
    }
    if (bytes < size)
        return false;
    const auto mems = this->mems<MemsMc68000>();
    frame.sr = mems->read16(low);
    frame.pc = wrap(mems->read32(low + 2));
    frame.ssp = wrap(low + size);
    return true;
}

void PinsMc68000Base::assertInt(uint8_t) {
    _intr = true;
    setIpl();
}

void PinsMc68000Base::negateInt(uint8_t) {
    _intr = false;
    setIpl();
}

// Stop at an instruction boundary: level 7 is taken there, on its change
// from a lower level, and the CPU parks in its vector read with PC and SR
// stacked. With |s| null the CPU has no bus cycle, stopped in STOP, and
// level 7 wakes it.
bool PinsMc68000Base::suspend(Signals *s) {
    _halting = true;
    setIpl();
    if (s == nullptr) {
        s = prepareCycle();
    } else {
        completeCycle(s);
        s = prepareCycle();
    }
    const auto vecAddr =
            regs<RegsMc68000>()->vecAddr(InstMc68000::VECNUM_HALT);
    auto guard = irq_ack_cycles;
    while (guard-- && !s->halt()) {
        if (s->iack() && s->level() == 7) {
            s->inject(InstMc68000::VECNUM_HALT);
            // Back to the device level: the next halt is a new change to 7.
            _halting = false;
            setIpl();
        }
        if (memRead(s) && s->dataSpace() && (s->addr & ~1) == vecAddr) {
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
bool PinsMc68000Base::step(bool show) {
    Cycles::reset();
    regs<RegsMc68000>()->restore(true);
    if (_cutShort) {
        cli.println("?step: restore cut short");
        return false;
    }
    if (show)
        Cycles::reset();
    const auto vecAddr =
            regs<RegsMc68000>()->vecAddr(InstMc68000::VECNUM_TRACE);
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
                (s->addr & ~1) == vecAddr) {
            RegsMc68000::Frame frame;
            const Signals *from = nullptr;
            if (findFrame(s, frame, from)) {
                frame.sr &= ~InstMc68000::SR_T;
                regs<RegsMc68000>()->parkInVector(vecAddr, frame);
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
bool PinsMc68000Base::loop() {
    const auto vecAddr =
            regs<RegsMc68000>()->vecAddr(InstMc68000::VECNUM_ILLEGAL);
    auto s = resumeCycle();
    while (true) {
        // First: a slot that timed out holds the status and address of
        // its use a ring ago.
        if (s->halt()) {
            // STOP: no bus cycle until an interrupt.
            if (_devs->loop() && haltSwitch())
                return suspend(nullptr);
            s = prepareCycle(wait_poll_clocks);
            continue;
        }
        if (memRead(s) && s->dataSpace() &&
                (s->addr & ~1) == vecAddr) {
            RegsMc68000::Frame frame;
            const Signals *from = nullptr;
            if (findFrame(s, frame, from)) {
                // ILLEGAL stacks its own address.
                regs<RegsMc68000>()->parkInVector(vecAddr, frame);
                Cycles::discard(from);
                return true;
            }
        }
        completeCycle(s);
        const auto serviced = _devs->loop();
        s = prepareCycle();
        // Only now, with a cycle recorded: suspend() completes it first.
        // Not at an acknowledge: the program's own interrupt is under way.
        if (!serviced || s->iack())
            continue;
        if (haltSwitch())
            return suspend(s->halt() ? nullptr : s);
    }
}

void PinsMc68000Base::run() {
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

void PinsMc68000Base::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr & ~1, InstMc68000::ILLEGAL);
}

void PinsMc68000Base::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

// Sort the ring's cycles with FetchMc68000; |anchored| when the ring was
// trimmed to start at an instruction.
void PinsMc68000Base::classifyCycles(
        FetchMc68000::Kind *kind, bool anchored) const {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    FetchMc68000::Cycle c[Cycles::MAX_CYCLES];
    for (auto i = 0u; i < cycles; ++i) {
        const auto s = g->next(i);
        c[i] = {s->addr, uint8_t(s->wordAccess() ? 2 : 1),
                memRead(s) && s->program()};
    }
    FetchMc68000::classify(c, cycles, anchored, kind);
}

// The latest instruction start that leaves at least the limit's count of
// fetches after it.
const SignalsImpl *PinsMc68000Base::findBacktraceStart() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    FetchMc68000::Kind kind[Cycles::MAX_CYCLES];
    classifyCycles(kind, false);
    uint32_t fetches = 0;
    auto start = 0u;
    _anchored = false;
    for (auto i = cycles; i-- > 0;) {
        if (kind[i] == FetchMc68000::FETCH || kind[i] == FetchMc68000::FLUSH)
            ++fetches;
        if (kind[i] == FetchMc68000::FLUSH) {
            start = i;
            _anchored = true;
            if (fetches >= _lineLimit)
                break;
        }
    }
    return g->next(start);
}

// No pin marks an instruction start, so one follows a flush; the ones
// after it follow by length, each shown as its first byte is fetched. A
// flush back to the instruction shown last is its refetch after a write
// to SR. With no flush in the ring, the stop PC anchors the fetches
// instead: the first even address whose lengths lead to it. Fetches left
// before any start show as raw cycles.
// ATTENTION: up to 4 bytes the prefetch read before a flush show as
// instructions though they never ran.
void PinsMc68000Base::printBacktrace() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    FetchMc68000::Kind kind[Cycles::MAX_CYCLES];
    classifyCycles(kind, _anchored);
    _anchored = false;
    auto anchor = anchorAt(kind, cycles);
    uint32_t next = 0;  // the next instruction's start
    uint32_t last = UINT32_MAX;  // the start of the instruction shown last
    for (auto i = 0u; i < cycles; ++i) {
        const auto s = g->next(i);
        auto k = kind[i];
        if (k == FetchMc68000::LOOSE && anchor != UINT32_MAX) {
            if (s->addr == anchor) {
                k = FetchMc68000::FLUSH;
                anchor = UINT32_MAX;
            }
        } else if (k == FetchMc68000::LOOSE && last != UINT32_MAX) {
            k = FetchMc68000::FETCH;  // after the anchor
        }
        if (k == FetchMc68000::FLUSH && s->addr != last)
            next = s->addr;
        if (k == FetchMc68000::FLUSH || k == FetchMc68000::FETCH) {
            const auto fetched = s->addr + (s->wordAccess() ? 2 : 1);
            while (next < fetched) {
                last = next;
                next = _mems->disassemble(next, 1);
            }
            if (!Debugger.verbose())
                continue;
        }
        s->print();
        idle();
    }
}

// Where the ring's loose fetches start an instruction, found from the stop
// PC: none if the ring has a flush, or no start leads to the PC.
uint32_t PinsMc68000Base::anchorAt(
        const FetchMc68000::Kind *kind, uint_fast8_t cycles) const {
#ifdef WITH_DISASSEMBLER
    const auto g = Signals::get();
    uint32_t first = UINT32_MAX;
    for (auto i = 0u; i < cycles; ++i) {
        if (kind[i] == FetchMc68000::FLUSH)
            return UINT32_MAX;
        if (kind[i] == FetchMc68000::LOOSE && first == UINT32_MAX)
            first = g->next(i)->addr;
    }
    if (first == UINT32_MAX)
        return UINT32_MAX;
    const auto pc = _regs->nextIp();
    const auto mems = this->mems<MemsMc68000>();
    for (auto start = (first + 1) & ~1; start <= pc; start += 2) {
        auto addr = start;
        while (addr < pc)
            addr += mems->insnLength(addr);
        if (addr == pc)
            return start;
    }
#endif
    return UINT32_MAX;
}

}  // namespace mc68000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
