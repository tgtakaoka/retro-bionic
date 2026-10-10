#include "pins_mc68hc16.h"
#include "debugger.h"
#include "devs_mc68hc16.h"
#include "digital_bus.h"
#include "inst_mc68hc16.h"
#include "mems_mc68hc16.h"
#include "pipe_mc68hc16.h"
#include "regs_mc68hc16.h"
#include "signals_mc68hc16.h"

namespace debugger {
namespace mc68hc16 {

using mc6800::DevsMc6800;

// clang-format off
/**
 * MC68HC16Z1 bus cycle (User's Manual 5.6.2, Table A-15). MODCLK is tied
 * low, so EXTAL is the system clock and the CPU is static. A cycle is six
 * states, S0 opening at a rising edge:
 *
 *        | S0 | S1 | S2 | S3 | S4 | S5 |
 *         ____      ____      ____
 * EXTAL  |    |____|    |____|    |____|
 *        _ ______________________________ _
 * ADDR   _X______________________________X_
 *        ______                       ____
 * #AS          |_____________________|
 *                        ^ #DSACK sampled    v read data latched
 *
 * Address, FC2:FC0 and SIZ1:SIZ0 are valid in S0; #AS, and #DS on a read,
 * assert in S1. #DSACK is sampled at the S2/S3 falling edge: until it is,
 * wait states repeat S2 and S3 (the bus monitor is off at reset). A write
 * drives data in S2 and asserts #DS in S3. Read data is latched at the
 * S4/S5 falling edge and the strobes negate after it. So with EXTAL low
 * the CPU sits in S1, S3, S5 or a wait state, and a cycle is held open by
 * withholding #DSACK1. #AVEC is tied low: an interrupt acknowledge (FC
 * 111) ends on its own and wants neither data nor #DSACK.
 */
// clang-format on

namespace {

constexpr auto ADDR_MASK = MemsMc68hc16::ADDR_MASK;

// ATTENTION: unmeasured. The high phase and the settling after each
// falling edge, for strobes that follow it by up to 29ns plus the
// EXTAL-to-CLKOUT skew.
constexpr auto extal_hi_ns = 50;
constexpr auto clk_delay_ns = 50;
constexpr auto reset_hi_ns = 200;
constexpr auto reset_lo_ns = 200;
// ATTENTION: unmeasured. #RESET held this long, more than the 20 clocks
// the data bus mode select needs set up; the MCU then holds it 512 more.
constexpr auto reset_cycles = 64;
constexpr auto reset_release_clocks = 2048;
// No #AS for this many clocks: the CPU stopped in WAI or LPSTOP. Long
// enough to clear the slowest instruction with no bus cycle.
constexpr auto halt_clocks = 1024;
// Bus cycles allowed for an #IRQ7 acknowledge; generous.
constexpr auto irq_ack_cycles = 1024;
// Cycles before an exception's vector read that may hold its frame: two
// stack words, the acknowledge and prefetches between.
constexpr auto frame_lookback = 10;

// Data bus mode select at reset (User's Manual Table 5-19): DATA1, DATA2
// and DATA7 low for BR/BG/BGACK, FC2:FC0 and ADDR23:ADDR19; DATA0 (16-bit
// CSBOOT), DATA8 (bus control pins), DATA9 (IRQ pins and MODCLK) and
// DATA11 (normal operation) high, as is the rest.
constexpr uint16_t DATA_STRAPS = 0xFF79;

const uint8_t PINS_LOW[] = {
        PIN_EXTAL,
        PIN_RESET,
        PIN_ASEL0,
        PIN_ASEL1,
};

const uint8_t PINS_HIGH[] = {
        PIN_DSACK0,
        PIN_DSACK1,
        PIN_IRQ7,
        PIN_IRQ1,
};

const uint8_t PINS_INPUT[] = {
        PIN_AS,
        PIN_RW,
        PIN_IPIPE0,
        PIN_IPIPE1,
        PIN_DS,
        PIN_CLKOUT,
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
        PIN_AL0,
        PIN_AL1,
        PIN_AL2,
        PIN_AL3,
        PIN_AH0,
        PIN_AH1,
        PIN_AH2,
        PIN_AH3,
};

inline void extal_lo() {
    digitalWriteFast(PIN_EXTAL, LOW);
}

inline void extal_hi() {
    digitalWriteFast(PIN_EXTAL, HIGH);
}

// One clock, high half first, then wait for what its fall moves.
inline void extal_cycle() {
    extal_hi();
    delayNanoseconds(extal_hi_ns);
    extal_lo();
    delayNanoseconds(clk_delay_ns);
}

void extal_cycle_reset() {
    extal_hi();
    delayNanoseconds(reset_hi_ns);
    extal_lo();
    delayNanoseconds(reset_lo_ns);
}

// #RESET is open drain both ways: drive it low or let the pull-up have
// it, never drive it high.
void assert_reset() {
    pinMode(PIN_RESET, OUTPUT);
    digitalWriteFast(PIN_RESET, LOW);
}

void release_reset() {
    pinMode(PIN_RESET, INPUT_PULLUP);
}

// #DSACK1 alone: a 16-bit port. #DSACK0 stays negated.
inline void assert_dsack1() {
    digitalWriteFast(PIN_DSACK1, LOW);
}

inline void negate_dsack1() {
    digitalWriteFast(PIN_DSACK1, HIGH);
}

// #IRQ7 is edge-sensitive and wants to stay low until acknowledged.
void assert_irq7() {
    digitalWriteFast(PIN_IRQ7, LOW);
}

void negate_irq7() {
    digitalWriteFast(PIN_IRQ7, HIGH);
}

inline uint16_t bothLanes(uint8_t v) {
    return static_cast<uint16_t>(v) << 8 | v;
}

inline bool memRead(const Signals *s) {
    return s->read() && !s->iack();
}

inline bool memWrite(const Signals *s) {
    return s->write() && !s->iack();
}

// The byte a byte cycle moves: the even address on D8-D15.
inline uint8_t lane(const Signals *s) {
    return (s->addr & 1) ? s->data : s->data >> 8;
}

uint32_t wrap(uint32_t addr) {
    return addr & ADDR_MASK;
}

}  // namespace

inline bool Signals::getControl() {
    cntl() = busRead(CNTL);
    return strobe();
}

inline void Signals::getPhase2() {
    cntl2() = busRead(CNTL);
}

PinsMc68hc16::PinsMc68hc16() {
    _devs = new DevsMc6800(ACIA_BASE_HC16);
    // Memory first: RegsMc68hc16 stages its restore frame there.
    _mems = new MemsMc68hc16();
    _regs = new RegsMc68hc16(this, mems<MemsMc68hc16>());
}

void PinsMc68hc16::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    Cycles::reset();
    assert_reset();
    busWrite(D, DATA_STRAPS);
    busMode(D, OUTPUT);
    for (auto i = 0; i < reset_cycles; i++)
        extal_cycle_reset();
    // The MCU stretches #RESET by 512 clocks after it is released, and
    // samples the data bus as it rises.
    release_reset();
    for (auto n = 0; digitalReadFast(PIN_RESET) == LOW; ++n) {
        if (n >= reset_release_clocks) {
            busMode(D, INPUT);
            cli.println("?reset: #RESET stuck low");
            return;
        }
        extal_cycle_reset();
    }
    // ATTENTION: the first cycle comes ten clocks after #RESET rises, and
    // the mode select must be off the bus by then.
    extal_cycle();
    busMode(D, INPUT);
    // With MODCLK low CLKOUT follows EXTAL.
    extal_hi();
    delayNanoseconds(extal_hi_ns);
    const auto clkHigh = digitalReadFast(PIN_CLKOUT) != LOW;
    extal_lo();
    delayNanoseconds(clk_delay_ns);
    const auto clkLow = digitalReadFast(PIN_CLKOUT) == LOW;
    if (!clkHigh || !clkLow)
        cli.println("?reset: CLKOUT does not follow EXTAL");

