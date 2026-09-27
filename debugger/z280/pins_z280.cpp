#include "pins_z280.h"
#include "debugger.h"
#include "devs_z280.h"
#include "inst_z280.h"
#include "mems_z280.h"
#include "regs_z280.h"
#include "signals_z280.h"

namespace debugger {
namespace z280 {

// clang-format off
/**
 * Z280 Z-BUS memory cycle (Z280 Technical Manual, Chapter 13).
 *
 * This board drives #XTALI and only observes #CLK. Measured on a
 * real part, #CLK is #XTALI scaled by the Bus Clock Select field:
 * 1/1 passes it straight through, 1/2 halves it, and while #RESET
 * is asserted it runs at #XTALI/4 until the first bus transaction.
 * resetPins() selects 1/1, so one #XTALI cycle is one #CLK cycle,
 * hence one T state. (Chapter 13 describes the scaling as relative
 * to a processor clock of #XTALI/2; the bus clock does not follow
 * that divider.)
 *
 *       |_____|     |_____|     |_____|     |_____|     |
 * XTALI |     |_____|     |_____|     |_____|     |_____|
 *        |_____|     |_____|     |_____|     |_____|     |
 * CLK    |     |_____|     |_____|     |_____|     |_____|
 *       ^^ ~16ns #XTALI to #CLK; a sample must sit past it
 *
 *      |      T1       |      T2       |      T3       |
 *      |_______|       |_______|       |_______|       |
 * CLK  |       |_______|       |_______|       |_______|
 *      |_|       |______________________________________
 * AS   | |_______|
 *      |_____________________|                         |
 * DS   |                     |_________________________|
 *                                              ^ #WAIT sampled here
 *
 * Rising edge of #AS latches address, status, #R//W and B//W. #DS
 * falls in the first half of T2 for reads, the second half for
 * writes, and rises at the T3/T1 boundary once data is sampled.
 * Halt and Refresh never strobe #DS; I/O and Interrupt-Acknowledge
 * add one automatic wait cycle before #DS falls.
 */
// clang-format on

namespace {

// Only #XTALI is driven; the protocol is counted in its phases. #AS is
// asserted one phase, then #DS falls one later on a read and two on a
// write. Every caller puts its own work in the high phase, so this sets
// the low one only: captured over a run the clock sat at 280ns with 89%
// of it high, leaving barely 30ns low, and the low half is the one with
// nothing in it to stretch it.
constexpr auto xtali_lo_ns = 20;
// Reset is clocked slower; the latch below is unreliable at that rate.
constexpr auto reset_lo_ns = 100;
constexpr auto reset_hi_ns = 100;
// How many times to redo the reset before giving up on the latch.
constexpr auto reset_retries = 8;
// Everything the CPU drives is anchored to the #XTALI edges we make, with
// one propagation delay: measured, #CLK rises and #AS falls 16ns after the
// #XTALI rise, and #AS rises 16ns after the fall -- so #AS is low for the
// high phase, shifted by that delay. The delay reaches 24ns, so a sample
// taken 20ns in read #AS high on the slow cycles and the loop below clocked
// straight past T1. This has to clear the worst case, not the typical one.
constexpr auto clk_delay_ns = 20;
// AD0-15 carries the address only while #AS is low, so it is sampled at the
// end of that window -- the rise latches the address and the CPU then turns
// AD around for data. #AS cannot rise until we drive #XTALI low, so waiting
// here only lets the address settle further.
constexpr auto addr_delay_ns = 20;
// ST0-ST3, #R//W and B//W are 3-state, valid only from the #AS rise.
constexpr auto status_delay_ns = 60;

// Bus transactions allowed for an #NMI acknowledge; generous.
constexpr auto nmi_ack_cycles = 1024;

const uint8_t PINS_LOW[] = {
        PIN_XTALI,
        PIN_RESET,
};

const uint8_t PINS_HIGH[] = {
        PIN_INTA,
        PIN_NMI,
        PIN_WAIT,

};

const uint8_t PINS_INPUT[] = {
        PIN_CLK,
        PIN_DS,
        PIN_AS,
        PIN_RW,
        PIN_BW,
        PIN_ST0,
        PIN_ST1,
        PIN_ST2,
        PIN_ST3,
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
        PIN_ADR16,
        PIN_ADR17,
        PIN_ADR18,
        PIN_ADR19,
        PIN_ADR20,
        PIN_ADR21,
        PIN_ADR22,
        PIN_ADR23,
};

inline void xtali_lo() {
    digitalWriteFast(PIN_XTALI, LOW);
}

inline void xtali_hi() {
    digitalWriteFast(PIN_XTALI, HIGH);
}

// Up to the rising edge. Returns high, as resetPins() needs.
void xtali_cycle_hi() {
    xtali_lo();
    delayNanoseconds(xtali_lo_ns);
    xtali_hi();
}

// The same, at the slower rate the reset sequence needs.
void xtali_cycle_reset() {
    xtali_lo();
    delayNanoseconds(reset_lo_ns);
    xtali_hi();
    delayNanoseconds(reset_hi_ns);
}

void assert_reset() {
    digitalWriteFast(PIN_RESET, LOW);
}

inline auto signal_clk() {
    return digitalReadFast(PIN_CLK);
}

// #CLK only takes the Bus Clock Select some cycles after the latch; while
// #RESET drives it, it stays on a fixed divider. Sampled once per #XTALI
// cycle past the first bus transaction it is steady at CS=01 and
// alternates at CS=00. #WAIT parks the CPU in T2, so these cycles are free.
bool busClockLatched() {
    auto changes = 0;
    auto prev = signal_clk();
    for (auto i = 0; i < 16; i++) {
        xtali_lo();
        delayNanoseconds(xtali_lo_ns);
        xtali_hi();
        delayNanoseconds(clk_delay_ns);
        const auto level = signal_clk();
        // Skip the first few, where the sampling phase is still settling.
        if (i >= 4 && level != prev)
            ++changes;
        prev = level;
    }
    xtali_lo();
    return changes == 0;
}

void negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
}

void assert_wait() {
    digitalWriteFast(PIN_WAIT, LOW);
}

void negate_wait() {
    digitalWriteFast(PIN_WAIT, HIGH);
}

// #NMI is falling-edge activated; negate before the next step.
void assert_nmi() {
    digitalWriteFast(PIN_NMI, LOW);
}

void negate_nmi() {
    digitalWriteFast(PIN_NMI, HIGH);
}

inline auto signal_as() {
    return digitalReadFast(PIN_AS);
}

inline auto signal_ds() {
    return digitalReadFast(PIN_DS);
}

// A word rides byte-swapped: low byte on AD8-15, high on AD0-7.
inline uint16_t swapBytes(uint16_t v) {
    return static_cast<uint16_t>((v << 8) | (v >> 8));
}

}  // namespace

PinsZ280::PinsZ280() {
    _devs = new DevsZ280();
    // Memory first: RegsZ280 stages its AF restore word there.
    _mems = new MemsZ280();
    _regs = new RegsZ280(this, mems<MemsZ280>());
}

// The latch misses about one reset in eight and comes up at CS=00,
// half speed, so this checks #CLK and repeats.
void PinsZ280::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    Cycles::reset();
    for (auto retry = 0;; ++retry) {
        // #RESET must be asserted 128 processor clocks, half the #XTALI rate.
        for (auto i = 0; i < (128 + 8) * 4; i++)
            xtali_cycle_reset();
        // With #RESET sampled high and #WAIT asserted, AD0-AD7 is sampled on
        // the falling processor clock into the Bus Timing and Initialization
        // register.
        // |7| 6| 5|4|32|10| CS: 00=1/2 01=1/1 10=1/4 (Bus Clock Select)
        // |1|BS|MP|0|LM|CS| LM: lower 8MB waits, MP: multiprocessor, BS: boot
        assert_wait();
        auto s = Signals::put();
        s->data = 0x80 | 1;  // Bus clock = XTALI
        s->outData();
        negate_reset();
        // Let #RESET settle; the hold loop opens on a falling edge.
        delayNanoseconds(reset_hi_ns);
        // #WAIT must stay asserted two processor clocks past #RESET, with AD
        // still driven at the sampling edge.
        for (auto i = 0; i < 16 * 2; i++)
            xtali_cycle_reset();
        // Release AD before the first fetch.
        s->inputMode();
        // #WAIT stalls the CPU in T2; record it before T2 takes the address.
        auto first = prepareCycle();
        if (busClockLatched() || retry == reset_retries) {
            // A refresh can come first: park on the reset fetch instead.
            // That is where the CPU stays from here on, and with the MMU
            // reset its address is the PC as well.
            negate_wait();
            first = skipToRead(first);
            assert_wait();
            _regs->setIp(first->addr);
            break;
        }
        assert_reset();
    }
    disableCache();
    disableRefresh();
    _regs->save();
}

