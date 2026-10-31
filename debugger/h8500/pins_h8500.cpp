#include "pins_h8500.h"
#include "debugger.h"
#include "devs_h8500.h"
#include "digital_bus.h"
#include "inst_h8500.h"
#include "mems_h8500.h"
#include "regs_h8500.h"
#include "signals_h8500.h"

namespace debugger {
namespace h8500 {

// clang-format off
/**
 * H8/500 external access cycle (H8/510 and H8/536 Hardware Manuals 3.7.6
 * and 7.3). The system clock ø is EXTAL/2, one state a ø period:
 *
 *               T1      T2      Tw      T3      T1
 *            ___     ___     ___     ___     ___
 *     ø   __/   \___/   \___/   \___/   \___/   \__
 *         ______                          ________
 *    #AS        \________________________/
 *    #RD        \________________________/
 *    A0-A23 ---<       address              >-----
 *    D0-D15 ----------------------< read  >-------
 *         ______________                  ________
 *  #HWR/#LWR            \________________/
 *                           ^       ^
 *                           #WAIT sampled
 *
 * #AS and #RD assert in T1 and the address holds through T3; the write
 * strobes assert in T2 with the data. In the pin wait mode #WAIT is
 * sampled at the ø rise of T2 and of every Tw, and inserts Tw while it is
 * low. The on-chip RAM and register field are accessed with #AS, #RD and
 * #WR high, so they never show here.
 *
 * Reset leaves 3 programmed waits in every external cycle and the #WAIT
 * pin a port: until the setup sequence selects the pin wait mode, every
 * cycle must be served within T3. EXTAL runs free from FlexPWM3: the
 * chips are dynamic, ø no slower than 2000ns. #WAIT stays asserted, so
 * every cycle waits in Tw once the setup has run, and is negated only
 * until #AS negates at the end of T3.
 */
// clang-format on

namespace {

// ATTENTION: unbenched. The EXTAL rate a run, and everything else, uses:
// ø 2 MHz, slow enough to serve the cycles reset leaves unheld.
constexpr uint32_t clock_input_hz = 4000000;
// EXTAL's high and low times at least this: half the 50ns EXTAL period
// that the 100ns tcyc minimum makes.
constexpr auto extal_half_min_ns = 25;
// #RES low this long resets: 520 tcyc for the H8/536's tRESW2, at the
// slowest ø, and more.
constexpr auto reset_us = 10000;
// #RES rises within this many EXTAL clocks of its release: only the
// pull-up drives it.
constexpr auto reset_release_clocks = 64;
// No #AS for this many EXTAL clocks: the CPU sleeps or runs from on-chip
// memory. Long enough to clear DIVXU with an external operand.
constexpr auto halt_clocks = 4096;
// EXTAL clocks between polls of a CPU already stopped: short enough to
// service the devices and the halt switch often.
constexpr auto wait_poll_clocks = 64;
// Bus cycles between polls of the halt switch in a run.
constexpr auto halt_poll_cycles = 16;
// EXTAL clocks a cycle takes, at most, to show its strobes, and to end
// once #WAIT is negated: T2, three programmed Tw, T3 and slack.
constexpr auto strobe_clocks = 16;
// Bus cycles allowed for an NMI to be taken; generous.
constexpr auto nmi_ack_cycles = 1024;
// Cycles before an exception's vector read that may hold its frame: SR,
// CP and PC, six bytes written a byte at a time on the H8/536, and the
// prefetches between.
constexpr auto frame_lookback = 16;
// ATTENTION: the prefetch past an injected sequence's last byte is fed
// NOPs, up to this far; how far the CPU fetches ahead is unverified (the
// Programming Manual counts one word per fetch cycle, 2.6.4).
constexpr auto prefetch_bytes = 4;
// NOPs fed at the park origin until a pending NMI is taken: a few
// instructions' worth.
constexpr auto nmi_nop_bytes = 16;

const uint8_t PINS_LOW[] = {
        PIN_WAIT,
        PIN_ASEL0,
        PIN_ASEL1,
};

const uint8_t PINS_HIGH[] = {
        PIN_NMI,
        PIN_IRQ0,
};

const uint8_t PINS_INPUT[] = {
        PIN_AS,
        PIN_RD,
        PIN_HWR,
        PIN_PHI,
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
        PIN_AM0,
        PIN_AM1,
        PIN_AM2,
        PIN_AM3,
        PIN_AH0,
        PIN_AH1,
        PIN_AH2,
        PIN_AH3,
};

// #RES is open drain on the H8/536, whose watchdog may drive it (RSTOE).
// Drive it low or let the pull-up have it.
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

// NMI takes its falling edge, NMIEG cleared at reset; it is held low until
// its vector read.
void assert_nmi() {
    digitalWriteFast(PIN_NMI, LOW);
}

void negate_nmi() {
    digitalWriteFast(PIN_NMI, HIGH);
}

inline uint16_t bothLanes(uint8_t v) {
    return static_cast<uint16_t>(v) << 8 | v;
}

// The byte a byte write moves: the even address on D8-D15, the H8/536's
// on D0-D7.
inline uint8_t lane(const Signals *s) {
    if (!Signals::bus16())
        return s->data;
    return (s->addr & 1) ? s->data : s->data >> 8;
}

}  // namespace

inline bool Signals::getControl() {
    cntl() = busRead(CNTL);
    return (cntl() & AS) == 0;
}

inline bool Signals::getStrobes() {
    strobes() = busRead(CNTL);
    return (strobes() & (RD | HWR | LWR)) != (RD | HWR | LWR);
}

PinsH8500::PinsH8500(const ChipH8500 &chip)
    : _chip(chip), _clock(extal_half_min_ns, chip.extalMinHz) {
    Signals::setChip(&chip);
    _devs = new DevsMc6800(ACIA_BASE_H8500);
    // Memory first: RegsH8500 stages its restore frame there.
    _mems = new MemsH8500(chip.addrMask);
    _regs = new RegsH8500(this, mems<MemsH8500>());
    _clockInput = clock_input_hz;
    _clockCycles = F_CPU / _clockInput;
}

bool PinsH8500::setClockInput(uint32_t hz) {
    if (hz > _chip.extalMaxHz)
        return false;
    const auto rate = _clock.setRate(hz);
    if (rate == 0)
        return false;
    _clockInput = rate;
    _clockCycles = F_CPU / rate;
    return true;
}

void PinsH8500::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    // The H8/510's #LWR; nothing drives the pin on the H8/536.
    pinMode(PIN_LWR, INPUT_PULLUP);
    assert_reset();
    // The clock runs from here on; nothing stops it.
    _clock.begin(_clockInput);
    _clock.start();
    delayMicroseconds(reset_us);