    // From here to setupBus() no clock may pass idle: CSBOOT covers
    // 00000H-7FFFFH from reset and ends a cycle by itself after 13 waits,
    // so a cycle parked there would complete with whatever the bus holds.
    const auto s = prepareCycle();
    // Anything but a read at 00000H means the board, most likely the
    // address mux, is not what the driver assumes.
    if (s->halt() || !memRead(s) || s->addr != InstMc68hc16::VEC_RESET) {
        cli.print("?reset: first cycle ");
        s->print();
        return;
    }
    // The reset vector, answered: bank 0, the park origin, SP and IZ 0.
    static constexpr uint8_t VECTOR[] = {
            0x00,
            0x00,  // ZK:SK:PK
            uint8_t(InstMc68hc16::ORG_PARK >> 8),
            uint8_t(InstMc68hc16::ORG_PARK),  // PC
            0x00,
            0x00,  // SP
            0x00,
            0x00,  // IZ
    };
    uint32_t org = InstMc68hc16::VEC_RESET;
    execInst(VECTOR, sizeof(VECTOR), org, InstMc68hc16::ORG_PARK);
    if (_cutShort) {
        cli.println("?reset: no fetch from the reset vector");
        printCycles();
        return;
    }
    _regs->setIp(org);
    setupBus();
    if (_cutShort) {
        cli.println("?reset: setup sequence cut short");
        printCycles();
        return;
    }
    _regs->save();
    _regs->reset();
}

// Reset leaves the software watchdog on and CSBOOT terminating every
// cycle below 80000H by itself. SYPCR is write-once: clearing it stops
// the watchdog and keeps the bus and halt monitors off, so a program's
// own SYPCR write is ignored. CSORBT cleared disables CSBOOT, so #DSACK1
// alone ends a cycle. On-chip registers show nothing on the bus.
void PinsMc68hc16::setupBus() {
    // clang-format off
    static constexpr uint8_t SEQ[] = {
        0xF5, 0x0F,              // LDAB #$0F
        0x27, 0xFA,              // TBEK            ; registers at $FF000
        0x17, 0x35, 0xFA, 0x21,  // CLR  $FA21      ; SYPCR
        0x27, 0x35, 0xFA, 0x4A,  // CLRW $FA4A      ; CSORBT
        0xB0, 0xEE,              // BRA  org
    };
    // clang-format on
    static_assert(SEQ[sizeof(SEQ) - 1] == uint8_t(-(sizeof(SEQ) + 4)),
            "BRA org: the displacement is from the BRA plus 6");
    auto org = _regs->nextIp();
    execInst(SEQ, sizeof(SEQ), org, EXIT_ORG);
    _regs->setIp(org);
}

void PinsMc68hc16::idle() {
    // The CPU is parked, so this only keeps the clock alive.
    extal_cycle();
}

// Clock up to S1 of the next cycle, or to the conclusion that the CPU
// stopped.
Signals *PinsMc68hc16::prepareCycle() {
    auto s = Signals::put();
    s->clearMark();  // Cycles::next() keeps the slot's old mark
    for (auto n = 0; !s->getControl(); ++n) {
        if (n >= halt_clocks) {
            s->markHalt();
            return s;
        }
        extal_cycle();
    }
    // The CPU moves only on our edges, so the address and status hold
    // while the muxes are stepped through.
    s->getAddr();
    s->addr = wrap(s->addr);
    return s;
}

// Let the parked CPU go. The address is the caller's to give: the ring is
// a log that reset and discard rewrite. The parked cycle still drives its
// status, so that is read again.
Signals *PinsMc68hc16::resumeCycle(uint32_t addr) {
    auto s = Signals::put();
    s->clearMark();
    s->getControl();
    s->getAddr();
    s->addr = wrap(addr);
    return s;
}

Signals *PinsMc68hc16::completeCycle(Signals *s) {
    if (s->halt()) {
        // Nothing moves; not recorded, so s->prev() stays the previous
        // real cycle.
        s->inputMode();
        return s;
    }

    const auto maddr = wrap(s->addr);
    const auto device =
            _devs->isSelected(maddr & ~1) || _devs->isSelected(maddr);
    if (memRead(s)) {
        if (s->readMemory()) {
            if (device) {
                s->data = s->wordAccess() ? uint16(_devs->read(maddr),
                                                    _devs->read(maddr + 1))
                                          : bothLanes(_devs->read(maddr));
            } else if (s->wordAccess()) {
                s->data = mems<MemsMc68hc16>()->read_bus(maddr);
            } else {
                // Drive both halves; the CPU takes its lane.
                s->data = bothLanes(_mems->read_byte(maddr));
            }
        }
        s->outData();
    }
    if (!s->iack())
        assert_dsack1();
    // S2 and S3: #DSACK1 sampled at the fall, write data driven by then.
    extal_cycle();
    if (memWrite(s)) {
        s->getData();
        if (s->writeMemory()) {
            if (device) {
                if (s->wordAccess()) {
                    _devs->write(maddr, hi(s->data));
                    _devs->write(maddr + 1, lo(s->data));
                } else {
                    _devs->write(maddr, lane(s));
                }
            } else if (s->wordAccess()) {
                mems<MemsMc68hc16>()->write_bus(maddr, s->data);
            } else {
                _mems->write_byte(maddr, lane(s));
            }
        }
    }
    // S4, where the second IPIPE phase is read, then S5: read data is
    // latched at the fall. ATTENTION: the phase 2 sample point is
    // unverified.
    extal_hi();
    delayNanoseconds(extal_hi_ns);
    s->getPhase2();
    extal_lo();
    delayNanoseconds(clk_delay_ns);
    // Hold until #AS negates: an acknowledge #AVEC ends takes its own time.
    // Probed apart, so |s| keeps its strobes.
    Signals probe;
    auto guard = halt_clocks;
    while (probe.getControl() && guard--)
        extal_cycle();

    negate_dsack1();
    s->inputMode();
    Cycles::next();
    return s;
}

namespace {
// The word a read at |addr| moves from |inst| placed at |org|: the even
// byte on D8-D15. Outside the window it is a NOP, so a prefetch past the
// end runs nothing.
uint16_t busWord(
        const uint8_t *inst, uint_fast8_t len, uint32_t org, uint32_t addr) {
    const auto at = [&](uint32_t a) -> uint8_t {
        const auto off = a - org;
        if (a >= org && off < len)
            return inst[off];
        return (a & 1) ? InstMc68hc16::NOP & 0xFF : InstMc68hc16::NOP >> 8;
    };
    const auto even = addr & ~UINT32_C(1);
    return static_cast<uint16_t>(at(even)) << 8 | at(even + 1);
}
}  // namespace

uint_fast8_t PinsMc68hc16::execute(const uint8_t *inst, uint_fast8_t len,
        uint8_t *buf, uint32_t *addrs, uint_fast8_t max, uint32_t &org,
        uint32_t exit) {
    uint_fast8_t cap = 0;
    // |inst| is answered by address. |exit| is where the sequence ends --
    // a prefetch past the window looks just like falling through, so the
    // caller must say.
    const uint32_t leaves = exit == EXIT_ORG ? org : wrap(exit);
    bool started = false;
    // The exit may lie inside the window: it counts only once the last
    // word has been fetched.
    bool fetchedAll = false;
    bool exited = false;
    auto s = resumeCycle(org);
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
        // there, a PULM or an RTI, come from memory.
        if (inWindow || (reading && s->program())) {
            const auto word = busWord(inst, len, org, s->addr);
            if (s->wordAccess()) {
                s->inject(word);
            } else {
                s->inject(bothLanes((s->addr & 1) ? lo(word) : hi(word)));
            }
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
                put(s->addr, hi(s->data));
                put(s->addr + 1, lo(s->data));
            } else {
                put(s->addr, lane(s));
            }
        }
        s = prepareCycle();
    }
    // Parked in it: the caller resumes from where the bus showed it, which
    // is |leaves| unless the guard ran out.
    _cutShort = !exited;
    org = s->addr;
    return cap;
}