void PinsZ280::idle() {
    // The CPU is parked, so this only keeps the clock alive.
    xtali_cycle_hi();
}

// Clock past whatever is not a memory read -- a refresh, whose address
// is the refresh counter's, not the PC's -- and return the read the CPU
// is then in. #WAIT must be negated on entry; the caller parks.
Signals *PinsZ280::skipToRead(Signals *s) {
    auto guard = Cycles::MAX_CYCLES;
    while (guard-- && !(s->memReq() && s->read())) {
        completeCycle(s);
        s = prepareCycle();
    }
    return s;
}

Signals *PinsZ280::prepareCycle() {
    auto s = Signals::put();
    // #AS may already be asserted on entry, so sample before clocking. No
    // critical section: what the CPU drives holds until the next edge.
    while (signal_as() != LOW) {
        xtali_lo();
        delayNanoseconds(xtali_lo_ns);
        xtali_hi();
        delayNanoseconds(clk_delay_ns);
    }
    // Take the address while #AS is still low, as late in that window as it
    // allows: the rise latches it and AD turns around for data afterwards.
    delayNanoseconds(addr_delay_ns);
    // assert_debug();
    s->getAddr();
    // negate_debug();
    // The status lines are 3-state and valid only from the #AS rise, which
    // xtali_lo() makes.
    xtali_lo();
    delayNanoseconds(status_delay_ns);
    // assert_debug();
    s->getControl();
    // negate_debug();
    // Halt and Refresh reach the caller; completeCycle() clocks them out.
    return s;
}

