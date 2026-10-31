#include "pins_h8300.h"
#include "debugger.h"
#include "devs_h8300.h"
#include "digital_bus.h"
#include "inst_h8300.h"
#include "mems_h8300.h"
#include "regs_h8300.h"
#include "signals_h8300.h"

namespace debugger {
namespace h8300 {

// clang-format off
/**
 * H8/300 external access cycle (H8/325 Series Hardware Manual 3.7.2 and
 * Figure 15-5). The system clock ø is EXTAL/2, one state a ø period:
 *
 *               T1      T2      Tw      T3      T1
 *            ___     ___     ___     ___     ___
 *     ø   __/   \___/   \___/   \___/   \___/   \__
 *         ______                          ________
 *    #AS        \________________________/
 *    #RD        \________________________/
 *    A0-A15 ---<       address              >-----
 *    D0-D7  ----------------------< read  >-------
 *         ______________                  ________
 *    #WR                \________________/
 *                            ^       ^
 *                            #WAIT sampled
 *
 * #AS and #RD assert in T1 and the address holds through T3; #WR asserts
 * in T2 with the data. #WAIT is sampled at the ø fall in the middle of T2
 * and of every Tw, and inserts Tw while it is low. Mode 1 has no wait
 * controller: #WAIT is live from reset. The on-chip register field is
 * accessed with #AS, #RD and #WR high, so it never shows here; nor does
 * the on-chip RAM, which the setup disables. A word moves as two byte
 * cycles, the even address first.
 *
 * EXTAL runs free from FlexPWM3: the chip is dynamic, ø no slower than
 * 2000ns. #WAIT stays asserted, so every cycle waits in Tw, and is negated
 * only until #AS negates at the end of T3.
 */
// clang-format on

namespace {

// ATTENTION: unbenched. The EXTAL rate a run, and everything else, uses:
// ø 2 MHz.
constexpr uint32_t clock_input_hz = 4000000;
// EXTAL's range: ø 0.5-10 MHz for the HD6413238F10 (Table 15-6).
constexpr uint32_t extal_min_hz = 1000000;
constexpr uint32_t extal_max_hz = 20000000;
// EXTAL's high and low times at least this: half the 50ns EXTAL period
// that the 100ns tcyc minimum makes.
constexpr auto extal_half_min_ns = 25;
// #RES low this long resets: 20ms at power-on (4.2.2).
constexpr auto reset_us = 20000;
// No #AS for this many EXTAL clocks: the CPU sleeps or runs from on-chip
// memory. Long enough to clear DIVXU and an interrupt's entry.
constexpr auto halt_clocks = 4096;
// EXTAL clocks between polls of a CPU already stopped: short enough to
// service the devices and the halt switch often.
constexpr auto wait_poll_clocks = 64;
// Bus cycles between polls of the halt switch in a run.
constexpr auto halt_poll_cycles = 16;
// EXTAL clocks a cycle takes, at most, to show its write strobe, and to
// end once #WAIT is negated: T2, a Tw, T3 and slack.
constexpr auto strobe_clocks = 16;
// Write data follows #WR by up to 25ns (tWDD - tWSD at ø 10 MHz), and
// more at slower clocks: read it this long after #WR.
// ATTENTION: check in a capture at the slowest ø.
constexpr auto write_data_ns = 75;
// Bus cycles allowed for an NMI to be taken; generous.
constexpr auto nmi_ack_cycles = 1024;
// Cycles before an exception's vector read that may hold its frame: PC
// and CCR, four bytes, and the prefetch before them.
constexpr auto frame_lookback = 8;
// The prefetch past an injected sequence's last byte is fed NOPs: the CPU
// fetches one word ahead (H8/300H Software Manual Table 2.10, the bus
// cycles the H8/300 shares).
constexpr auto prefetch_bytes = 2;
// NOPs fed at the park origin after a break, until its NMI is taken.
constexpr auto park_nops = 16;

const uint8_t PINS_LOW[] = {
        PIN_WAIT,
        PIN_RES,
};

const uint8_t PINS_HIGH[] = {
        PIN_NMI,
        PIN_IRQ0,
        PIN_IRQ1,
        PIN_IRQ2,
        PIN_RXD,
};

const uint8_t PINS_INPUT[] = {
        PIN_AS,
        PIN_RD,
        PIN_WR,
        PIN_IOS,
        PIN_PHI,
        PIN_TXD,
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
        PIN_AH8,
        PIN_AH9,
        PIN_AH10,
        PIN_AH11,
        PIN_AH12,
        PIN_AH13,
        PIN_AH14,
        PIN_AH15,
};

inline void negate_reset() {
    digitalWriteFast(PIN_RES, HIGH);
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

}  // namespace

inline bool Signals::getControl() {
    cntl() = busRead(CNTL);
    return (cntl() & AS) == 0;
}

inline bool Signals::getStrobes() {
    strobes() = busRead(CNTL);
    return (strobes() & (RD | WR)) != (RD | WR);
}

PinsH8300::PinsH8300() : _clock(extal_half_min_ns, extal_min_hz) {
    _devs = new DevsH8300();
    _mems = new MemsH8300();
    _regs = new RegsH8300(this, mems<MemsH8300>());
    _clockInput = clock_input_hz;
    _clockCycles = F_CPU / _clockInput;
}

bool PinsH8300::setClockInput(uint32_t hz) {
    if (hz > extal_max_hz)
        return false;
    const auto rate = _clock.setRate(hz);
    if (rate == 0)
        return false;
    _clockInput = rate;
    _clockCycles = F_CPU / rate;
    devs<DevsH8300>()->sciClock(rate / 2);
    return true;
}

void PinsH8300::resetPins() {
    // #RES and #WAIT low first: the first cycle, the reset vector's, waits
    // for the Teensy.
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    // The clock runs from here on; nothing stops it.
    _clock.begin(_clockInput);
    _clock.start();
    devs<DevsH8300>()->sciClock(_clockInput / 2);
    delayMicroseconds(reset_us);

    Cycles::reset();
    negate_reset();
    // The reset vector, a word at H'0000 (Table 4-2).
    const auto s = prepareCycle();
    // Anything but a read at H'0000 means the board is not what the
    // driver assumes.
    if (s->halt() || !s->read() || s->addr != InstH8300::VEC_RESET) {
        cli.print("?reset: first cycle ");
        s->print();
        return;
    }
    static constexpr uint8_t VECTOR[] = {
            uint8_t(InstH8300::ORG_PARK >> 8),
            uint8_t(InstH8300::ORG_PARK),
    };
    uint16_t org = InstH8300::VEC_RESET;
    execInst(VECTOR, sizeof(VECTOR), org, InstH8300::ORG_PARK);
    if (_cutShort) {
        cli.println("?reset: no fetch from the reset vector");
        printCycles();
        return;
    }
    _regs->setIp(org);
    if (!setupBus()) {
        cli.println("?reset: setup sequence cut short");
        printCycles();
        return;
    }
    _regs->save();
    _regs->reset();
}

// RAME cleared in SYSCR (H'FFC4 bit 0, 2.4.2) moves the on-chip RAM
// off-chip, so a stack there shows on the bus. SP gets the park stack
// until a program sets its own: R7 is undetermined at reset, and an NMI
// needs a stack it can be seen pushing to. Interrupts are masked from
// reset, and none is taken right after its exception handling.
bool PinsH8300::setupBus() {
    // clang-format off
    static constexpr uint8_t SEQ[] = {
        0x7F, 0xC4, 0x72, 0x00,  // BCLR  #0, @H'FFC4:8
        0x79, 0x07, uint8_t(InstH8300::STACK_PARK >> 8),
        uint8_t(InstH8300::STACK_PARK),  // MOV.W #sp, R7
        0x40, 0xF6,  // BRA   org
    };
    // clang-format on
    static_assert(SEQ[sizeof(SEQ) - 1] == uint8_t(-sizeof(SEQ)),
            "BRA org: the displacement is from the end of the BRA");
    auto org = regs<RegsH8300>()->parkedAt();
    execInst(SEQ, sizeof(SEQ), org, EXIT_ORG);
    _regs->setIp(org);
    return !_cutShort;
}

Signals *PinsH8300::prepareCycle() {
    return prepareCycle(halt_clocks);
}

// Wait for #AS and read the address, then for a strobe: #RD asserts with
// #AS, #WR in T2.
Signals *PinsH8300::prepareCycle(uint_fast16_t clocks) {
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
Signals *PinsH8300::resumeCycle(uint16_t addr) {
    auto s = Signals::put();
    s->clearMark();
    s->getControl();
    s->getAddr();
    s->getStrobes();
    s->addr = addr;
    return s;
}

// The CPU waits in Tw: serve the cycle, then negate #WAIT until #AS
// negates at the end of T3 and assert it again, well before the next
// cycle's T2 fall. The address bus is not multiplexed, so read data goes
// out at once. Masked: a USB interrupt here would let the next cycle
// through unserved.
Signals *PinsH8300::completeCycle(Signals *s) {
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
        delayNanoseconds(write_data_ns);
        s->getData();
        storeWrite(s);
    }
    const uint32_t limit = strobe_clocks * _clockCycles;
    noInterrupts();
    // ATTENTION: #WAIT changes with no regard to ø; check in a capture
    // whether it ever lands at a sampling ø fall.
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
    interrupts();
    Cycles::next();
    return s;
}

void PinsH8300::serveRead(Signals *s) {
    if (!s->readMemory())
        return;
    s->data = _devs->isSelected(s->addr) ? _devs->read(s->addr)
                                         : _mems->read(s->addr);
}

void PinsH8300::storeWrite(Signals *s) {
    if (!s->writeMemory())
        return;
    if (_devs->isSelected(s->addr)) {
        _devs->write(s->addr, s->data);
    } else {
        _mems->write(s->addr, s->data);
    }
}

namespace {
// The byte at |addr| of |inst| placed at |org|. Outside the window it is
// a NOP, so a prefetch past the end runs nothing.
uint8_t instByte(
        const uint8_t *inst, uint_fast8_t len, uint16_t org, uint16_t addr) {
    const uint16_t off = addr - org;
    if (off < len)
        return inst[off];
    return InstH8300::NOP;
}
}  // namespace

uint_fast8_t PinsH8300::execute(const uint8_t *inst, uint_fast8_t len,
        uint8_t *buf, uint_fast8_t max, uint16_t &org, uint32_t exit) {
    uint_fast8_t cap = 0;
    // |inst| is answered by address. |exit| is where the sequence ends --
    // a prefetch past the window looks just like falling through, so the
    // caller must say.
    const uint16_t leaves = exit == EXIT_ORG ? org : exit;
    const uint16_t end = org + len;
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
        if (started && cap >= max && fetchedAll && reading &&
                s->addr == leaves) {
            exited = true;
            break;
        }
        if (reading)
            started = true;
        const uint16_t off = s->addr - org;
        const auto inWindow = reading && off < len;
        if (inWindow && off == len - 1)
            fetchedAll = true;
        // With no fetch status on the bus, a read just past the window is
        // taken for prefetch and gets NOPs; other reads outside it come
        // from memory.
        const uint16_t past = s->addr - end;
        const auto prefetch = reading && past < prefetch_bytes;
        if (inWindow || prefetch) {
            s->inject(instByte(inst, len, org, s->addr));
        } else if (s->write()) {
            s->capture();
        }
        completeCycle(s);
        if (s->write() && cap < max) {
            if (buf)
                buf[cap] = s->data;
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

void PinsH8300::execInst(
        const uint8_t *inst, uint_fast8_t len, uint16_t &org, uint32_t exit) {
    execute(inst, len, nullptr, 0, org, exit);
}

uint_fast8_t PinsH8300::captureWrites(const uint8_t *inst, uint_fast8_t len,
        uint8_t *buf, uint_fast8_t max, uint16_t &org, uint32_t exit) {
    return execute(inst, len, buf, max, org, exit);
}

bool PinsH8300::answerVector(uint16_t vecAddr, uint16_t handler) {
    const uint8_t vector[] = {uint8_t(handler >> 8), uint8_t(handler)};
    auto org = vecAddr;
    execute(vector, sizeof(vector), nullptr, 0, org, handler);
    return !_cutShort;
}

// The frame an interrupt stacked shortly before its vector read |s|: the
// PC at SP-2, then CCR at SP-4 (Figure 4-5), each word high byte first.
// |from| becomes the first of the writes.
bool PinsH8300::findFrame(
        const Signals *s, RegsH8300::Frame &frame, const Signals *&from) const {
    const Signals *w[4];
    uint_fast8_t n = 0;
    for (auto i = 1; i <= frame_lookback && n < 4; ++i) {
        const auto t = s->prev(i);
        if (t->write())
            w[3 - n++] = t;
    }
    if (n < 4)
        return false;
    const auto pch = w[0];
    const uint16_t sp = pch->addr + 2;
    if (w[1]->addr != uint16_t(sp - 1) || w[2]->addr != uint16_t(sp - 4) ||
            w[3]->addr != uint16_t(sp - 3))
        return false;
    frame.pc = uint16(pch->data, w[1]->data);
    frame.ccr = w[2]->data;
    frame.sp = sp;
    from = pch;
    return true;
}

// Stop at an instruction boundary: NMI is taken there and the CPU parks in
// its vector read with the frame stacked. With |org| UINT32_MAX the CPU
// has no parked cycle, asleep, and NMI wakes it.
bool PinsH8300::suspend(uint32_t org) {
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
        if (s->read() && s->addr == InstH8300::VEC_NMI) {
            RegsH8300::Frame frame;
            const Signals *from = nullptr;
            if (findFrame(s, frame, from)) {
                negate_nmi();
                // The frame and this read are the debugger's.
                Cycles::discard(from);
                regs<RegsH8300>()->parkInVector(s->addr, frame);
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

// One instruction: NMI asserted once the instruction at the PC has begun
// is taken at its end. The restore's JMP fetches the PC's word, so the
// instruction's own cycles start with the third.
// ATTENTION: unverified that NMI asserted there is taken after this very
// instruction. An LDC, ANDC, ORC or XORC defers it past the next one
// (H8/300H Software Manual Table 3.1).
bool PinsH8300::step(bool show) {
    const auto pc = _regs->nextIp();
    if (_mems->read(pc) == InstH8300::BREAK &&
            _mems->read(pc + 1) == InstH8300::HALT_VEC) {
        // The halt is left unexecuted.
        return false;
    }
    Cycles::reset();
    _regs->restore();
    if (_cutShort) {
        cli.println("?step: restore cut short");
        return false;
    }
    if (show)
        Cycles::reset();
    auto s = resumeCycle(regs<RegsH8300>()->parkedAt());
    auto nmi = false;
    for (auto n = 0; n < nmi_ack_cycles; ++n) {
        if (n == 2 && !nmi) {
            assert_nmi();
            nmi = true;
        }
        if (s->halt()) {
            // SLEEP: NMI wakes it.
            if (!nmi) {
                assert_nmi();
                nmi = true;
            }
            s = prepareCycle();
            continue;
        }
        if (nmi && s->read() && s->addr == InstH8300::VEC_NMI) {
            RegsH8300::Frame frame;
            const Signals *from = nullptr;
            if (findFrame(s, frame, from)) {
                negate_nmi();
                Cycles::discard(from);
                regs<RegsH8300>()->parkInVector(s->addr, frame);
                Cycles::Hold hold;  // keep the step's cycles for printCycles()
                _regs->save();
                if (show)
                    printCycles();
                return true;
            }
        }
        completeCycle(s);
        s = prepareCycle();
    }
    negate_nmi();
    cli.println("?step: no NMI vector read");
    return false;
}

// A word read in H'0000-H'00FF right after the prefetch past a JSR
// @@aa:8 in memory whose aa names it: a breakpoint, which patches only
// the opcode byte since the breakpoints save one byte each, or the halt
// a program ends with, JSR @@H'04. |pc| becomes the JSR's address.
bool PinsH8300::isBreak(const Signals *s, uint16_t &pc) const {
    if (!s->read() || s->addr > 0xFF)
        return false;
    const auto next = s->prev(2);
    if (!next->read() || !s->prev()->read() ||
            s->prev()->addr != uint16_t(next->addr + 1))
        return false;
    pc = next->addr - InstH8300::BREAK_LEN;
    // A word read ignores the address's lowest bit.
    if (_mems->read(pc) != InstH8300::BREAK ||
            (_mems->read(pc + 1) & ~1) != s->addr)
        return false;
    return s->addr == InstH8300::VEC_HALT || isBreakPoint(pc);
}

// A JSR leaves the interrupt mask as the program had it, so a pending
// interrupt could take the save's first fetch. NMI outranks them all:
// asserted as the JSR reads its vector at |vecAddr|, which is answered
// with the park origin, it is taken at the JSR's end and masks everything. The
// JSR's push gives SP, the NMI's frame CCR; the JSR's return address and the
// NMI's PC are the debugger's.
// ATTENTION: that NMI is taken by that boundary is unverified.
bool PinsH8300::maskBreak(Signals *s, uint16_t pc) {
    const auto vecAddr = s->addr;
    static constexpr uint8_t VECTOR[] = {
            uint8_t(InstH8300::ORG_PARK >> 8),
            uint8_t(InstH8300::ORG_PARK),
    };
    assert_nmi();
    const Signals *push = nullptr;
    auto guard = nmi_ack_cycles;
    while (guard-- && !s->halt()) {
        if (s->read() && s->addr == InstH8300::VEC_NMI) {
            RegsH8300::Frame frame;
            const Signals *from = nullptr;
            if (push && findFrame(s, frame, from)) {
                negate_nmi();
                frame.pc = pc;
                frame.sp = push->addr + 2;
                regs<RegsH8300>()->parkInVector(s->addr, frame);
                return true;
            }
        }
        const uint16_t vec = s->addr - vecAddr;
        const uint16_t park = s->addr - InstH8300::ORG_PARK;
        if (s->read() && vec < sizeof(VECTOR)) {
            s->inject(VECTOR[vec]);
        } else if (s->read() && park < park_nops) {
            // NOPs at the park origin until NMI is taken.
            s->inject(InstH8300::NOP);
        } else if (s->write()) {
            s->capture();
            // The JSR's push, the PC high byte first.
            if (push == nullptr)
                push = s;
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
bool PinsH8300::loop() {
    auto s = resumeCycle(regs<RegsH8300>()->parkedAt());
    uint_fast8_t poll = 0;
    while (true) {
        uint16_t pc;
        if (isBreak(s, pc)) {
            // The JSR's prefetch and everything after are the debugger's.
            const auto from = s->prev(2);
            const auto parked = maskBreak(s, pc);
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

void PinsH8300::run() {
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

void PinsH8300::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, InstH8300::BREAK);
}

namespace {
uint8_t irqPin(uint8_t name) {
    switch (name) {
    case INTR_IRQ0:
        return PIN_IRQ0;
    case INTR_IRQ1:
        return PIN_IRQ1;
    case INTR_IRQ2:
        return PIN_IRQ2;
    default:
        return 0xFF;
    }
}
}  // namespace

// #IRQ0-#IRQ2 are low-level sensed from reset (ISCR); a program enables
// each in IER.
void PinsH8300::assertInt(uint8_t name) {
    const auto pin = irqPin(name);
    if (pin != 0xFF)
        digitalWriteFast(pin, LOW);
}

void PinsH8300::negateInt(uint8_t name) {
    const auto pin = irqPin(name);
    if (pin != 0xFF)
        digitalWriteFast(pin, HIGH);
}

void PinsH8300::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

// ATTENTION: every cycle, undisassembled. Without fetch status the bus
// doesn't show where instructions start.
void PinsH8300::printBacktrace() {
    printCycles();
}

}  // namespace h8300
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
