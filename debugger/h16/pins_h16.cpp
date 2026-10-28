#include "pins_h16.h"
#include "debugger.h"
#include "devs_h16.h"
#include "digital_bus.h"
#include "inst_h16.h"
#include "mems_h16.h"
#include "regs_h16.h"
#include "signals_h16.h"

namespace debugger {
namespace h16 {

// clang-format off
/**
 * HD641016 bus cycle (User's Manual 4.9.2, Figures 4-88, 4-89 and 12-7).
 * The system clock PHI is EXTAL/2, one state a PHI period:
 *
 *               T1      T2      Tw      T3      T1
 *            ___     ___     ___     ___     ___
 *   PHI   __/   \___/   \___/   \___/   \___/   \__
 *         ______                          ________
 *    #AS        \________________________/
 *   A1/D0 --<addr >-----< read data >--------<addr
 *         ___________                    _________
 *  #HDS/#LDS         \__________________/
 *                         ^       ^      ^
 *                         #WAIT sampled  read data latched
 *
 * #AS asserts at the T1 fall, and A1-A16 hold until the T2 rise; the
 * data strobes assert after that rise, but the CPU lets go of A1/D0-
 * A16/D15 only by the T2 fall (Figure 17-4: tDSD1, tADZ1, tADZ2). #WAIT,
 * sampled at the fall of T2 and of every Tw, inserts Tw states while it
 * is low; the CPU then holds its outputs. Read data is latched at the
 * T3 fall, and the strobes negate after it.
 *
 * EXTAL runs free from FlexPWM3: the chip is dynamic, PHI no slower than
 * 1000ns. #WAIT stays asserted, so every cycle waits in Tw until it is
 * served, and is negated only until #AS negates at the end of T3.
 */
// clang-format on

namespace {

constexpr auto ADDR_MASK = MemsH16::ADDR_MASK;

// ATTENTION: unbenched. The EXTAL rate a run, and everything else, uses:
// PHI 2 MHz. The HD641016CP10 takes EXTAL up to 20 MHz, and no chip
// takes it below 2 MHz (tcyc 1000ns max).
constexpr uint32_t clock_input_hz = 4000000;
constexpr uint32_t clock_input_min_hz = 2000000;
constexpr uint32_t clock_input_max_hz = 20000000;
// EXTAL's high and low times at least this: half the 50ns EXTAL period
// that the 100ns tcyc minimum makes.
constexpr auto extal_half_min_ns = 25;
// #RES and #BRTRY low together this long resets: at least 12 PHI clocks,
// and 50ms at system power-on (User's Manual 4.11), which every reset
// takes rather than tell the first apart.
constexpr auto reset_us = 50000;
// #RES rises within this many EXTAL clocks of its release: only the
// pull-up drives it.
constexpr auto reset_release_clocks = 64;
// No #AS for this many EXTAL clocks: the CPU sleeps or halted. Long
// enough to clear the slowest instruction with the prefetch queue full.
constexpr auto halt_clocks = 4096;
// EXTAL clocks between polls of a CPU already stopped: short enough to
// service the devices and the halt switch often.
constexpr auto wait_poll_clocks = 64;
// Bus cycles between polls of the halt switch in a run.
constexpr auto halt_poll_cycles = 16;
// EXTAL clocks a cycle takes, at most, to show its data strobes, and to
// end once #WAIT is negated: T2, a Tw, T3 and slack.
constexpr auto strobe_clocks = 16;
// Bus cycles allowed for an #NMI to be taken; generous.
constexpr auto nmi_ack_cycles = 1024;
// Cycles before an exception's vector read that may hold its frame: the
// PC and SR, three words or six bytes, and prefetches between.
constexpr auto frame_lookback = 16;

const uint8_t PINS_LOW[] = {
        PIN_WAIT,
        PIN_BRTRY,
};

const uint8_t PINS_HIGH[] = {
        PIN_NMI,
        PIN_IRQ0,
};

const uint8_t PINS_INPUT[] = {
        PIN_AS,
        PIN_RW,
        PIN_HDS,
        PIN_LDS,
        PIN_PHI,
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
        PIN_AH17,
        PIN_AH18,
        PIN_AH19,
        PIN_AH20,
        PIN_AH21,
        PIN_AH22,
        PIN_AH23,
        PIN_PF,
        PIN_ST0,
        PIN_ST1,
        PIN_ST2,
};

// #RES is open drain both ways: the RESET instruction drives it low for
// 256 clocks. Drive it low or let the pull-up have it.
void assert_reset() {
    pinMode(PIN_RES, OUTPUT);
    digitalWriteFast(PIN_RES, LOW);
}

void release_reset() {
    pinMode(PIN_RES, INPUT_PULLUP);
}

inline void assert_wait() {
    digitalWriteFast(PIN_WAIT, LOW);
}

inline void negate_wait() {
    digitalWriteFast(PIN_WAIT, HIGH);
}

// #NMI is always falling-edge triggered (User's Manual Figure 4-68); it
// is held low until its vector read.
void assert_nmi() {
    digitalWriteFast(PIN_NMI, LOW);
}

void negate_nmi() {
    digitalWriteFast(PIN_NMI, HIGH);
}

inline uint16_t bothLanes(uint8_t v) {
    return static_cast<uint16_t>(v) << 8 | v;
}

inline bool memRead(const Signals *s) {
    return s->memory() && s->read();
}

inline bool memWrite(const Signals *s) {
    return s->memory() && s->write();
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
    return (cntl() & AS) == 0;
}

inline bool Signals::getStrobes() {
    strobes() = busRead(CNTL);
    return (strobes() & (HDS | LDS)) != (HDS | LDS);
}

PinsH16::PinsH16() : _clock(extal_half_min_ns, clock_input_min_hz) {
    _devs = new DevsMc6800(ACIA_BASE_H16);
    // Memory first: RegsH16 stages its restore frame there.
    _mems = new MemsH16();
    _regs = new RegsH16(this, mems<MemsH16>());
    _clockInput = clock_input_hz;
    _clockCycles = F_CPU / _clockInput;
}

bool PinsH16::setClockInput(uint32_t hz) {
    if (hz > clock_input_max_hz)
        return false;
    const auto rate = _clock.setRate(hz);
    if (rate == 0)
        return false;
    _clockInput = rate;
    _clockCycles = F_CPU / rate;
    return true;
}

void PinsH16::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    // A power-on reset: #NMI high while #RES and #BRTRY are low.
    assert_reset();
    // The clock runs from here on; nothing stops it.
    _clock.begin(_clockInput);
    _clock.start();
    delayMicroseconds(reset_us);