// Let the parked CPU go; AD0-15 no longer carries the address.
Signals *PinsZ280::resumeCycle(uint32_t addr) {
    auto s = Signals::put();
    // The ring is a log that reset and discard rewrite, and parked in T2
    // AD carries data, so the address cannot come from either: the caller
    // supplies the one captured when the CPU was parked.
    s->addr = addr;
    s->getControl();
    negate_wait();
    return s;
}

// Run it to the end. Halt and Refresh move no data, nor strobe #DS.
Signals *PinsZ280::completeCycle(Signals *s) {
    if (s->halt() || s->refresh()) {
        // No data moves; clock it out to a cycle boundary. Neither is
        // recorded -- a halted CPU repeats them for as long as it sits
        // there -- so s->prev() stays the previous real transaction.
        idle();
        s->inputMode();
        return s;
    }

    // T2. #DS may be negated (from prepareCycle) or asserted (resumeCycle).
    // That second case is why nothing here counts T states: after
    // resumeCycle() the transaction is coming back out of a #WAIT stretch
    // and there is no way to tell which T state the CPU is in, so both
    // edges below are found by watching #DS, never by clocking a fixed
    // number of cycles from a known one.
    for (;;) {
        const auto ds = signal_ds();
        if (ds == LOW)
            break;
        xtali_lo();
        delayNanoseconds(xtali_lo_ns);
        xtali_hi();
        delayNanoseconds(clk_delay_ns);
    }

    // An I/O port address is 16 bits; A16-A23 carry only the I/O Page.
    // Every device branch below is gated on ioReq(): a device may answer an
    // I/O transaction and nothing else. Selecting on the address alone let
    // one answer an #NMI acknowledge -- which carries no address at all, and
    // asks for no data, since #NMI vectors to 0066H by itself -- whenever
    // the stale lines happened to fall in its range.
    const uint16_t ioaddr = s->addr;

    if (s->read()) {
        if (s->memReq()) {
            if (s->readMemory()) {
                if (s->wordAccess()) {
                    s->data = mems<MemsZ280>()->read_zbus(s->addr);
                } else {
                    // Drive both halves; the CPU samples the one
                    // matching the address parity.
                    const uint16_t v = _mems->read_byte(s->addr);
                    s->data = v | (v << 8);
                }
            }
        } else if (s->intAck()) {
            // No address; the vector rides AD0-7, or the CPU reads a NOP.
            s->data = _devs->vector();
        } else if (s->ioReq() && _devs->isSelected(ioaddr) &&
                s->readMemory()) {
            if (s->wordAccess()) {
                s->data = swapBytes(_devs->read(ioaddr));
            } else {
                const uint16_t v = _devs->read(ioaddr);
                s->data = v | (v << 8);
            }
        }
        // The CPU samples AD while T3 #CLK is high, so drive here in T2,
        // well ahead of it, and hold until #DS negates.
        // assert_debug();
        s->outData();
        // negate_debug();
    } else if (s->write()) {
        // Data is valid when #DS asserts on a write, so latch it once.
        // assert_debug();
        s->getData();
        // negate_debug();
        if (s->memReq()) {
            if (s->writeMemory()) {
                if (s->wordAccess()) {
                    mems<MemsZ280>()->write_zbus(s->addr, s->data);
                } else {
                    // Driven on both halves; pick by address parity.
                    const uint8_t v = (s->addr & 1) ? lo(s->data) : hi(s->data);
                    _mems->write_byte(s->addr, v);
                }
            }
        } else if (s->ioReq() && _devs->isSelected(ioaddr) &&
                s->writeMemory()) {
            if (s->wordAccess()) {
                _devs->write(ioaddr, swapBytes(s->data));
            } else {
                // Byte I/O always uses AD0-AD7.
                _devs->write(ioaddr, lo(s->data));
            }
        }
    }

    // T3. Hold until #DS negates: a read must drive AD0-15 until then.
    for (;;) {
        xtali_lo();
        delayNanoseconds(xtali_lo_ns);
        xtali_hi();
        delayNanoseconds(clk_delay_ns);
        // Compared against LOW, never HIGH: a fast read may hand back the
        // masked register bit rather than 1, which is not equal to HIGH.
        if (signal_ds() != LOW)
            break;
    }

    s->inputMode();
    Cycles::next();
    return s;
}