    Cycles::reset();
    // No cycle can wait for the Teensy until the setup selects the pin
    // wait mode: a USB interrupt in one would let it end unserved.
    noInterrupts();
    release_reset();
    for (auto n = 0; digitalReadFast(PIN_RES) == LOW; ++n) {
        if (n >= reset_release_clocks) {
            interrupts();
            cli.println("?reset: #RES stuck low");
            return;
        }
        delayNanoseconds(1000000000 / _clockInput);
    }

    // The reset vector: CP in the low byte of the word at H'0000, then PC
    // (Hardware Manual Figure 4-2).
    const auto s = prepareCycle();
    // Anything but a read at H'0000 means the board is not what the
    // driver assumes.
    if (s->halt() || !s->read() || s->addr != InstH8500::VEC_RESET) {
        interrupts();
        cli.print("?reset: first cycle ");
        s->print();
        return;
    }
    static constexpr uint8_t VECTOR[] = {
            0,
            uint8_t(InstH8500::ORG_PARK >> 16),  // CP
            uint8_t(InstH8500::ORG_PARK >> 8),
            uint8_t(InstH8500::ORG_PARK),  // PC
    };
    uint32_t org = InstH8500::VEC_RESET;
    execInst(VECTOR, sizeof(VECTOR), org, InstH8500::ORG_PARK);
    if (_cutShort) {
        interrupts();
        cli.println("?reset: no fetch from the reset vector");
        printCycles();
        return;
    }
    _regs->setIp(org);
    const auto setup = setupBus();
    interrupts();
    if (!setup) {
        cli.println("?reset: setup sequence cut short");
        printCycles();
        return;
    }
    _regs->save();
    _regs->reset();
}

// DP first: the register field is in page 0, and DP is undetermined at
// reset. WCR H'F8 selects the pin wait mode with no programmed waits;
// #IRQ0's pin function is enabled where this chip has it. TP and SP give
// the CPU a stack in the park page, external, until a program sets its
// own (the reset routine Hardware Manual 4.2.3 asks for). The LDC to TP
// holds interrupts off for the next instruction, though all are masked
// from reset anyway.
bool PinsH8500::setupBus() {
    const uint8_t SEQ[] = {
            0x04, 0x00, 0x8D,  // LDC.B   #0, DP
            0x15, uint8_t(_chip.wcr >> 8), uint8_t(_chip.wcr), 0x06,
            0xF8,  // MOV:G.B #H'F8, @wcr:16
            0x15, uint8_t(_chip.irq0eReg >> 8), uint8_t(_chip.irq0eReg),
            uint8_t(0xC0 | _chip.irq0eBit),  // BSET.B  #bit, @irq0e:16
            0x04, uint8_t(InstH8500::STACK_PARK >> 16),
            0x8F,  // LDC.B   #page, TP
            0x5F, uint8_t(InstH8500::STACK_PARK >> 8),
            uint8_t(InstH8500::STACK_PARK),  // MOV:I.W #sp, R7
            0x20, uint8_t(-20),              // BRA     org
    };
    static_assert(sizeof(SEQ) == 20, "BRA org: from the end of the BRA");
    auto org = _regs->nextIp();
    execInst(SEQ, sizeof(SEQ), org, EXIT_ORG);
    _regs->setIp(org);
    return !_cutShort;
}

Signals *PinsH8500::prepareCycle() {
    return prepareCycle(halt_clocks);
}

// Wait for #AS and read the address, then for a strobe: #RD asserts with
// #AS, the write strobes in T2.
Signals *PinsH8500::prepareCycle(uint_fast16_t clocks) {
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
    const uint32_t from = ARM_DWT_CYCCNT;
    const uint32_t strobe = strobe_clocks * _clockCycles;
    while (!s->getStrobes()) {
        if (ARM_DWT_CYCCNT - from >= strobe)
            break;
    }
    return s;
}

// Let the parked CPU go. The address is the caller's to give: the ring is
// a log that reset and discard rewrite. The parked cycle still drives its
// strobes.
Signals *PinsH8500::resumeCycle(uint32_t addr) {
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
// cycle's T2 rise. The address bus is not multiplexed, so read data goes
// out at once. Masked: a USB interrupt here would let the next cycle
// through unserved.
Signals *PinsH8500::completeCycle(Signals *s) {
    if (s->halt()) {
        // Nothing moves; not recorded, so s->prev() stays the previous
        // real cycle.
        s->inputMode();
        return s;
    }
    if (s->read()) {
        serveRead(s);
        s->stageData();
        s->driveData();
    } else if (s->write()) {
        // ATTENTION: write data is read as soon as a write strobe shows;
        // both come in T2, the data held through the Tw.
        s->getData();
        storeWrite(s);
    }
    const uint32_t limit = strobe_clocks * _clockCycles;
    // Left masked if it was: the reset serves its unheld cycles with
    // interrupts off throughout.
    uint32_t primask;
    __asm__ volatile("mrs %0, primask" : "=r"(primask));
    noInterrupts();
    // ATTENTION: #WAIT changes with no regard to ø; check in a capture
    // whether it ever lands at a sampling ø rise.
    negate_wait();
    const uint32_t start = ARM_DWT_CYCCNT;
    // The read data is latched at the T3 fall, before #RD negates.
    Signals probe;
    if (s->read()) {
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
    if ((primask & 1) == 0)
        interrupts();
    Cycles::next();
    return s;
}

// Look up the read. Memory gives the aligned pair, which serves a byte or
// a word read alike; a device gives only the byte the address selects, so
// a status read never consumes data, and a word read of a device is not
// served.
void PinsH8500::serveRead(Signals *s) {
    if (!s->readMemory())
        return;
    const auto maddr = wrap(s->addr);
    if (!Signals::bus16()) {
        s->data = _devs->isSelected(maddr) ? _devs->read(maddr)
                                           : _mems->read(maddr);
    } else if (_devs->isSelected(maddr)) {
        s->data = bothLanes(_devs->read(maddr));
    } else {
        s->data = mems<MemsH8500>()->read_bus(maddr & ~1);
    }
}

void PinsH8500::storeWrite(Signals *s) {
    if (!s->writeMemory())
        return;
    const auto mems = this->mems<MemsH8500>();
    const auto maddr = wrap(s->addr);
    if (Signals::bus16() && s->wordWrite()) {
        if (_devs->isSelected(maddr)) {
            _devs->write(maddr, hi(s->data));
            _devs->write(maddr + 1, lo(s->data));
        } else {
            mems->write_bus(maddr & ~1, s->data);
        }
    } else if (_devs->isSelected(maddr)) {
        _devs->write(maddr, lane(s));
    } else {
        mems->write_byte(maddr, lane(s));
    }
}

namespace {
// The byte at |addr| of |inst| placed at |org|. Outside the window it is
// a NOP, so a prefetch past the end runs nothing.
uint8_t instByte(
        const uint8_t *inst, uint_fast8_t len, uint32_t org, uint32_t addr) {
    const auto off = addr - org;
    if (addr >= org && off < len)
        return inst[off];
    return InstH8500::NOP;
}

// Inject what |s| reads of |inst|: the aligned pair on the H8/510.
void injectInst(
        Signals *s, const uint8_t *inst, uint_fast8_t len, uint32_t org) {
    if (Signals::bus16()) {
        const auto even = s->addr & ~UINT32_C(1);
        s->inject(static_cast<uint16_t>(instByte(inst, len, org, even)) << 8 |
                  instByte(inst, len, org, even + 1));
    } else {
        s->inject(instByte(inst, len, org, s->addr));
    }
}
}  // namespace

uint_fast8_t PinsH8500::execute(const uint8_t *inst, uint_fast8_t len,
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
        const auto reading = s->read();
        // A read moves the aligned pair on the H8/510, so it is in the
        // window when either byte is.
        const auto first = Signals::bus16() ? s->addr & ~UINT32_C(1) : s->addr;
        const auto last = Signals::bus16() ? first + 1 : first;
        const auto atExit = fetchedAll && reading && first <= leaves &&
                            leaves <= last;
        if (started && cap >= max && atExit) {
            exited = true;
            break;
        }
        if (reading)
            started = true;
        const auto end = org + len;
        const auto inWindow = reading && last >= org && first < end;
        if (inWindow && last >= end - 1)
            fetchedAll = true;
        // With no fetch status on the bus, a read just past the window is
        // taken for prefetch and gets NOPs; other reads outside it, an RTE
        // or a vector, come from memory.
        const auto prefetch = reading && first >= end &&
                              first < end + prefetch_bytes;
        if (inWindow || prefetch) {
            injectInst(s, inst, len, org);
        } else if (s->write()) {
            s->capture();
        }
        completeCycle(s);
        if (s->write()) {
            const auto put = [&](uint32_t a, uint8_t v) {
                if (cap < max) {
                    if (buf)
                        buf[cap] = v;
                    if (addrs)
                        addrs[cap] = wrap(a);
                    ++cap;
                }
            };
            if (Signals::bus16() && s->wordWrite()) {
                put(s->addr & ~1, hi(s->data));
                put((s->addr & ~1) + 1, lo(s->data));
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

void PinsH8500::execInst(
        const uint8_t *inst, uint_fast8_t len, uint32_t &org, uint32_t exit) {
    execute(inst, len, nullptr, nullptr, 0, org, exit);
}

uint_fast8_t PinsH8500::captureWrites(const uint8_t *inst, uint_fast8_t len,
        uint8_t *buf, uint32_t *addrs, uint_fast8_t max, uint32_t &org,
        uint32_t exit) {
    return execute(inst, len, buf, addrs, max, org, exit);
}

// A vector is a word holding CP in its low byte, then PC.
bool PinsH8500::answerVector(uint32_t vecAddr, uint32_t handler) {
    const uint8_t vector[] = {0, uint8_t(handler >> 16), uint8_t(handler >> 8),
            uint8_t(handler)};
    auto org = vecAddr;
    execute(vector, sizeof(vector), nullptr, nullptr, 0, org, handler);
    return !_cutShort;
}

// The read of the vector at |vec|, in page 0. A vector read and a program
// fetch look alike on the bus: findFrame() tells them apart.
bool PinsH8500::isVector(const Signals *s, uint32_t vec) const {
    return s->read() && (s->addr & ~UINT32_C(1)) == vec;
}

// The frame an exception stacked shortly before its vector read |s|: SR
// lowest, then a word holding CP, then PC (Hardware Manual Table 4-3), as
// the writes left them in memory. Three word writes, or six byte writes
// on the H8/536. |from| becomes the first of them.
bool PinsH8500::findFrame(const Signals *s, RegsH8500::Frame &frame,
        const Signals *&from) const {
    uint_fast8_t bytes = 0;
    uint32_t low = UINT32_MAX;
    for (auto i = 1; i <= frame_lookback && bytes < 6; ++i) {
        const auto t = s->prev(i);
        if (!t->write())
            continue;
        const auto word = Signals::bus16() && t->wordWrite();
        bytes += word ? 2 : 1;
        const auto a = word ? t->addr & ~UINT32_C(1) : t->addr;
        if (a < low)
            low = a;
        from = t;
    }
    if (bytes < 6)
        return false;
    const auto mems = this->mems<MemsH8500>();
    frame.sr = mems->read16(low);
    frame.pc = static_cast<uint32_t>(mems->read(wrap(low + 3))) << 16 |
               mems->read16(wrap(low + 4));
    // TP:SP before the frame was stacked; the stack wraps within its page.
    frame.sp = (low & ~UINT32_C(0xFFFF)) | ((low + 6) & 0xFFFF);
    return true;
}

// Stop at an instruction boundary: NMI is taken there and the CPU parks in
// its vector read with the frame stacked. With |org| UINT32_MAX the CPU
// has no parked cycle, asleep, and NMI wakes it.
bool PinsH8500::suspend(uint32_t org) {
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
    // Bounded: with the stack in on-chip RAM the frame never shows.
    auto guard = nmi_ack_cycles;
    while (guard-- && !s->halt()) {
        if (isVector(s, InstH8500::VEC_NMI)) {
            RegsH8500::Frame frame;
            const Signals *from = nullptr;
            if (findFrame(s, frame, from)) {
                negate_nmi();
                // The frame and this read are the debugger's.
                Cycles::discard(from);
                regs<RegsH8500>()->parkInVector(s->addr, frame);
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
// instruction raises instead goes to its handler with T cleared, so NMI,
// asserted at its vector read, parks the CPU at the handler.
// ATTENTION: unverified that a trace taken with T set by RTE follows the
// very next instruction, and whether a trace follows a trap as well.
bool PinsH8500::step(bool show) {
    Cycles::reset();
    regs<RegsH8500>()->restore(true);
    if (_cutShort) {
        cli.println("?step: restore cut short");
        return false;
    }
    if (show)
        Cycles::reset();
    auto s = resumeCycle(regs<RegsH8500>()->parkedAt());
    auto parked = false;
    auto nmi = false;
    for (auto guard = Cycles::MAX_CYCLES; guard && !parked; --guard) {
        if (s->halt()) {
            // SLEEP: NMI wakes it.
            if (!suspend(UINT32_MAX))
                return false;
            parked = true;
            break;
        }
        RegsH8500::Frame frame;
        const Signals *from = nullptr;
        const auto vector = isException(s, frame, from);
        if (vector == InstH8500::VEC_TRACE || isHalt(s, frame, from)) {
            parked = maskBreak(s, frame);
            Cycles::discard(from);
            if (!parked)
                return false;
            break;
        }
        if (nmi && vector == InstH8500::VEC_NMI) {
            negate_nmi();
            Cycles::discard(from);
            regs<RegsH8500>()->parkInVector(s->addr, frame);
            parked = true;
            break;
        }
        if (vector != InstH8500::VEC_END && !nmi) {
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

// The vector |s| reads when it follows the frame an exception stacks;
// VEC_END otherwise. |frame| is that frame, and |from| its first write.
uint32_t PinsH8500::isException(const Signals *s, RegsH8500::Frame &frame,
        const Signals *&from) const {
    if (!s->read() || s->addr >= InstH8500::VEC_END || (s->addr & 3) != 0 ||
            !findFrame(s, frame, from))
        return InstH8500::VEC_END;
    return s->addr;
}

// A read of the invalid instruction vector after a breakpoint reports a
// break. ATTENTION: the PC an invalid instruction stacks "is not
// necessarily the address of the first byte" (Hardware Manual Table
// 4-3); a breakpoint up to two bytes below it is taken until the encoding
// sample's invalid-pc line tells.
bool PinsH8500::isBreak(const Signals *s, RegsH8500::Frame &frame,
        const Signals *&from) const {
    if (!isVector(s, InstH8500::VEC_BREAK) || !findFrame(s, frame, from))
        return false;
    for (uint32_t back = 1; back <= 2; ++back) {
        const auto pc = wrap(frame.pc - back);
        if (isBreakPoint(pc)) {
            frame.pc = pc;
            return true;
        }
    }
    return isBreakPoint(frame.pc);
}

// A read of the TRAPA #15 vector, pointing at itself, after a TRAPA #15
// reports the halt a program ends with.
bool PinsH8500::isHalt(const Signals *s, RegsH8500::Frame &frame,
        const Signals *&from) const {
    if (!isVector(s, InstH8500::VEC_HALT) || !findFrame(s, frame, from))
        return false;
    const auto pc = wrap(frame.pc - InstH8500::HALT_PC_OFFSET);
    if (_mems->read(pc) != InstH8500::TRAPA ||
            _mems->read(pc + 1) != InstH8500::TRAPA_HALT)
        return false;
    const auto vec = InstH8500::VEC_HALT;
    const uint32_t handler = static_cast<uint32_t>(_mems->read(vec + 1)) << 16 |
                             mems<MemsH8500>()->read16(vec + 2);
    if (handler != vec)
        return false;
    frame.pc = pc;
    return true;
}

// A trace, a break or a halt leaves the interrupt mask as the program had
// it, so a pending interrupt could take the save's first fetch. NMI
// outranks them all: asserted as the vector is answered with the park
// origin, it is taken at the first boundary and masks everything. Its own
// frame is the debugger's; the program's state is |frame|.
// ATTENTION: that NMI is taken by that boundary is unverified.
bool PinsH8500::maskBreak(Signals *s, const RegsH8500::Frame &frame) {
    static constexpr uint8_t VECTOR[] = {
            0,
            uint8_t(InstH8500::ORG_PARK >> 16),
            uint8_t(InstH8500::ORG_PARK >> 8),
            uint8_t(InstH8500::ORG_PARK),
    };
    const auto vecAddr = s->addr & ~UINT32_C(3);
    assert_nmi();
    auto guard = nmi_ack_cycles;
    while (guard-- && !s->halt()) {
        if (isVector(s, InstH8500::VEC_NMI)) {
            negate_nmi();
            regs<RegsH8500>()->parkInVector(s->addr, frame);
            return true;
        }
        const auto vector = s->read() && (s->addr & ~UINT32_C(3)) == vecAddr;
        const auto park = s->read() && s->addr >= InstH8500::ORG_PARK &&
                          s->addr < InstH8500::ORG_PARK + nmi_nop_bytes;
        if (vector || park) {
            // The vector, then NOPs at the park origin until NMI is taken.
            injectInst(s, VECTOR, sizeof(VECTOR), vecAddr);
        } else if (s->write()) {
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
bool PinsH8500::loop() {
    auto s = resumeCycle(regs<RegsH8500>()->parkedAt());
    uint_fast8_t poll = 0;
    while (true) {
        const Signals *from;
        RegsH8500::Frame frame;
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

void PinsH8500::run() {
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

void PinsH8500::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, InstH8500::BREAK);
}

// #IRQ0 is low-level triggered on both chips; the setup enabled its pin.
void PinsH8500::assertInt(uint8_t) {
    digitalWriteFast(PIN_IRQ0, LOW);
}

void PinsH8500::negateInt(uint8_t) {
    digitalWriteFast(PIN_IRQ0, HIGH);
}

void PinsH8500::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

// ATTENTION: every cycle, undisassembled. Without fetch status the bus
// doesn't show where instructions start.
void PinsH8500::printBacktrace() {
    printCycles();
}

}  // namespace h8500
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
