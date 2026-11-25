#include "pins_dcj11.h"
#include "debugger.h"
#include "devs_pdp11.h"
#include "digital_bus.h"
#include "inst_pdp11.h"
#include "mems_pdp11.h"
#include "regs_pdp11.h"
#include "signals_dcj11.h"
#include "watchdog.h"

namespace debugger {
namespace dcj11 {

using pdp11::DevsPdp11;
using pdp11::Kw11l;
using pdp11::InstPdp11;
using pdp11::MemsDcj11;
using pdp11::RegsPdp11;
using pdp11::Rk11;
using pdp11::Rq;

// clang-format off
/**
 * DCJ11 bus cycles (DCJ11 User's Guide, Chapter 3 and Appendix B). The
 * board drives CLK1 with #TEST2 grounded, every edge of it from the
 * debugger: the DCJ11 is static and waits at any of them. An edge is
 * counted from e0, the fall #ALE falls after; the User's Guide's T0 is
 * the rise before it, where #STRB rises, and T1-T7 the rises e1-e13.
 *
 *          e0  e1  e2  e3  e4  e5  e6  e7  e8  e9  e10 e11 e12 e13 e14 e15
 * CLK1  __/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__
 *       ______       ____________________________________________________
 * #ALE        \_____/              the address, AIO and BS until e2
 *       __                                                     __________
 * #STRB   \_____/  \_______________________________________ ... /
 *       ______________          ________________________________________
 * #BUFCTL               \______/        \______ ...   a read; DAL is the
 *                                                     CPU's while high
 *       ___________________________________________
 * #SCTL                                            \____ ...  to 3 edges
 *                                                             after #CONT
 * A NIO ends at e8, the next e0. So does a memory read with #MISS high:
 * its data is latched at e5, T3's rise, off DAL between e3 and e5. Every
 * other cycle stretches: the CPU drives DAL again from e5 to e7, a
 * read's data goes out after that and DV latches it, a write's is valid
 * while #SCTL is low. #CONT is sampled at e8 and every fourth edge after;
 * from the sample S, #SCTL rises at S+3 and the next e0 is S+8.
 *
 * The CPU's own #ABORT, an MMU or address error, falls two edges before
 * e0 and tells the cycle to be ignored; an abort from outside has to
 * come in the stretch.
 */
// clang-format on

namespace {

// The clock's halves are 28ns at least and its period 67ns (DCJ11 User's
// Guide, Table B-1): coded 24, the code between two edges makes them
// 32ns and up on the pin, a period of 72ns and up; coded 28 measured 40
// and 96 in this build, 32 and 80 in another.
constexpr auto lo_ns = 24;
constexpr auto hi_ns = 24;
// Read data's setup to T3's rise or DV, 35ns, and through the TXS0108E.
constexpr auto data_ns = 50;
// Read data's hold from T3's rise, 20ns in the table: coded 20 broke one
// V6 compile in three, 60 passes.
constexpr auto hold_ns = 60;
// #CONT's setup is 30ns.
constexpr auto cont_ns = 40;
// A stretched read's data out and latched by DV at e7, for the first
// #CONT sample at e8: a cycle of eight periods rather than ten.
constexpr auto early_dv = true;
// Pairs of edges #SCTL may take to rise after #CONT.
constexpr auto cont_pairs = 16;
// NIO cycles or periods without any other cycle before a run counts the
// CPU as in WAIT.
constexpr auto idle_cycles = 1024;
// #INIT is held at least 25 periods.
constexpr auto init_periods = 64;
// Cycles of the power-up microroutine before the first fetch.
constexpr auto power_up_cycles = 256;
// Bus cycles allowed for a halt or a step to reach the debugger.
constexpr auto trap_cycles = 256;
// Cycles back from the restart fetch the trap frame's writes may lie.
constexpr auto frame_cycles = 16;
// The 74HCT259's address setup and enable pulse.
constexpr auto latch_ns = 30;

// The power-up configuration (DCJ11 User's Guide, Figure 8-3): POK,
// power-up mode 3 (start at its bits 15-9), and a kernel HALT traps to 4.
// With the MMU off 160000 and up is the I/O page, so the start is in
// memory.
constexpr uint16_t ORG_START = 0001000;
constexpr uint16_t POWER_UP_CONFIG = ORG_START | 0000017;
// General-purpose write codes (DCJ11 User's Guide, Table 3-3).
constexpr uint8_t GP_BUS_RESET = 0014;
constexpr uint8_t GP_EVENT_ACK = 0100;
// The debugger's traps go here; the debugger answers its fetch. Kernel
// I page 0 is always mapped whole, unlike page 7, where 2.11BSD maps its
// overlays (some shorter than 6 KB). Clear of the vectors and of the
// stack below 1000.
constexpr uint16_t ORG_RESTART = 0017000;
// PS's current and previous modes.
constexpr uint16_t PSW_MODE = 0140000;
constexpr uint16_t PSW_PREVIOUS = 0030000;
constexpr uint16_t MOV_ABS_PUSH = 0013746;  // MOV @#addr, -(SP)
constexpr uint16_t BIC_IMM_SP = 0042716;    // BIC #imm, (SP)
constexpr uint16_t BIS_IMM_SP = 0052716;    // BIS #imm, (SP)
constexpr uint16_t RTI = 0000002;
constexpr uint16_t VEC_PWRFAIL = 0000024;
// A HALT outside kernel mode, and in it with this halt option, traps here.
constexpr uint16_t VEC_HALT = 0000004;
// The emulated memory takes all of DAL21-DAL00.
constexpr uint32_t MAX_ADDR = 017777777;
// The maintenance register, a system register the DCJ11 leaves to its
// board: a KDJ11-A module id, which 2.11BSD's boot reads as an 11/73.
constexpr uint16_t MAINT_REG = 0177750;
constexpr uint16_t MAINT_KDJ11A = 0000020;

// The 74HCT259's outputs.
constexpr uint8_t Q_PWRF = 0;
constexpr uint8_t Q_EVENT = 1;
constexpr uint8_t Q_HALT = 2;
constexpr uint8_t Q_IRQ0 = 3;
constexpr uint8_t Q_IRQ1 = 4;
constexpr uint8_t Q_IRQ3 = 6;
constexpr uint8_t Q_MISS = 7;

const uint8_t PINS_HIGH[] = {
        PIN_CLK,
        PIN_INIT,
        PIN_CONT,
        PIN_LSTRB,
        PIN_ASEL1,
        PIN_DV,
};

const uint8_t PINS_LOW[] = {
        PIN_LDATA,
        PIN_SEL2,
        PIN_ASEL0,
};

const uint8_t PINS_INPUT[] = {
        PIN_ALE,
        PIN_SCTL,
        PIN_BUFCTL,
        PIN_MAP,
        PIN_PRDC,
        PIN_STRB,
        PIN_MUX0,
        PIN_MUX1,
        PIN_MUX2,
        PIN_MUX3,
        PIN_DAL0,
        PIN_DAL1,
        PIN_DAL2,
        PIN_DAL3,
        PIN_DAL4,
        PIN_DAL5,
        PIN_DAL6,
        PIN_DAL7,
        PIN_DAL8,
        PIN_DAL9,
        PIN_DAL10,
        PIN_DAL11,
        PIN_DAL12,
        PIN_DAL13,
        PIN_DAL14,
        PIN_DAL15,
};

inline void clk_lo() {
    digitalWriteFast(PIN_CLK, LOW);
}

inline void clk_hi() {
    digitalWriteFast(PIN_CLK, HIGH);
}

// An edge and the half after it.
inline void fall() {
    clk_lo();
    delayNanoseconds(lo_ns);
}

inline void rise() {
    clk_hi();
    delayNanoseconds(hi_ns);
}

void assert_init() {
    digitalWriteFast(PIN_INIT, LOW);
}

void negate_init() {
    digitalWriteFast(PIN_INIT, HIGH);
}

void assert_cont() {
    digitalWriteFast(PIN_CONT, LOW);
}

void negate_cont() {
    digitalWriteFast(PIN_CONT, HIGH);
}

// DV is low active: the read latch is open while it is high and closes
// as it falls (J-11 Data Chip Specification, MDV-L).
void assert_dv() {
    digitalWriteFast(PIN_DV, LOW);
}

void negate_dv() {
    digitalWriteFast(PIN_DV, HIGH);
}

// digitalReadFast() may return the port bit, not 1: compare with LOW.
inline bool ale_asserted() {
    return digitalReadFast(PIN_ALE) == LOW;
}

inline bool sctl_asserted() {
    return digitalReadFast(PIN_SCTL) == LOW;
}

inline bool bufctl_asserted() {
    return digitalReadFast(PIN_BUFCTL) == LOW;
}

// A cycle whose data the debugger drives: a read, IACK or GP read. GP
// and IACK cycles are known by AIO alone; their BS is undefined.
inline bool served_read(const Signals *s) {
    return !s->write() && !s->gpWrite() &&
           (s->iack() || s->gpRead() || !s->internal());
}

inline bool strb_asserted() {
    return digitalReadFast(PIN_STRB) == LOW;
}

inline bool abort_asserted() {
    return digitalReadFast(PIN_ABORT) == LOW;
}

// The debugger's own #ABORT, open-collector as the DCJ11's is: a bus
// timeout, which traps through 4.
void assert_abort() {
    digitalWriteFast(PIN_ABORT, LOW);
    pinMode(PIN_ABORT, OUTPUT);
}

void negate_abort() {
    digitalWriteFast(PIN_ABORT, HIGH);
    pinMode(PIN_ABORT, INPUT_PULLUP);
}

// Whether |s| reads where no device answers: Unix sizes its memory by
// the bus timeout at the I/O page's first words, 760000-760077, and
// 2.11BSD's boot looks for a UNIBUS map at 770200-770376, which a read
// of 0 would turn on.
inline bool bus_timeout(const Signals *s) {
    if (!s->read() || s->fetch() || !s->ioPage())
        return false;
    const auto reg = s->addr & 017777;
    return (reg & 017700) == 0 || (reg & 017600) == 010200;
}

// Sets output |q| of the 259 to |level|, and leaves the 153s at select 2.
void write_latch(uint8_t q, bool level) {
    digitalWriteFast(PIN_ASEL0, (q & 1) ? HIGH : LOW);
    digitalWriteFast(PIN_ASEL1, (q & 2) ? HIGH : LOW);
    digitalWriteFast(PIN_SEL2, (q & 4) ? HIGH : LOW);
    digitalWriteFast(PIN_LDATA, level ? HIGH : LOW);
    delayNanoseconds(latch_ns);
    digitalWriteFast(PIN_LSTRB, LOW);
    delayNanoseconds(latch_ns);
    digitalWriteFast(PIN_LSTRB, HIGH);
    digitalWriteFast(PIN_ASEL0, LOW);
    digitalWriteFast(PIN_ASEL1, HIGH);
}

void assert_pwrf() {
    write_latch(Q_PWRF, LOW);
}

void negate_pwrf() {
    write_latch(Q_PWRF, HIGH);
}

}  // namespace

PinsDcj11::PinsDcj11() {
    _mems = new MemsDcj11(MAX_ADDR);
    _devs = new DevsPdp11(_mems);
    _regs = new RegsPdp11(this, mems<MemsDcj11>(), true, true);
}


void PinsDcj11::resetPins() {
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    // The DCJ11's pull-up alone lets #ABORT glitch through the TXS0108E.
    pinMode(PIN_ABORT, INPUT_PULLUP);

    // The 259 powers up random: set every output while #INIT is low.
    assert_init();
    write_latch(Q_PWRF, HIGH);
    write_latch(Q_EVENT, HIGH);
    write_latch(Q_HALT, LOW);  // HALT is active high
    for (auto q = Q_IRQ0; q <= Q_IRQ3; ++q)
        write_latch(q, LOW);
    write_latch(Q_MISS, HIGH);  // a memory read is not stretched
    for (auto i = 0; i < init_periods; i++) {
        fall();
        rise();
    }
    fall();
    negate_init();
    Cycles::reset();
    _devPending = false;
    _trapVector = 0;

    // GP writes, the configuration read, register writes and memory
    // probes, then the first fetch.
    Signals *s = nullptr;
    for (auto guard = power_up_cycles; guard > 0; --guard) {
        s = prepareCycle();
        if (s && s->fetch() && s->addr16() == ORG_START)
            break;
        if (s)
            completeCycle(s);
    }
    if (s == nullptr || !s->fetch() || s->addr16() != ORG_START) {
        cli.print("?reset: fetch ");
        if (s)
            s->print();
        else
            cli.println();
        return;
    }
    _regs->setIp(ORG_START);
    _regs->save();
    // A reset keeps SP, which after power-up may lie where G's frame and
    // a program's first push would trap.
    const auto regs = this->regs<RegsPdp11>();
    if (regs->kernelStackLow() || regs->stackInIoPage())
        regs->setSp(ORG_START);
}

// From a fall, e0 of the next cycle at the earliest, to e1 of the next
// cycle that isn't NIO, parked there with CLK1 high: the address stays
// until e2. Only at e1 is #ALE low with #STRB high. A NIO's other edges
// need no look. Null when none comes, as in a WAIT.
Signals *PinsDcj11::prepareCycle() {
    for (auto guard = idle_cycles; guard > 0; --guard) {
        rise();  // e1
        if (ale_asserted() && !strb_asserted()) {
            auto s = Signals::put();
            s->clearMark();  // Cycles::next() keeps the slot's old mark
            s->getAddr(abort_asserted());
            if (!s->nio()) {
                _parked = *s;
                return s;
            }
            for (auto e = 2; e < 8; e += 2) {
                fall();
                rise();
            }
        }
        fall();  // e0
    }
    return nullptr;
}

// Takes the parked cycle up again, at e1.
Signals *PinsDcj11::resumeCycle() {
    auto s = Signals::put();
    *s = _parked;
    s->clear();
    return s;
}

// From e1 to the next cycle's e0, a fall.
Signals *PinsDcj11::completeCycle(Signals *s) {
    settleDevRead(s);
    const auto write = s->write() || s->gpWrite();
    const auto read = served_read(s);
    if (read) {
        if (s->iack()) {
            s->data = devs<DevsPdp11>()->vector(s->iackLevel());
        } else if (s->gpRead()) {
            const auto code = s->addr & 0377;
            s->data = (code == 0 || code == 2) ? POWER_UP_CONFIG : 0;
        } else {
            retrieveData(s);
        }
    }
    const auto fast = s->read() && s->memory();
    fall();  // e2: the address leaves
    rise();  // e3
    if (fast) {
        clk_lo();  // e4
        s->outData();
        delayNanoseconds(data_ns);
        // The CPU drives DAL again 20ns after T3's rise.
        noInterrupts();
        clk_hi();  // e5: latched
        delayNanoseconds(hold_ns);
        s->inputMode();
        interrupts();
    } else {
        fall();  // e4
        rise();  // e5
    }
    fall();  // e6
    rise();  // e7: #BUFCTL falls again in a stretch
    const auto timeout = bus_timeout(s);
    // Whether #CONT's first sample, at e8, takes it.
    auto first = !read;
    if (read && !fast && early_dv && !timeout) {
        s->outData();
        delayNanoseconds(data_ns);
        assert_dv();
        first = true;
    } else if (read) {
        fall();  // e8
        if (fast && !bufctl_asserted()) {
            // Done at e7: this is the next cycle's e0.
            Cycles::next();
            return s;
        }
        s->outData();
        delayNanoseconds(data_ns);
        rise();    // e9: #SCTL falls
        clk_lo();  // e10
        if (timeout)
            assert_abort();
        assert_dv();
        delayNanoseconds(lo_ns);
        clk_hi();  // e11
        if (timeout) {
            // The abort is sampled where #CONT is: a sample of its own.
            delayNanoseconds(hi_ns);
            fall();
            rise();
            fall();
            clk_hi();
        }
    }
    // #CONT is held until #SCTL rises, three edges after the sample that
    // took it; a write's data is read in the first low half, #SCTL low.
    assert_cont();
    delayNanoseconds(cont_ns);
    if (first) {
        fall();  // e8: sampled
        rise();  // e9: #SCTL falls
    }
    auto pairs = cont_pairs;
    do {
        clk_lo();
        if (write && pairs == cont_pairs)
            s->getData();
        delayNanoseconds(lo_ns);
        rise();
    } while (sctl_asserted() && --pairs);
    if (pairs == 0) {
        cli.print("?#SCTL ");
        s->print();
    }
    // S+3
    negate_cont();
    negate_dv();
    s->inputMode();
    if (timeout)
        negate_abort();
    fall();
    rise();  // S+5: #BUFCTL rises
    fall();
    rise();  // S+7: #STRB rises
    fall();  // S+8: the next cycle's e0
    if (write && !s->aborted())
        storeData(s);
    if (s->aborted() && Debugger.verbose()) {
        cli.print("?#ABORT ");
        s->print();
    }
    Cycles::next();
    return s;
}

// Reads are whole words; the CPU picks the byte it needs. The system
// registers other than the maintenance register, and the I/O page
// outside its devices, read 0.
void PinsDcj11::retrieveData(Signals *s) {
    if (!s->readMemory())
        return;
    const uint16_t addr = s->addr16() & ~1;
    const auto devs = this->devs<DevsPdp11>();
    if (s->ioPage() && devs->isSelected(addr)) {
        s->data = devs->peek(addr);
        _devRead = addr;
        _devPending = true;
    } else if (s->system() && addr == MAINT_REG) {
        s->data = MAINT_KDJ11A;
    } else if (s->memory()) {
        // The little-endian word in one read.
        s->data = mems<MemsDcj11>()->read_word(s->addr / 2);
    } else {
        s->data = 0;
    }
}

// A byte write drives the byte on the half its address picks.
void PinsDcj11::storeData(Signals *s) {
    if (!s->write() || !s->writeMemory()) {
        if (!s->gpWrite())
            return;
        const auto code = s->addr & 0377;
        if (code == GP_BUS_RESET) {
            // A RESET instruction, and the power-up microroutine.
            _devs->reset();
        } else if (code == GP_EVENT_ACK) {
            devs<DevsPdp11>()->acknowledgeClock();
        }
        return;
    }
    if (s->ioPage()) {
        const uint16_t addr = s->addr16();
        const auto devs = this->devs<DevsPdp11>();
        if (!devs->isSelected(addr & ~1))
            return;
        if (s->byteWrite()) {
            devs->writeByte(addr, s->writeLow() ? lo(s->data) : hi(s->data));
        } else {
            devs->write(addr, s->data);
        }
        return;
    }
    if (!s->memory())
        return;
    const uint32_t at = s->addr & ~1;
    if (!s->byteWrite()) {
        _mems->write16(at, s->data);
    } else if (s->writeLow()) {
        _mems->write_byte(at, lo(s->data));
    } else {
        _mems->write_byte(at | 1, hi(s->data));
    }
}

// A device read takes effect unless the cycle after it writes the same
// word: that read was a read-modify-write's.
void PinsDcj11::settleDevRead(const Signals *s) {
    if (!_devPending)
        return;
    _devPending = false;
    if (s->write() && (s->addr16() & ~1) == _devRead)
        return;
    _devs->read(_devRead);
}

void PinsDcj11::execute(const uint16_t *inst, uint_fast8_t words,
        uint16_t *buf, uint_fast8_t max, uint16_t &org, uint32_t exit,
        const uint16_t *stack, uint16_t stackAt, uint_fast8_t stackWords) {
    const uint16_t bytes = 2 * words;
    uint_fast8_t cap = 0;
    // The exit counts only once the sequence's last word was read: it
    // may lie inside the sequence (save() returns to its own start).
    bool whole = false;
    auto s = resumeCycle();
    // Bound the damage: a wrong dump beats a hung debugger.
    for (auto guard = Cycles::MAX_CYCLES; s && guard > 0; --guard) {
        const uint16_t addr = s->addr16();
        const uint16_t off = addr - org;
        // A prefetch past the sequence may come before its last
        // instruction is done; only a demand read ends it.
        if (s->fetch() && whole && cap >= max &&
                (addr == exit ||
                        (exit == EXIT_END && off >= bytes && !s->request())))
            break;
        if (s->read()) {
            const uint16_t at = addr - stackAt;
            if (off < bytes) {
                s->inject(inst[off / 2]);
                if (off / 2 + 1u == words)
                    whole = true;
            } else if (stack && at < 2u * stackWords) {
                s->inject(stack[at / 2]);
            }
        } else if (s->write() && cap < max) {
            s->capture();
        }
        const auto capturing = s->write() && !s->writeMemory();
        completeCycle(s);
        if (capturing && buf) {
            buf[cap] = s->writeHigh() ? hi(s->data) : s->data;
            ++cap;
        }
        s = prepareCycle();
    }
    // Parked in it; hand the caller where that is.
    if (s)
        org = s->addr16();
}

void PinsDcj11::execInst(const uint16_t *inst, uint_fast8_t words,
        uint16_t &org, uint32_t exit, const uint16_t *stack, uint16_t stackAt,
        uint_fast8_t stackWords) {
    execute(inst, words, nullptr, 0, org, exit, stack, stackAt, stackWords);
}

void PinsDcj11::captureWrites(const uint16_t *inst, uint_fast8_t words,
        uint16_t *buf, uint_fast8_t max, uint16_t &org) {
    execute(inst, words, buf, max, org, org, nullptr, 0, 0);
}

// Whether the BPT whose vector |s| reads is the debugger's: one at a
// break point, its fetch or the prefetch after it just before |s|, or
// any while the vector points at itself, as a program that halts to the
// debugger leaves it.
bool PinsDcj11::breakTrap(const Signals *s) const {
    if (_mems->read(InstPdp11::VEC_BPT) == InstPdp11::VEC_BPT)
        return true;
    const auto back = Cycles::cycles() < 2U ? Cycles::cycles() : 2U;
    for (auto i = 1U; i <= back; ++i) {
        const auto w = s->prev(i);
        if (!w->fetch())
            break;
        if (w->data == InstPdp11::BPT && isBreakPoint(w->addr16()))
            return true;
    }
    return false;
}

// Whether the trap whose vector |s| reads follows a HALT: its fetch, or
// the prefetch after it, just before |s|, past the power-up
// configuration read the HALT makes. Unlike the other vectors this one
// is watched through a whole run, so its whole bus address must match:
// kernel D page 0 at 0, as V6 and 2.11BSD map it.
bool PinsDcj11::haltTrap(const Signals *s) const {
    const auto back = Cycles::cycles() < 3U ? Cycles::cycles() : 3U;
    for (auto i = 1U; i <= back; ++i) {
        const auto w = s->prev(i);
        if (w->gpRead())
            continue;
        if (!w->fetch())
            break;
        if (w->data == InstPdp11::HALT)
            return true;
    }
    return false;
}

// Whether |s|, a prepared cycle, enters the debugger. BPT and a T-bit
// trace read the vector at 14, #PWRF the one at 24, a HALT the one at 4:
// the debugger answers with the restart address and priority 7, and
// parks at its fetch, which the memory management may have put anywhere
// on the bus. The PC and PS the trap pushed are the two latest writes.
// A BPT's PC is its own unless |step| ran it. A program reads those
// words as data too, and may handle BPT itself: the vector at 14 is
// answered only in a |step| or for the debugger's BPT, the one at 24
// only while |halting| holds #PWRF. Only a kernel's HALT stops; one in
// another mode goes on through the program's vector at 4.
bool PinsDcj11::parkAfterFrame(Signals *&s, bool step, bool halting) {
    if (s->read() && !s->fetch() && s->memory()) {
        const uint16_t addr = s->addr16();
        if ((addr == InstPdp11::VEC_BPT && (step || breakTrap(s))) ||
                (addr == VEC_PWRFAIL && halting) ||
                (s->addr == VEC_HALT && haltTrap(s))) {
            s->inject(ORG_RESTART);
            _trapVector = addr;
            // Still asserted, #PWRF would trap again before the fetch.
            if (addr == VEC_PWRFAIL)
                negate_pwrf();
        } else if (_trapVector && addr == _trapVector + 2) {
            s->inject(InstPdp11::PSW_PRI7);
        }
        return false;
    }
    if (_trapVector == 0 || !s->fetch())
        return false;
    const auto vector = _trapVector;
    _trapVector = 0;
    const Signals *frame[2] = {nullptr, nullptr};
    const Signals *first = nullptr;
    const auto back = Cycles::cycles() < frame_cycles ? Cycles::cycles()
                                                      : frame_cycles;
    for (auto i = 1U; i <= back; ++i) {
        const auto w = s->prev(i);
        if (w->read() && w->addr16() == vector)
            first = w;
        if (w->write() && w->memory() && frame[1] == nullptr) {
            frame[frame[0] ? 1 : 0] = w;
            first = w;
        }
    }
    if (frame[1] == nullptr)
        return false;
    // The PC lies below the PS, by the 16-bit addresses: the debugger runs
    // with the MMU off.
    const auto pc =
            frame[0]->addr16() < frame[1]->addr16() ? frame[0] : frame[1];
    const auto ps = pc == frame[0] ? frame[1] : frame[0];
    if (uint16_t(pc->addr16() + 2) != ps->addr16())
        return false;
    const uint16_t at =
            (vector == InstPdp11::VEC_BPT && !step) ? pc->data - 2 : pc->data;
    if (vector == VEC_HALT && (ps->data & PSW_MODE) != 0) {
        passTrap(s, vector, ps->data);
        return false;
    }
    // A trace trap pushed the T bit the step set.
    const uint16_t psw = ps->data & ~InstPdp11::PSW_T;
    Cycles::discard(first);
    regs<RegsPdp11>()->parkInTrap(at, psw, ORG_RESTART, s->addr16());
    return true;
}

// Ends the trap through |vector| the debugger took, from the restart
// fetch |s|, as the CPU would have: the program's PC and PS from the
// vector, the previous mode from |ps|, the one the trap pushed. The
// vector is read, and the frame pushed, through the kernel's mapping,
// which haltTrap() found putting kernel D page 0 at 0. |s| is then the
// handler's first fetch.
void PinsDcj11::passTrap(Signals *&s, uint16_t vector, uint16_t ps) {
    const uint16_t previous = (ps >> 2) & PSW_PREVIOUS;
    const uint16_t seq[] = {
            MOV_ABS_PUSH, uint16_t(vector + 2),  // MOV @#vector+2, -(SP)
            BIC_IMM_SP, PSW_PREVIOUS,            // BIC #PSW_PREVIOUS, (SP)
            BIS_IMM_SP, previous,                // BIS #previous, (SP)
            MOV_ABS_PUSH, vector,                // MOV @#vector, -(SP)
            RTI,
    };
    uint16_t org = s->addr16();
    execInst(seq, sizeof(seq) / sizeof(seq[0]), org);
    s = resumeCycle();
}

// Raises #PWRF on |s|, a prepared cycle, or on a CPU in WAIT when null:
// the instruction finishes, then the trap through 24.
bool PinsDcj11::suspend(Signals *s) {
    assert_pwrf();
    for (auto guard = trap_cycles; guard > 0; --guard) {
        if (s) {
            if (parkAfterFrame(s, false, true))
                return true;
            completeCycle(s);
        }
        s = prepareCycle();
    }
    negate_pwrf();
    _trapVector = 0;
    return false;
}

bool PinsDcj11::rawStep() {
    // A BPT or HALT traps with the trace trap still pending.
    const auto regs = this->regs<RegsPdp11>();
    const auto inst = _mems->read(regs->nextIp());
    if (inst == InstPdp11::BPT || inst == InstPdp11::HALT)
        return false;
    auto s = resumeCycle();
    for (auto guard = trap_cycles; guard > 0; --guard) {
        completeCycle(s);
        s = prepareCycle();
        if (s == nullptr)
            break;  // in WAIT
        if (parkAfterFrame(s, true, false))
            return true;
    }
    return suspend(s);
}

bool PinsDcj11::step(bool show) {
    Cycles::reset();
    auto regs = this->regs<RegsPdp11>();
    // The step's trace trap pushes a frame; SP is left as the program has
    // it, which after power-up may be anywhere.
    if (regs->kernelStackLow()) {
        cli.println("?step: SP below 000400");
        return false;
    }
    regs->setTrace(true);
    _regs->restore();
    regs->setTrace(false);
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

// Runs until a BPT or the halt switch. Interrupts stay on: one only
// lengthens a half. Returns whether the registers can be saved.
bool PinsDcj11::loop() {
    Signals *s = resumeCycle();
    for (uint_fast8_t n = 0;; ++n) {
        // Quiet, as in a WAIT.
        const auto quiet = s == nullptr;
        if (quiet)
            Watchdog::feed();
        if (s && parkAfterFrame(s, false, false))
            return true;
        // The devices' slow service comes round on its own clock; its
        // check costs a cycle a hundred ns, so not every one.
        const auto serviced = ((n & 15) == 0 || quiet) && _devs->loop();
        if ((serviced || quiet) && haltSwitch())
            return suspend(s);
        if (s)
            completeCycle(s);
        s = prepareCycle();
    }
}

void PinsDcj11::run() {
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

void PinsDcj11::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, InstPdp11::BPT);
}

// The line clock's is #EVENT, level 6 through vector 100, and the disk's
// IRQ1, level 5. Every other request is IRQ0, level 4. An IRQ's vector is
// served at the IACK.
namespace {
// The disks share IRQ1, level 5; the console has IRQ0.
inline bool level5(uint8_t name) {
    return name == Rk11::VECTOR || name == Rq::VECTOR;
}
}  // namespace

void PinsDcj11::assertInt(uint8_t name) {
    if (name == Kw11l::VECTOR) {
        write_latch(Q_EVENT, LOW);
    } else {
        write_latch(level5(name) ? Q_IRQ1 : Q_IRQ0, HIGH);
    }
}

// A shared line stays up while the other device still requests.
void PinsDcj11::negateInt(uint8_t name) {
    if (name == Kw11l::VECTOR) {
        write_latch(Q_EVENT, HIGH);
    } else if (level5(name)) {
        if (!devs<DevsPdp11>()->requesting(5))
            write_latch(Q_IRQ1, LOW);
    } else {
        write_latch(Q_IRQ0, LOW);
    }
}

void PinsDcj11::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

// Discarded prefetches and operand words are fetch()es too, so the
// instructions are counted as printBacktrace() walks them.
const SignalsImpl *PinsDcj11::findBacktraceStart() {
    const auto g = Signals::get();
    const auto insns = walkInsns(UINT32_MAX);
    if (insns <= _lineLimit)
        return g;
    return g->next(walkInsns(insns - _lineLimit));
}

// The cycle the |nth| instruction printBacktrace() shows starts at, or
// with no such one, how many there are.
uint32_t PinsDcj11::walkInsns(uint32_t nth) const {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    const auto mems = this->mems<MemsDcj11>();
    uint32_t from = 0, to = 0, insns = 0;
    for (auto i = 0u; i < cycles; ++i) {
        const auto s = g->next(i);
        const uint16_t addr = s->addr16();
        if (s->fetch() && (addr < from || addr >= to) && !discarded(i)) {
            if (insns++ == nth)
                return i;
            from = addr;
            to = addr + mems->insnLength(addr);
        }
    }
    return insns;
}

// An IREQ the CPU threw away: a demand fetch comes next, as after a
// taken branch, JMP, JSR, RTS or SOB, or none does, as the prefetch past
// the stop PC.
bool PinsDcj11::discarded(uint32_t i) const {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    if (!g->next(i)->request())
        return false;
    while (++i < cycles) {
        const auto s = g->next(i);
        if (s->fetch())
            return !s->request();
    }
    return true;
}

// One line per instruction, then its data cycles; its operand words only
// when verbose.
void PinsDcj11::printBacktrace() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    uint32_t from = 0, to = 0;  // the last instruction's bytes
    for (auto i = 0u; i < cycles; ++i) {
        const auto s = g->next(i);
        const uint16_t addr = s->addr16();
        const auto outside = addr < from || addr >= to;
        if (s->fetch() && outside && !discarded(i)) {
            if (Debugger.verbose())
                s->print();
            from = addr;
            to = _mems->disassemble(addr, 1);
        } else if (!s->read() || (outside && !s->fetch()) ||
                   Debugger.verbose()) {
            s->print();
        }
        idle();
    }
}

}  // namespace dcj11
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