uint16_t PinsZ280::injectRead(uint16_t data) {
    auto s = prepareCycle();
    completeCycle(s->inject(data));
    return s->addr;
}

// The aligned pair around |off|, on the lanes read_zbus() uses: the
// even-address byte on AD8-15, the odd one on AD0-7. The lane follows
// |addr|, not |off|: at an odd origin they disagree and the stream
// would be delivered shifted by a byte.
uint16_t PinsZ280::zbusWord(
        const uint8_t *inst, uint_fast8_t len, uint32_t off, uint32_t addr) {
    const auto at = [&](uint32_t i) {
        return i < len ? inst[i] : InstZ280::NOP;
    };
    return (addr & 1) ? uint16(off ? at(off - 1) : InstZ280::NOP, at(off))
                      : uint16(at(off), at(off + 1));
}

uint16_t PinsZ280::execute(const uint8_t *inst, uint_fast8_t len, uint8_t *buf,
        uint_fast8_t max, uint32_t &org, uint32_t exit) {
    uint16_t addr = 0;
    uint_fast8_t cap = 0;
    // |inst| is answered by address. Uncached the CPU reads a word at
    // every PC value, so a running position would run at twice the rate.
    // |exit| is where the sequence ends -- a prefetch past the window
    // looks just like falling through, so the caller must say, as a
    // physical address (RegsZ280::physical()).
    const uint32_t leaves = exit == EXIT_END    ? org + len
                            : exit == EXIT_ORG ? org
                                               : exit;
    bool started = false;
    auto s = resumeCycle(org);
    // Bound the damage: a wrong dump beats a hung debugger.
    auto guard = Cycles::MAX_CYCLES;
    while (guard--) {
        const auto reading = s->memReq() && s->read();
        const uint32_t off = reading ? s->addr - org : 0;
        // Done at that address once captured; other outside reads come
        // from memory. A jump target is matched by its word: with the
        // cache on the CPU fetches the even-aligned word containing it.
        const auto explicitExit = exit != EXIT_END && exit != EXIT_ORG;
        const auto atExit = explicitExit ? (s->addr | 1) == (leaves | 1)
                                         : s->addr == leaves;
        if (started && reading && cap >= max && atExit)
            break;
        if (reading)
            started = true;
        const auto injecting = reading && off < len;
        const auto capturing = s->memReq() && s->write() && cap < max;
        if (injecting) {
            // Every read is answered from |inst| at the offset asked for.
            if (s->wordAccess()) {
                s->inject(zbusWord(inst, len, off, s->addr));
            } else {
                const uint8_t b0 = inst[off];
                s->inject(static_cast<uint16_t>(b0) << 8 | b0);
            }
        } else if (capturing) {
            if (cap == 0)
                addr = s->addr;
            s->capture();
        }
        completeCycle(s);
        if (capturing && buf) {
            if (s->wordAccess()) {
                buf[cap++] = hi(s->data);
                if (cap < max)
                    buf[cap++] = lo(s->data);
            } else {
                buf[cap++] = (s->addr & 1) ? lo(s->data) : hi(s->data);
            }
        }
        s = prepareCycle();
    }
    // Park in it, and hand the caller where that is -- as the bus showed
    // it, which is |leaves| unless the guard ran out.
    assert_wait();
    org = s->addr;
    return addr;
}