    Cycles::reset();
    digitalWriteFast(PIN_BRTRY, HIGH);
    release_reset();
    for (auto n = 0; digitalReadFast(PIN_RES) == LOW; ++n) {
        if (n >= reset_release_clocks) {
            cli.println("?reset: #RES stuck low");
            return;
        }
        delayNanoseconds(1000000000 / _clockInput);
    }

    // The reset vector, SSP then PC, comes in by prefetch from 000000H,
    // and the prefetch runs on to 00000FH before the PC is taken (User's
    // Manual Figure 4-99).
    const auto s = prepareCycle();
    // Anything but a read at 000000H means the board is not what the
    // driver assumes.
    if (s->halt() || !memRead(s) || s->addr != InstH16::VEC_RESET) {
        cli.print("?reset: first cycle ");
        s->print();
        return;
    }
    // ATTENTION: SSP is set to the park origin, off chip, until a
    // restore loads the program's.
    static constexpr uint8_t VECTOR[] = {
            uint8_t(InstH16::ORG_PARK >> 24),
            uint8_t(InstH16::ORG_PARK >> 16),
            uint8_t(InstH16::ORG_PARK >> 8),
            uint8_t(InstH16::ORG_PARK),  // SSP
            uint8_t(InstH16::ORG_PARK >> 24),
            uint8_t(InstH16::ORG_PARK >> 16),
            uint8_t(InstH16::ORG_PARK >> 8),
            uint8_t(InstH16::ORG_PARK),  // PC
    };
    uint32_t org = InstH16::VEC_RESET;
    execInst(VECTOR, sizeof(VECTOR), org, InstH16::ORG_PARK);
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

// Reset leaves 7 programmed Tw and a Tp in every cycle, and DRAM refresh
// cycles every 32 states (MCR H'F0E0, AWCR0 H'0047, Section 12.2). MCR
// H'8000 keeps the #WAIT input only; AWCR0 H'0000 drops the waits and
// lets user mode in. Area 0 alone covers the address space from reset.
// Both registers are on chip.
void PinsH16::setupBus() {
    // clang-format off
    static constexpr uint8_t SEQ[] = {
        0x0D, 0x73, 0x00, 0x00, 0x80, 0x00,  // MOV.W #$8000, @$FFFFF8 ; MCR
        0x77, 0x00, 0xFF, 0xFF, 0xF8,
        0x1D, 0x00, 0x77, 0x00, 0xFF, 0xFF,  // MOV.W #$0000, @$FFFF2C ; AWCR0
        0x2C,
        0x99, 0xFF, 0xEB,                    // BRA   org
    };
    // clang-format on
    static_assert(SEQ[sizeof(SEQ) - 2] == 0xFF &&
                          SEQ[sizeof(SEQ) - 1] == uint8_t(-sizeof(SEQ)),
            "BRA org: the displacement is from the end of the BRA");
    auto org = _regs->nextIp();
    execInst(SEQ, sizeof(SEQ), org, EXIT_ORG);
    _regs->setIp(org);
}

Signals *PinsH16::prepareCycle() {
    return prepareCycle(halt_clocks);
}

// Wait for #AS and read the address at once, then for the data strobes,
// which a byte cycle needs for A0. A refresh cycle has none.
Signals *PinsH16::prepareCycle(uint_fast16_t clocks) {
    auto s = Signals::put();
    s->clearMark();  // Cycles::next() keeps the slot's old mark
    const uint32_t start = ARM_DWT_CYCCNT;
    const uint32_t limit = clocks * _clockCycles;
    while (!s->getControl()) {
        if (ARM_DWT_CYCCNT - start >= limit) {
            s->markHalt();
            return s;
        }
    }
    s->getAddr();
    if (s->memory()) {
        const uint32_t from = ARM_DWT_CYCCNT;
        const uint32_t strobe = strobe_clocks * _clockCycles;
        while (!s->getStrobes()) {
            if (ARM_DWT_CYCCNT - from >= strobe)
                break;
        }
        if ((s->strobes() & Signals::HDS) != 0)
            s->addr |= 1;  // #LDS alone: the odd byte
    } else {
        s->strobes() = Signals::NONE;
    }
    return s;
}

// Let the parked CPU go. The address is the caller's to give: A1-A16
// carry data by now, and the ring is a log that reset and discard
// rewrite. The parked cycle still drives its status and strobes.
Signals *PinsH16::resumeCycle(uint32_t addr) {
    auto s = Signals::put();
    s->clearMark();
    s->getControl();
    s->getAddr();
    s->getStrobes();
    s->addr = wrap(addr);
    return s;
}

// The CPU waits in Tw: serve the cycle, then negate #WAIT until #AS
// negates at the end of T3 and assert it again, well before the next
// cycle's T2 fall. Masked: a USB interrupt here would let the next cycle
// through unserved.
Signals *PinsH16::completeCycle(Signals *s) {
    if (s->halt()) {
        // Nothing moves; not recorded, so s->prev() stays the previous
        // real cycle.
        s->inputMode();
        return s;
    }
    if (memRead(s)) {
        serveRead(s);
        s->stageData();
        // Past the T2 fall, the bus is ours.
        const uint32_t start = ARM_DWT_CYCCNT;
        const uint32_t limit = strobe_clocks * _clockCycles;
        while (busRead(CNTL) & Signals::PHI) {
            if (ARM_DWT_CYCCNT - start >= limit)
                break;
        }
        s->driveData();
    } else if (memWrite(s)) {
        // Write data is valid before the strobes assert (twDS).
        s->getData();
        storeWrite(s);
    }
    const uint32_t limit = strobe_clocks * _clockCycles;
    noInterrupts();
    // #WAIT must change outside its sampling window at a PHI fall (setup
    // 35 ns, hold 7-10 ns). A read gets here just past the T2 fall.
    // ATTENTION: a write negates it with no regard to PHI; check in a
    // capture whether it ever lands in the window.
    negate_wait();
    const uint32_t start = ARM_DWT_CYCCNT;
    // A read's data is released as the strobes negate.
    Signals probe;
    if (memRead(s)) {
        while (probe.getStrobes()) {
            if (ARM_DWT_CYCCNT - start >= limit)
                break;
        }
        s->inputMode();
    }
    while (probe.getControl()) {
        if (ARM_DWT_CYCCNT - start >= limit)
            break;
    }
    assert_wait();
    interrupts();
    Cycles::next();
    return s;
}

// Look up the read. A word access is even, so its address alone
// selects.
void PinsH16::serveRead(Signals *s) {
    if (s->readMemory()) {
        const auto maddr = wrap(s->addr);
        if (_devs->isSelected(maddr)) {
            s->data = s->wordAccess() ? uint16(_devs->read(maddr),
                                                _devs->read(maddr + 1))
                                      : bothLanes(_devs->read(maddr));
        } else {
            // The aligned pair: a byte read takes its own lane of it.
            s->data = mems<MemsH16>()->read_bus(maddr & ~1);
        }
    }
}

void PinsH16::storeWrite(Signals *s) {
    if (!s->writeMemory())
        return;
    const auto mems = this->mems<MemsH16>();
    const auto maddr = wrap(s->addr);
    if (_devs->isSelected(maddr)) {
        if (s->wordAccess()) {
            _devs->write(maddr, hi(s->data));
            _devs->write(maddr + 1, lo(s->data));
        } else {
            _devs->write(maddr, lane(s));
        }
    } else if (s->wordAccess()) {
        mems->write_bus(maddr, s->data);
    } else {
        mems->write_byte(maddr, lane(s));
    }
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
        return InstH16::NOP;
    };
    const auto even = addr & ~UINT32_C(1);
    return static_cast<uint16_t>(at(even)) << 8 | at(even + 1);
}

// Inject |word| as |s| moves it: whole, or its own lane on both.
void injectWord(Signals *s, uint16_t word) {
    if (s->wordAccess()) {
        s->inject(word);
    } else {
        s->inject(bothLanes((s->addr & 1) ? word : word >> 8));
    }
}
}  // namespace

uint_fast8_t PinsH16::execute(const uint8_t *inst, uint_fast8_t len,
        uint8_t *buf, uint32_t *addrs, uint_fast8_t max, uint32_t &org,
        uint32_t exit) {
    uint_fast8_t cap = 0;
    // |inst| is answered by address. |exit| is where the sequence ends --
    // a prefetch past the window looks just like falling through, so the
    // caller must say.
    const uint32_t leaves = exit == EXIT_ORG ? org : wrap(exit);
    bool started = false;
    // The exit may lie inside the window: it counts only once the last
    // byte has been fetched.
    bool fetchedAll = false;
    bool exited = false;
    auto s = resumeCycle(org);
    // Bound the damage: a wrong dump beats a hung debugger.
    auto guard = Cycles::MAX_CYCLES;
    while (guard--) {
        if (s->halt())
            break;
        const auto reading = memRead(s);
        const auto atExit = fetchedAll && reading && s->programFetch() &&
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
        // there, an LDM or an RTE, come from memory.
        if (inWindow || (reading && s->programFetch())) {
            injectWord(s, busWord(inst, len, org, s->addr));
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
    // The CPU is parked in |s|: the caller resumes from its address, which
    // is |leaves| unless the guard ran out.
    _cutShort = !exited;
    org = s->addr;
    return cap;
}

void PinsH16::execInst(
        const uint8_t *inst, uint_fast8_t len, uint32_t &org, uint32_t exit) {
    execute(inst, len, nullptr, nullptr, 0, org, exit);
}

uint_fast8_t PinsH16::captureWrites(const uint8_t *inst, uint_fast8_t len,
        uint8_t *buf, uint32_t *addrs, uint_fast8_t max, uint32_t &org,
        uint32_t exit) {
    return execute(inst, len, buf, addrs, max, org, exit);
}

bool PinsH16::answerVector(uint32_t vecAddr, uint32_t handler) {
    const uint8_t vector[] = {uint8_t(handler >> 24), uint8_t(handler >> 16),
            uint8_t(handler >> 8), uint8_t(handler)};
    auto org = vecAddr;
    execute(vector, sizeof(vector), nullptr, nullptr, 0, org, handler);
    return !_cutShort;
}

// The read of the vector at |offset| from EBR: data space, not a fetch
// (User's Manual Table 4-13 note).
bool PinsH16::isVector(const Signals *s, uint32_t offset) const {
    return memRead(s) && !s->programFetch() &&
           s->addr == wrap(regs<RegsH16>()->ebr() + offset);
}

namespace {
// The frame an exception stacked shortly before its vector read |s|: SR
// below the PC (User's Manual Figure 4-62), as the writes left them in
// memory. Three word writes, or more from an odd SSP. |from| becomes the
// first of them.
bool findFrame(const Signals *s, const MemsH16 *mems, RegsH16::Frame &frame,
        const Signals *&from) {
    uint_fast8_t bytes = 0;
    uint32_t low = UINT32_MAX;
    for (auto i = 1; i <= frame_lookback && bytes < 6; ++i) {
        const auto t = s->prev(i);
        if (!memWrite(t))
            continue;
        bytes += t->wordAccess() ? 2 : 1;
        if (t->addr < low)
            low = t->addr & (t->wordAccess() ? ~UINT32_C(1) : ~UINT32_C(0));
        from = t;
    }
    if (bytes < 6)
        return false;
    frame.sr = mems->read16(low);
    frame.pc = (static_cast<uint32_t>(mems->read16(wrap(low + 2))) << 16 |
                       mems->read16(wrap(low + 4))) &
               ADDR_MASK;
    frame.sp = wrap(low + 6);
    return true;
}
}  // namespace

// Stop at an instruction boundary: #NMI is taken there and the CPU parks
// in its vector read with PC and SR stacked. With |org| UINT32_MAX the
// CPU has no parked cycle, asleep, and #NMI wakes it.
bool PinsH16::suspend(uint32_t org) {
    Signals *s;
    if (org == UINT32_MAX) {
        assert_nmi();
        s = prepareCycle();
    } else {
        s = resumeCycle(org);
        assert_nmi();
        completeCycle(s);
        s = prepareCycle();
    }
    // Bounded: with SSP in the on-chip RAM the frame never shows.
    auto guard = nmi_ack_cycles;
    while (guard-- && !s->halt()) {
        if (isVector(s, InstH16::VEC_NMI)) {
            RegsH16::Frame frame;
            const Signals *from = nullptr;
            if (findFrame(s, mems<MemsH16>(), frame, from)) {
                negate_nmi();
                // The frame and this read are the debugger's.
                Cycles::discard(from);
                regs<RegsH16>()->parkInVector(s->addr, frame);
                return true;
            }
        }
        completeCycle(s);
        s = prepareCycle();
    }
    negate_nmi();
    cli.println(s->halt() ? "?halt: no NMI vector read (stack on chip?)"
                          : "?halt: no NMI vector read");
    return false;
}

// One instruction under trace: the restore sets T in the SR that RTE
// takes, and the trace exception after the instruction parks the CPU in
// its vector read; T is cleared as the trace is taken. An exception the
// instruction raises instead goes to its handler with T cleared, so
// #NMI, asserted at its vector read, parks the CPU at the handler.
// ATTENTION: unverified that a trace taken with T set by RTE follows the
// very next instruction, and whether a trace follows a trap as well.
bool PinsH16::step(bool show) {
    Cycles::reset();
    regs<RegsH16>()->restore(true);
    if (_cutShort) {
        cli.println("?step: restore cut short");
        return false;
    }
    if (show)
        Cycles::reset();
    auto s = resumeCycle(regs<RegsH16>()->parkedAt());
    auto parked = false;
    auto nmi = false;
    for (auto guard = Cycles::MAX_CYCLES; guard && !parked; --guard) {
        if (s->halt()) {
            // SLEEP: #NMI wakes it.
            if (!suspend(UINT32_MAX))
                return false;
            parked = true;
            break;
        }
        RegsH16::Frame frame;
        const Signals *from = nullptr;
        const auto vector = isException(s, frame, from);
        if (vector == InstH16::VEC_TRACE || isHalt(s, frame, from)) {
            parked = maskBreak(s, frame);
            Cycles::discard(from);
            if (!parked)
                return false;
            break;
        }
        if (nmi && vector == InstH16::VEC_NMI) {
            negate_nmi();
            Cycles::discard(from);
            regs<RegsH16>()->parkInVector(s->addr, frame);
            parked = true;
            break;
        }
        if (vector != InstH16::VEC_END && !nmi) {
            assert_nmi();
            nmi = true;
        }
        completeCycle(s);
        s = prepareCycle();
    }
    if (!parked) {
        negate_nmi();
        cli.println("?step: no trace vector read");
        return false;
    }
    Cycles::Hold hold;  // keep the step's cycles for printCycles()
    _regs->save();
    if (show)
        printCycles();
    return true;
}

// The vector |s| reads, as an offset from EBR, when it follows the frame
// an exception stacks; VEC_END otherwise. |frame| is that frame, and
// |from| its first write.
uint32_t PinsH16::isException(
        const Signals *s, RegsH16::Frame &frame, const Signals *&from) const {
    if (!memRead(s) || s->programFetch())
        return InstH16::VEC_END;
    const auto offset = wrap(s->addr - regs<RegsH16>()->ebr());
    if (offset >= InstH16::VEC_END || (offset & 3) != 0 ||
            !findFrame(s, mems<MemsH16>(), frame, from))
        return InstH16::VEC_END;
    return offset;
}

// A read of the break vector at a breakpoint reports a break.
bool PinsH16::isBreak(
        const Signals *s, RegsH16::Frame &frame, const Signals *&from) const {
    return isVector(s, InstH16::VEC_BREAK) &&
           findFrame(s, mems<MemsH16>(), frame, from) &&
           isBreakPoint(frame.pc);
}

// A read of the TRAPA #15 vector, pointing at itself, after a TRAPA #15
// reports the halt a program ends with.
bool PinsH16::isHalt(
        const Signals *s, RegsH16::Frame &frame, const Signals *&from) const {
    if (!isVector(s, InstH16::VEC_HALT) ||
            !findFrame(s, mems<MemsH16>(), frame, from))
        return false;
    const auto pc = wrap(frame.pc - InstH16::HALT_PC_OFFSET);
    if (_mems->read(pc) != InstH16::TRAPA ||
            _mems->read(pc + 1) != InstH16::TRAPA_HALT)
        return false;
    const auto vec = wrap(s->addr);
    const uint32_t handler =
            static_cast<uint32_t>(_mems->read16(vec)) << 16 |
            _mems->read16(vec + 2);
    if ((handler & ADDR_MASK) != vec)
        return false;
    frame.pc = pc;
    return true;
}

// A trace, a break or a halt leaves the interrupt mask as the program had it, so a
// pending interrupt could take the save's first fetch. #NMI outranks them
// all: asserted as the vector is answered with the park origin, it is
// taken at the first boundary and masks everything. Its own frame is the
// debugger's; the program's state is |frame|.
// ATTENTION: that #NMI is taken by that boundary is unverified.
bool PinsH16::maskBreak(Signals *s, const RegsH16::Frame &frame) {
    static constexpr uint8_t VECTOR[] = {
            uint8_t(InstH16::ORG_PARK >> 24),
            uint8_t(InstH16::ORG_PARK >> 16),
            uint8_t(InstH16::ORG_PARK >> 8),
            uint8_t(InstH16::ORG_PARK),
    };
    const auto vecAddr = s->addr;
    assert_nmi();
    auto guard = nmi_ack_cycles;
    while (guard-- && !s->halt()) {
        if (isVector(s, InstH16::VEC_NMI)) {
            negate_nmi();
            regs<RegsH16>()->parkInVector(s->addr, frame);
            return true;
        }
        const auto vector = memRead(s) && !s->programFetch() &&
                            (s->addr & ~3) == vecAddr;
        if (vector || (memRead(s) && s->programFetch())) {
            injectWord(s, busWord(VECTOR, sizeof(VECTOR), vecAddr, s->addr));
        } else if (memWrite(s)) {
            s->capture();
        }
        completeCycle(s);
        s = prepareCycle();
    }
    negate_nmi();
    cli.println("?break: no NMI vector read");
    return false;
}

// Free-run. Returns whether the CPU stopped at a boundary its registers
// can be saved from. The ring is left holding the program's cycles only.
bool PinsH16::loop() {
    auto s = resumeCycle(regs<RegsH16>()->parkedAt());
    uint_fast8_t poll = 0;
    while (true) {
        const Signals *from;
        RegsH16::Frame frame;
        if (isBreak(s, frame, from) || isHalt(s, frame, from)) {
            const auto parked = maskBreak(s, frame);
            Cycles::discard(from);
            return parked;
        }
        if (s->halt()) {
            // SLEEP: no bus cycle until an interrupt.
            _devs->loop();
            if (haltSwitch())
                return suspend(UINT32_MAX);
            s = prepareCycle(wait_poll_clocks);
            continue;
        }
        completeCycle(s);
        _devs->loop();
        s = prepareCycle();
        // Only now, with a cycle recorded: suspend() opens with
        // resumeCycle(), which reads that slot back.
        if (++poll < halt_poll_cycles)
            continue;
        poll = 0;
        if (haltSwitch())
            return s->halt() ? suspend(UINT32_MAX) : suspend(s->addr);
    }
}

void PinsH16::run() {
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

void PinsH16::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, InstH16::BREAK);
}

void PinsH16::assertInt(uint8_t) {
    digitalWriteFast(PIN_IRQ0, LOW);
}

void PinsH16::negateInt(uint8_t) {
    digitalWriteFast(PIN_IRQ0, HIGH);
}

void PinsH16::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

// ATTENTION: every cycle, undisassembled. The prefetch queue hides where
// instructions start: PF marks fetches, not the ones executed.
void PinsH16::printBacktrace() {
    printCycles();
}

}  // namespace h16
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