void PinsMc68hc16::execInst(
        const uint8_t *inst, uint_fast8_t len, uint32_t &org, uint32_t exit) {
    execute(inst, len, nullptr, nullptr, 0, org, exit);
}

uint_fast8_t PinsMc68hc16::captureWrites(const uint8_t *inst, uint_fast8_t len,
        uint8_t *buf, uint32_t *addrs, uint_fast8_t max, uint32_t &org,
        uint32_t exit) {
    return execute(inst, len, buf, addrs, max, org, exit);
}

bool PinsMc68hc16::answerVector(uint32_t vecAddr, uint32_t handler) {
    const uint8_t vector[] = {uint8_t(handler >> 8), uint8_t(handler)};
    auto org = vecAddr;
    execute(vector, sizeof(vector), nullptr, nullptr, 0, org, handler);
    return !_cutShort;
}

namespace {
// The frame an exception stacked shortly before its vector read |s|: CCR
// below PC (CPU16RM Figure 9-1), as the writes left them in memory. Two
// word writes, or four byte writes from an odd SP. |from| becomes the
// first of them.
bool findFrame(const Signals *s, const MemsMc68hc16 *mems,
        RegsMc68hc16::Frame &frame, const Signals *&from) {
    uint_fast8_t bytes = 0;
    uint32_t low = UINT32_MAX;
    for (auto i = 1; i <= frame_lookback && bytes < 4; ++i) {
        const auto t = s->prev(i);
        if (!memWrite(t))
            continue;
        const auto a = t->wordAccess() ? t->addr & ~UINT32_C(1) : t->addr;
        bytes += t->wordAccess() ? 2 : 1;
        if (a < low)
            low = a;
        from = t;
    }
    if (bytes < 4)
        return false;
    frame.ccr = mems->read16(low);
    const uint32_t pc = mems->read16(wrap(low + 2));
    frame.pc = static_cast<uint32_t>(frame.ccr & 0xF) << 16 | pc;
    frame.sp = wrap(low + 2);
    return true;
}
}  // namespace