void PinsZ280::disableRefresh() {
    // Clearing Refresh Enable does not stop the transactions -- the rate
    // field becomes a minimum bus transaction rate (9.3) -- but it buys
    // the longest interval; rate 0 is a special case, not the slowest.
    // The register is on-chip at FFxxE8, and reset clears the I/O Page, so
    // set the page to FF and put it back or later I/O never reaches the bus.
    static constexpr uint8_t REFRESH_OFF[] = {
        0x0E, 0x08,        // LD C, 08H       ; I/O Page register
        0x21, 0xFF, 0x00,  // LD HL, 00FFH    ; on-chip peripheral page
        0xED, 0x6E,        // LDCTL (C), HL
        0x01, 0xE8, 0x00,  // LD BC, 00E8H    ; Refresh Rate register
        0x3E, 0x3F,        // LD A, 3FH       ; E=0, slowest rate
        0xED, 0x79,        // OUT (C), A
        0x0E, 0x08,        // LD C, 08H       ; I/O Page register
        0x21, 0x00, 0x00,  // LD HL, 0000H    ; back to the reset value
        0xED, 0x6E,        // LDCTL (C), HL
        0x18, 0xE9,        // JR 0000H
    };
    // clang-format on
    auto org = _regs->nextIp();
    execInst(REFRESH_OFF, sizeof(REFRESH_OFF), org, EXIT_ORG);
}

void PinsZ280::disableCache() {
    // The on-chip memory comes up as a cache, hiding fetches. I=1 stops
    // new ones, PCACHE drops the rest, and the jump home parks the CPU.
    static constexpr uint8_t CACHE_OFF[] = {
        0x0E, 0x12,        // LD C, 12H       ; Cache Control register
        0x21, 0x60, 0x00,  // LD HL, 0060H    ; I=1 D=1: cache nothing
        0xED, 0x6E,        // LDCTL (C), HL
        0xED, 0x65,        // PCACHE          ; drop the lines filled before
        0x00,              // NOP             ; keep the array even
        0x18, 0xF4,        // JR 0000H
    };
    // clang-format on
    auto org = _regs->nextIp();
    execInst(CACHE_OFF, sizeof(CACHE_OFF), org, EXIT_ORG);
}


void PinsZ280::execInst(
        const uint8_t *inst, uint_fast8_t len, uint32_t &org, uint32_t exit) {
    execute(inst, len, nullptr, 0, org, exit);
}

uint16_t PinsZ280::captureWrites(const uint8_t *inst, uint_fast8_t len,
        uint8_t *buf, uint_fast8_t max, uint32_t &org, uint32_t exit) {
    return execute(inst, len, buf, max, org, exit);
}

// Step one instruction; the #NMI vector fetch is aborted with a RETN.
// The last memory read before |s|: the program's fetch, whose page the
// PC is in. The transaction the halt was noticed on may be anything --
// an acknowledge cycle carries no address.
static uint32_t lastFetchBefore(const Signals *s, uint32_t fallback) {
    for (auto i = 1; i <= 8; ++i) {
        const auto t = s->prev(i);
        if (t->memReq() && t->read())
            return t->addr;
    }
    return fallback;
}