// Stop at an instruction boundary: #IRQ7 is taken there and the CPU parks
// in its vector read with PC and CCR stacked. #IRQ7 is asserted after
// |holdOff| bus cycles. With |org| UINT32_MAX the CPU has no parked cycle,
// stopped in WAI or LPSTOP, and #IRQ7 wakes it.
bool PinsMc68hc16::suspend(uint32_t org, uint_fast8_t holdOff) {
    Signals *s;
    if (org == UINT32_MAX) {
        assert_irq7();
        s = prepareCycle();
    } else {
        s = resumeCycle(org);
        if (holdOff == 0)
            assert_irq7();
        completeCycle(s);
        s = prepareCycle();
        for (uint_fast8_t n = 1; n < holdOff && !s->halt(); ++n) {
            completeCycle(s);
            s = prepareCycle();
        }
        assert_irq7();
    }
    // Bound the wait; loop() polls the halt switch only between steps.
    auto guard = irq_ack_cycles;
    while (guard-- && !s->halt()) {
        if (s->iack())
            negate_irq7();
        if (memRead(s) && s->dataSpace() && s->addr == InstMc68hc16::VEC_IRQ7) {
            RegsMc68hc16::Frame frame;
            const Signals *from = nullptr;
            if (findFrame(s, mems<MemsMc68hc16>(), frame, from)) {
                negate_irq7();
                // The frame and this read are the debugger's.
                Cycles::discard(from);
                // An asynchronous exception stacks the next PC plus 6.
                frame.pc = wrap(frame.pc - InstMc68hc16::IRQ_PC_OFFSET);
                regs<RegsMc68hc16>()->parkInVector(
                        RegsMc68hc16::IRQ7, s->addr, frame);
                return true;
            }
        }
        completeCycle(s);
        s = prepareCycle();
    }
    negate_irq7();
    cli.println(s->halt() ? "?halt: no IRQ7 acknowledge (stack on chip?)"
                          : "?halt: no IRQ7 acknowledge");
    return false;
}

bool PinsMc68hc16::step(bool show) {
    // #IRQ7 asserted as the CPU resumes may be taken before the first
    // instruction runs: if nothing changed, assert it later.
    // ATTENTION: where one instruction ends is unverified on the bench.
    const auto regs = this->regs<RegsMc68hc16>();
    const auto before = regs->fingerprint();
    for (uint_fast8_t holdOff = 0; holdOff < 3; ++holdOff) {
        Cycles::reset();
        _regs->restore();
        if (show)
            Cycles::reset();
        if (!suspend(regs->parkedAt(), holdOff))
            return false;
        Cycles::Hold hold;  // keep the step's cycles for printCycles()
        _regs->save();
        if (regs->fingerprint() != before)
            break;
    }
    if (show)
        printCycles();
    return true;
}

// A read of the SWI vector reports a break: a patched breakpoint, or the
// halt convention of an SWI vector pointing at itself. From |from| on the
// cycles are the debugger's.
bool PinsMc68hc16::isSwiBreak(Signals *s, const Signals *&from) {
    RegsMc68hc16::Frame frame;
    if (!findFrame(s, mems<MemsMc68hc16>(), frame, from))
        return false;
    // SWI stacks its own address plus 8.
    const auto bp = wrap(frame.pc - InstMc68hc16::SWI_PC_OFFSET);
    if (_mems->read16(bp) != InstMc68hc16::SWI)
        return false;
    if (!isBreakPoint(bp) &&
            _mems->read16(InstMc68hc16::VEC_SWI) != InstMc68hc16::VEC_SWI)
        return false;
    frame.pc = bp;
    regs<RegsMc68hc16>()->parkInVector(RegsMc68hc16::SWI, s->addr, frame);
    return true;
}