bool PinsZ280::suspend(uint32_t org) {
    // #NMI goes out during the opcode fetch: later steps two, earlier none.
    auto s = resumeCycle(org);
    assert_nmi();
    completeCycle(s);
    s = prepareCycle();
    // Bound the wait; loop() polls the halt switch only between steps.
    auto guard = nmi_ack_cycles;
    auto seen = 0;
    while (guard--) {
        const Signals *push = nullptr;
        if (s->memReq() && s->read() && seen >= 5) {
            // Interrupt mode 3 takes the NMI through the vector table:
            // an acknowledge cycle, then it pushes the PC, the MSR and
            // the word the acknowledge read as an identifier, reads the
            // MSR and PC of the handler from the table, and fetches
            // there -- with no prefetch in between, the pipeline having
            // been flushed. That fetch is where the CPU parks: the
            // debugger's sequences run in system mode from the
            // handler's address, and restore() returns with RETIL from
            // the MSR word, leaving the identifier below it. The handler
            // itself is never executed. What its table entry must
            // provide is an MSR that disables interrupts.
            const auto r2 = s->prev(1);
            const auto r1 = s->prev(2);
            const auto w3 = s->prev(3);  // identifier
            const auto w2 = s->prev(4);  // MSR
            const auto w1 = s->prev(5);  // PC
            if (r2->memReq() && r2->read() && r1->memReq() && r1->read() &&
                    w3->memReq() && w3->write() && w2->memReq() &&
                    w2->write() && w1->memReq() && w1->write() &&
                    w2->addr == w3->addr + 2 && w1->addr == w2->addr + 2 &&
                    r2->addr == r1->addr + 2 &&
                    (s->addr & 0xFFF) == (swapBytes(r2->data) & 0xFFF)) {
                negate_nmi();
                const uint16_t pc = swapBytes(w1->data);
                const uint16_t msr = swapBytes(w2->data);
                // The pushes, the table reads and this fetch are the
                // debugger's; the program's cycles end before them.
                Cycles::discard(w1);
                assert_wait();
                this->regs<RegsZ280>()->parkInVector(RegsZ280::NMI3, pc,
                        lastFetchBefore(w1, org), w2->addr, s->addr, msr);
                return true;
            }
        }
        if (s->addr == InstZ280::ORG_NMI && s->read() && s->memReq()) {
            // A prefetch can land between the push and this fetch, so the
            // push is not always s->prev() -- scan back for it, as
            // isRst38Break() does. Insisting on s->prev() made the
            // acknowledge go unrecognised whenever one did: the guard below
            // then ran out and left the CPU inside the NMI service, and the
            // PC saved from there was garbage. That is what made a halt, and
            // the step over a breakpoint that a continue needs, fail
            // intermittently.
            // Only a prefetch may intervene, so accept the write either
            // immediately before this fetch or one read behind it. Scanning
            // further back matches an ordinary program write instead -- a
            // write-heavy program then reports a pushed PC that is really a
            // stored datum.
            const auto one = s->prev(1);
            if (one->memReq() && one->write()) {
                push = one;
            } else if (one->memReq() && one->read()) {
                const auto two = s->prev(2);
                if (two->memReq() && two->write())
                    push = two;
            }
        }
        if (push != nullptr) {
            negate_nmi();
            // The push and this fetch are the debugger's.
            Cycles::discard(push);
            assert_wait();
            this->regs<RegsZ280>()->parkInVector(RegsZ280::NMI,
                    swapBytes(push->data), lastFetchBefore(push, org),
                    push->addr, s->addr);
            return true;
        }
        completeCycle(s);
        s = prepareCycle();
        ++seen;
    }
    negate_nmi();
    return false;
}

bool PinsZ280::rawStep() {
    // Stop before a HALT: once halted there is no boundary for #NMI.
    if (_mems->read_byte(_regs->nextIp()) == InstZ280::HALT)
        return false;
    return suspend(this->regs<RegsZ280>()->parkedAt());
}