// Free-run. Returns whether the CPU stopped at a boundary its registers can
// be saved from. The ring is left holding the program's cycles only.
bool PinsMc68hc16::loop() {
    auto s = resumeCycle(regs<RegsMc68hc16>()->parkedAt());
    while (true) {
        const Signals *from;
        if (memRead(s) && s->dataSpace() && s->addr == InstMc68hc16::VEC_SWI &&
                isSwiBreak(s, from)) {
            Cycles::discard(from);
            return true;
        }
        if (s->halt()) {
            // WAI or LPSTOP: no bus cycle until an interrupt.
            _devs->loop();
            if (haltSwitch())
                return suspend(UINT32_MAX);
            s = prepareCycle();
            continue;
        }
        completeCycle(s);
        _devs->loop();
        s = prepareCycle();
        // Only now, with a cycle recorded: suspend() opens with
        // resumeCycle(), which reads that slot back.
        if (haltSwitch())
            return s->halt() ? suspend(UINT32_MAX) : suspend(s->addr);
    }
}

void PinsMc68hc16::run() {
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

void PinsMc68hc16::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr & ~1, InstMc68hc16::SWI);
}

void PinsMc68hc16::assertInt(uint8_t) {
    digitalWriteFast(PIN_IRQ1, LOW);
}

void PinsMc68hc16::negateInt(uint8_t) {
    digitalWriteFast(PIN_IRQ1, HIGH);
}

void PinsMc68hc16::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

// Mark each cycle an instruction started in with how far back its opcode
// was fetched, from IPIPE as the ring holds it.
void PinsMc68hc16::markStarts() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    PipeMc68hc16::Cycle pipe[Cycles::MAX_CYCLES];
    int16_t starts[Cycles::MAX_CYCLES];
    for (auto i = 0u; i < cycles; ++i) {
        const auto s = g->next(i);
        pipe[i] = {s->phase1(), s->phase2(), memRead(s) && s->program(),
                s->iack()};
    }
    PipeMc68hc16::track(pipe, cycles, starts);
    for (auto i = 0u; i < cycles; ++i) {
        const auto s = g->next(i);
        if (starts[i] == PipeMc68hc16::NOT_START) {
            s->clearMark();
        } else {
            s->markStart(starts[i] > UINT8_MAX ? 0 : starts[i]);
        }
    }
}

const SignalsImpl *PinsMc68hc16::findBacktraceStart() {
    markStarts();
    return backtraceStartFrom<Signals>(Signals::get(), _lineLimit);
}

// One line per instruction started, then its data transfers; fetches only
// when verbose. With no START in the ring, every cycle.
void PinsMc68hc16::printBacktrace() {
    markStarts();
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    auto starts = 0u;
    for (auto i = 0u; i < cycles; ++i) {
        if (g->next(i)->fetch())
            ++starts;
    }
    for (auto i = 0u; i < cycles; ++i) {
        const auto s = g->next(i);
        if (starts && s->fetch()) {
            const auto back = s->back();
            if (back && back <= i) {
                _mems->disassemble(g->next(i - back)->addr, 1);
            } else {
                s->print();
                idle();
                continue;
            }
        }
        if (starts == 0 || !memRead(s) || !s->program() || Debugger.verbose())
            s->print();
        idle();
    }
}

}  // namespace mc68hc16
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