bool PinsZ280::step(bool show) {
    Cycles::reset();
    this->regs<RegsZ280>()->cacheForStep(true);
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



// A restart to 0038H reports a break: a patched breakpoint or the exit convention.
bool PinsZ280::isRst38Break(Signals *s) {
    // A prefetch can land between the push and this fetch; look back for it.
    const Signals *push = nullptr;
    for (auto i = 1; i <= 4 && push == nullptr; ++i) {
        const auto t = s->prev(i);
        if (t->memReq() && t->write())
            push = t;
    }
    if (push == nullptr)
        return false;
    // RST 38H is one byte; take whichever of the two holds the opcode.
    const uint16_t resume = swapBytes(push->data);
    uint16_t pc = resume - 1;
    if (_mems->read_byte(pc) != InstZ280::RST38)
        pc = resume;
    // A mode 1 interrupt vectors here too, so require the opcode.
    if (_mems->read_byte(pc) != InstZ280::RST38)
        return false;
    if (!isBreakPoint(pc) &&
            _mems->read_byte(InstZ280::ORG_RST38) != InstZ280::RST38)
        return false;
    // Park in the vector fetch: the sequences run here, in system space,
    // never in the program's lines, which may be cached.
    assert_wait();
    this->regs<RegsZ280>()->parkInVector(
            RegsZ280::RST, pc, pc, push->addr, s->addr);
    return true;
}

// Free-run: stepping with #NMI starves maskable interrupts and is slow.
// Returns whether the CPU stopped at a boundary its registers can be
// saved from. The ring is left holding the program's cycles only: each
// exit drops what the debugger itself put there, as the Z80 does, so the
// dump that follows -- and the save after it -- show what ran, not the
// machinery that stopped it.
bool PinsZ280::loop() {
    auto s = resumeCycle(this->regs<RegsZ280>()->parkedAt());
    while (true) {
        // Rarest first: the address, then the two bit tests, then the
        // scan.
        if (s->addr == InstZ280::ORG_RST38 && s->read() && s->memReq() &&
                isRst38Break(s)) {
            // From the vector fetch on it is the RET and JP injected to
            // unwind the restart.
            Cycles::discard(s);
            return true;
        }
        if (s->halt()) {
            // Halted there is no instruction boundary to inject at, so
            // nothing can be saved: the registers stay as last saved.
            Cycles::discard(s);
            return false;
        }
        completeCycle(s);
        _devs->loop();
        s = prepareCycle();
        // Only now, with a transaction recorded: suspend() opens with
        // resumeCycle(), which reads that slot back. Asked before
        // prepareCycle() it would find the one Cycles::next() had just
        // cleared and complete a cycle that does not exist -- which only
        // sometimes does visible damage, so a halt looked intermittent.
        if (haltSwitch()) {
            const auto stop = s;
            if (suspend(s->addr)) {
                // The #NMI push, vector fetch and injected RETN.
                Cycles::discard(stop);
                return true;
            }
            // When it fails the CPU is left somewhere inside the NMI
            // service, so everything save() read there would be garbage --
            // SP included -- and restore() would write that back on the
            // next continue and destroy the program. Keep the registers
            // from the last good save, as step() already does. The ring is
            // left alone too: suspend() may have run its whole guard, and
            // |stop| no longer points anywhere meaningful.
            return false;
        }
    }
}

void PinsZ280::run() {
    this->regs<RegsZ280>()->cacheForStep(false);
    _regs->restore();
    Cycles::reset();
    saveBreakInsts();
    const auto stopped = loop();
    restoreBreakInsts();
    disassembleCycles();
    // After the dump, so its injected reads and captured writes never
    // appear in it -- step() prints before saving for the same reason.
    if (stopped)
        _regs->save();
}

void PinsZ280::setBreakInst(uint32_t addr) const {
    // MemsZ280 is a byte memory to Mems, so this patches one byte.
    _mems->put_prog(addr, InstZ280::RST38);
}

void PinsZ280::assertInt(uint8_t) {
    digitalWriteFast(PIN_INTA, LOW);
}

void PinsZ280::negateInt(uint8_t) {
    digitalWriteFast(PIN_INTA, HIGH);
}

void PinsZ280::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

void PinsZ280::disassembleCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles;) {
        const auto s = g->next(i);
        s->print();
        ++i;
        idle();
    }
}

}  // namespace z280
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
