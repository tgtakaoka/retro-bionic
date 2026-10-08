#include "pins_z380.h"
#include "debugger.h"
#include "devs_z380.h"
#include "digital_bus.h"
#include "inst_z380.h"
#include "mems_z380.h"
#include "regs_z380.h"
#include "signals_z380.h"

namespace debugger {
namespace z380 {

using z80::DevsZ80;

// clang-format off
/**
 * Z380 memory transaction (Z380 Product Specification, External
 * Interface, Figures 3-10; AC Characteristics). CLKSEL is tied high, so
 * CLKI is the direct clock and BUSCLK is CLKI inverted: it rises up to
 * 30ns after CLKI falls and falls up to 27ns after CLKI rises, and
 * outputs follow a BUSCLK edge by 6.5ns.
 *
 *        |  T1 |  T2 |  T3 |  T4 |
 *              _____       _____
 * CLKI   _____|     |_____|     |_____
 *        _____       _____       _____
 * BUSCLK      |_____|     |_____|
 *        _ _______________________ ___
 * ADDR   _X_______________________X___
 *        ______                   ____
 * #MRD         |_________________|
 *                               v latched
 * #WAIT        ^     ^     ^ sampled
 *
 * #MRD and #MWR assert at the end of T1 and negate at the end of T4; a
 * write drives data from the start of T1. So with CLKI low (BUSCLK high)
 * a strobe reads negated in T1 and asserted in T3, and that is the only
 * phase the strobes are sampled in. #WAIT inserts whole BUSCLK cycles at
 * any of the three boundaries, after the internal waits; the strobes stay
 * asserted meanwhile.
 *
 * An I/O transaction is four IOCLK cycles (Figures 17-20), #IORD/#IOWR
 * asserted from the second rise to the fourth fall, where read data is
 * latched; setupBus() makes IOCLK BUSCLK/2. An #INT0 acknowledge asserts
 * #M1 alone for its five cycles (Figure 21), and the RETI reproduced on
 * the I/O bus asserts #M1 with #IORD while the CPU drives the data.
 * Refresh and on-chip I/O strobe neither #MRD/#MWR nor #IORD/#IOWR, and
 * neither does a halt.
 */
// clang-format on

namespace {

// The CPU is static; the clock is counted in its phases and rests low,
// BUSCLK high. The high phase is the Z8038018's 24.5ns minimum width;
// clk_delay_ns clears BUSCLK's rise and the outputs it moves (30ns +
// 6.5ns) before anything is sampled.
constexpr auto clki_hi_ns = 25;
constexpr auto clk_delay_ns = 40;
constexpr auto reset_hi_ns = 100;
constexpr auto reset_lo_ns = 100;
// #RESET wants five stable BUSCLK cycles; give it more.
constexpr auto reset_cycles = 16;
// With no #HALT, a CPU that strobes nothing for this many clocks halted.
// Long enough to clear the slowest instruction with no bus transaction.
constexpr auto halt_clocks = 1024;
// Bus transactions allowed for an #NMI acknowledge; generous.
constexpr auto nmi_ack_cycles = 1024;
// After #M1 alone, how many BUSCLK cycles to wait for #IORD before taking
// it for an #INT0 acknowledge. A RETI asserts #IORD 1.5 IOCLK after #M1
// (Figure 23), 3 BUSCLK at IOCLK = BUSCLK/2; an acknowledge latches its
// vector 5 IOCLK in (Figure 21), so the wait must end well before that.
constexpr auto reti_iord_clocks = 6;

const uint8_t PINS_LOW[] = {
        PIN_CLKI,
        PIN_RESET,
        PIN_ASEL0,
        PIN_ASEL1,
};

const uint8_t PINS_HIGH[] = {
        PIN_INT0,
        PIN_NMI,
        PIN_WAIT,
};

const uint8_t PINS_INPUT[] = {
        PIN_M1,
        PIN_MRD,
        PIN_MWR,
        PIN_IORD,
        PIN_IOWR,
        PIN_BHEN,
        PIN_BLEN,
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

inline void clki_lo() {
    digitalWriteFast(PIN_CLKI, LOW);
}

inline void clki_hi() {
    digitalWriteFast(PIN_CLKI, HIGH);
}

// One BUSCLK cycle, low half first, then wait for what its rise moves.
inline void clki_cycle() {
    clki_hi();
    delayNanoseconds(clki_hi_ns);
    clki_lo();
    delayNanoseconds(clk_delay_ns);
}

void clki_cycle_reset() {
    clki_hi();
    delayNanoseconds(reset_hi_ns);
    clki_lo();
    delayNanoseconds(reset_lo_ns);
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

// A word rides byte-swapped: the low byte, at the even address, on D8-D15.
inline uint16_t swapBytes(uint16_t v) {
    return static_cast<uint16_t>((v << 8) | (v >> 8));
}

inline uint16_t bothLanes(uint8_t v) {
    return static_cast<uint16_t>(v) << 8 | v;
}

inline bool memRead(const Signals *s) {
    return s->memReq() && s->read();
}

inline bool memWrite(const Signals *s) {
    return s->memReq() && s->write();
}

}  // namespace

inline bool Signals::getControl() {
    cntl() = busRead(CNTL);
    return cntl() != NONE;
}

inline void Signals::getBen() {
    ben() = busRead(BEN);
}

PinsZ380::PinsZ380() {
    _devs = new DevsZ80(USART_BASE);
    // Memory first: RegsZ380 stages its AF restore word there.
    _mems = new MemsZ380();
    _regs = new RegsZ380(this, mems<MemsZ380>());
}

void PinsZ380::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    Cycles::reset();
    for (auto i = 0; i < reset_cycles; i++)
        clki_cycle_reset();
    // The first fetch, from 00000000H 3.5 BUSCLK after #RESET rises, is
    // held open by #WAIT after T1: that is where the CPU parks.
    assert_wait();
    negate_reset();
    const auto s = prepareCycle();
    // Anything but a read at 00000000H means the board, most likely the
    // address mux, is not what the driver assumes: say so rather than
    // save registers from a CPU that is not parked.
    if (s->halt() || !memRead(s) || s->addr != 0) {
        cli.print("?reset: first cycle ");
        s->print();
        return;
    }
    _regs->setIp(s->addr);
#ifdef PROFILE_CYCLES
    dataLoopback();
#endif
    setupBus();
    if (_cutShort) {
        cli.println("?reset: setup sequence cut short");
        printCycles();
        return;
    }
    _regs->save();
    _regs->reset();
}

// Reset leaves 7+3+7 waits on every memory transaction below 1MB and in
// the top one, 7 on I/O, and IOCLK at BUSCLK/8. Clear the waits, so #WAIT
// alone stretches a transaction, and run IOCLK at BUSCLK/2: at BUSCLK an
// acknowledge latches its vector before #M1 can be told from a RETI.
// On-chip I/O has no external strobe, so these writes do not show on the
// bus.
void PinsZ380::setupBus() {
    // clang-format off
    static constexpr uint8_t SEQ[] = {
        0xAF,              // XOR A
        0xED, 0x39, 0x08,  // OUT0 (08H), A  ; LMWR
        0xED, 0x39, 0x09,  // OUT0 (09H), A  ; UMWR
        0xED, 0x39, 0x0A,  // OUT0 (0AH), A  ; MMWR0
        0xED, 0x39, 0x0B,  // OUT0 (0BH), A  ; MMWR1
        0xED, 0x39, 0x0C,  // OUT0 (0CH), A  ; MMWR2
        0xED, 0x39, 0x0D,  // OUT0 (0DH), A  ; MMWR3
        0xED, 0x39, 0x0E,  // OUT0 (0EH), A  ; IOWR
        0x3E, 0x02,        // LD A, 02H
        0xED, 0x39, 0x11,  // OUT0 (11H), A  ; IOCR0: IOCLK = BUSCLK/2
        0x18, 0xE3,        // JR org
    };
    // clang-format on
    static_assert(SEQ[sizeof(SEQ) - 1] == uint8_t(-sizeof(SEQ)),
            "JR org: the displacement is minus the length");
    auto org = _regs->nextIp();
    execInst(SEQ, sizeof(SEQ), org, EXIT_ORG);
}

#ifdef PROFILE_CYCLES
// Send a walking 1 and a walking 0 through each data lane and read them
// back from the CPU's push, so a bad data line shows at reset; silent when
// all lines pass. Opcodes ride the
// other lane, so a fault on the lane under test corrupts only the operand.
void PinsZ380::dataLoopback() {
    const auto start = _regs->nextIp();  // even: the reset fetch
    auto org = start;
    // LD SP,8000H; a fault on a low bit can only make it 8004H, still even.
    static constexpr uint8_t SP_8000[] = {0x31, 0x00, 0x80, 0x00};
    execInst(SP_8000, sizeof(SP_8000), org);
    uint8_t high[2] = {0, 0};  // per lane: bits read 1 for a 0 sent
    uint8_t low[2] = {0, 0};   // per lane: bits read 0 for a 1 sent
    for (uint_fast8_t lane = 0; lane < 2 && !_cutShort; ++lane) {
        for (uint_fast8_t k = 0; k < 16 && !_cutShort; ++k) {
            const uint8_t sent = k < 8 ? (1 << k) : ~(1 << (k - 8));
            // odd lane: 3E n F5 at 0,1,2; even lane: one NOP first. The
            // JR $+2 fetches the exit again after the push has landed.
            uint8_t seq[] = {0x3E, sent, 0xF5, 0x00, 0x18, 0x00};
            if (lane == 1) {
                seq[0] = 0x00, seq[1] = 0x3E, seq[2] = sent, seq[3] = 0xF5;
            }
            uint8_t buf[2] = {0, 0};
            captureWrites(seq, sizeof(seq), buf, sizeof(buf), org);
            const uint8_t got = buf[1];  // A, pushed above F
            high[lane] |= got & ~sent;
            low[lane] |= ~got & sent;
            if (got != sent || _cutShort) {
                cli.print(lane ? "?data even lane: sent "
                               : "?data odd lane: sent ");
                cli.printHex(sent, 2);
                cli.print(" got ");
                cli.printHex(got, 2);
                cli.println(_cutShort ? " (cut short)" : "");
            }
        }
    }
    // Everything after a cut-short sequence starts from the wrong place.
    if (_cutShort)
        return;
    // Park back at the reset fetch, where setupBus() starts.
    const uint8_t back[] = {0xC3, uint8_t(start), uint8_t(start >> 8), 0x00};
    execInst(back, sizeof(back), org, start);
    if ((high[0] | low[0] | high[1] | low[1]) == 0)
        return;
    cli.print("?data loopback: odd D0-D7 high=");
    cli.printHex(high[0], 2);
    cli.print(" low=");
    cli.printHex(low[0], 2);
    cli.print(", even D8-D15 high=");
    cli.printHex(high[1], 2);
    cli.print(" low=");
    cli.printlnHex(low[1], 2);
}
#endif

void PinsZ380::idle() {
    // The CPU is parked, so this only keeps the clock alive.
    clki_cycle();
}

// Clock up to T3 of the next transaction (or the wait cycle #WAIT holds
// it in), or to the conclusion that the CPU halted.
Signals *PinsZ380::prepareCycle() {
    auto s = Signals::put();
    s->clearMark();  // Cycles::next() keeps the slot's old mark
    for (auto n = 0; !s->getControl(); ++n) {
        if (n >= halt_clocks) {
            s->markHalt();
            return s;
        }
        clki_cycle();
    }
    // The CPU moves only on our edges, so the address and status hold
    // while the muxes are stepped through.
    s->getBen();
    // #M1 alone opens an #INT0 acknowledge, but also each half of a RETI
    // transaction, which #IORD joins later and whose data the CPU drives
    // (Figure 23): wait for #IORD before answering.
    for (auto n = 0; s->intAck() && n < reti_iord_clocks; ++n) {
        clki_cycle();
        s->getControl();
    }
    s->getAddr();
    return s;
}

// Let the parked CPU go. The address is the caller's to give: the ring is
// a log that reset and discard rewrite.
Signals *PinsZ380::resumeCycle(uint32_t addr) {
    auto s = Signals::put();
    s->clearMark();
    s->addr = addr;
    s->getControl();
    s->getBen();
    negate_wait();
    return s;
}

Signals *PinsZ380::completeCycle(Signals *s) {
    if (s->halt()) {
        // Nothing moves; not recorded, so s->prev() stays the previous
        // real transaction.
        s->inputMode();
        return s;
    }

    // Only A0-A23 are decoded: the 16MB mirrors across the 4GB space.
    const auto maddr = s->addr & MemsZ380::ADDR_MASK;
    // I/O ports are decoded from all 32 bits: IN A,(n) puts A on A8-A15,
    // so the samples use INA/OUTA and the (C) forms.
    const auto ioaddr = s->addr;

    if (s->reti()) {
        // The CPU drives the reproduced RETI opcodes.
        s->getData();
    } else if (s->read()) {
        if (s->memReq()) {
            if (s->readMemory()) {
                if (s->wordAccess()) {
                    s->data = mems<MemsZ380>()->read_zbus(maddr);
                } else {
                    // Drive both halves; the CPU takes its lane.
                    s->data = bothLanes(_mems->read_byte(maddr));
                }
            }
        } else if (s->intAck()) {
            // The vector rides D0-D7.
            s->data = bothLanes(_devs->vector());
        } else if (s->ioReq() && _devs->isSelected(ioaddr) && s->readMemory()) {
            s->data = bothLanes(_devs->read(ioaddr));
        }
        s->outData();
    } else if (s->write()) {
        // Write data is driven from the start of the transaction.
        s->getData();
        if (s->memReq()) {
            if (s->writeMemory()) {
                if (s->wordAccess()) {
                    mems<MemsZ380>()->write_zbus(maddr, s->data);
                } else {
                    const uint8_t v = (maddr & 1) ? lo(s->data) : hi(s->data);
                    _mems->write_byte(maddr, v);
                }
            }
        } else if (s->ioReq() && _devs->isSelected(ioaddr) &&
                   s->writeMemory()) {
            // Byte I/O uses D0-D7.
            _devs->write(ioaddr, lo(s->data));
        }
    }

    // Hold until the strobes of this transaction negate, seen in the next
    // T1: read data is latched at the edge that negates them. Probed apart,
    // so |s| keeps them.
    Signals probe;
    auto guard = halt_clocks;
    while (probe.getControl() && guard--)
        clki_cycle();

    s->inputMode();
    if (_holdRing) {
        // the debugger's own cycles before the dump: reuse the head slot
        s->clear();
    } else {
        Cycles::next();
    }
    return s;
}

// The pair of bytes a word read at |addr| moves, from |inst| placed at
// |org|: the even-address byte on D8-D15, the odd one on D0-D7.
uint16_t PinsZ380::zbusWord(
        const uint8_t *inst, uint_fast8_t len, uint32_t org, uint32_t addr) {
    const auto at = [&](uint32_t a) {
        const auto off = a - org;
        return a >= org && off < len ? inst[off] : InstZ380::NOP;
    };
    const auto even = addr & ~UINT32_C(1);
    return uint16(at(even), at(even + 1));
}

uint32_t PinsZ380::execute(const uint8_t *inst, uint_fast8_t len, uint8_t *buf,
        uint_fast8_t max, uint32_t &org, uint32_t exit) {
    uint32_t lowest = UINT32_MAX;
    uint_fast8_t cap = 0;
    // |inst| is answered by address. |exit| is where the sequence ends --
    // a prefetch past the window looks just like falling through, so the
    // caller must say.
    const uint32_t leaves = exit == EXIT_END   ? org + len
                            : exit == EXIT_ORG ? org
                                               : exit;
    const auto explicitExit = exit != EXIT_END && exit != EXIT_ORG;
    bool started = false;
    // The target of the jump a sequence ends with may lie inside its own
    // window: it counts only once the last byte has been fetched.
    bool fetchedAll = false;
    bool exited = false;
    auto s = resumeCycle(org);
    // Bound the damage: a wrong dump beats a hung debugger.
    auto guard = Cycles::MAX_CYCLES;
    while (guard--) {
        if (s->halt())
            break;
        const auto reading = memRead(s);
        // A jump target is matched by its word: the CPU may fetch the
        // aligned word containing it.
        const auto atExit =
                fetchedAll && (explicitExit ? (s->addr | 1) == (leaves | 1)
                                            : s->addr == leaves);
        if (started && reading && cap >= max && atExit) {
            exited = true;
            break;
        }
        if (reading)
            started = true;
        // A word read moves the aligned pair, so it is in the window when
        // either byte is.
        const auto first = s->wordAccess() ? s->addr & ~UINT32_C(1) : s->addr;
        const auto last = s->wordAccess() ? first + 1 : first;
        const auto injecting = reading && last >= org && first < org + len;
        if (injecting && last >= org + len - 1)
            fetchedAll = true;
        const auto capturing = memWrite(s) && cap < max;
        if (injecting) {
            const auto word = zbusWord(inst, len, org, s->addr);
            if (s->wordAccess()) {
                s->inject(word);
            } else {
                s->inject(bothLanes((s->addr & 1) ? lo(word) : hi(word)));
            }
        } else if (capturing) {
            if (cap == 0)
                _firstWrite = s->addr;
            if (s->addr < lowest)
                lowest = s->addr;
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
    // A trap or a fetch from elsewhere leaves the sequence unfinished.
    _cutShort = !exited;
    org = s->addr;
    return lowest;
}

void PinsZ380::execInst(
        const uint8_t *inst, uint_fast8_t len, uint32_t &org, uint32_t exit) {
    execute(inst, len, nullptr, 0, org, exit);
}

uint32_t PinsZ380::captureWrites(const uint8_t *inst, uint_fast8_t len,
        uint8_t *buf, uint_fast8_t max, uint32_t &org, uint32_t exit) {
    return execute(inst, len, buf, max, org, exit);
}

namespace {
// The PC an RST or NMI pushed shortly before |s|: one word, two in
// Extended mode. Returns the earliest of those writes, nullptr if none.
// The program may have entered Extended mode since the SR was last read,
// so callers take both readings and RegsZ380::save() picks.
const Signals *findPush(
        const Signals *s, bool extended, RegsZ380::Frame &frame) {
    // A prefetch can land between the push and the vector fetch.
    const Signals *w[2] = {nullptr, nullptr};
    auto n = 0;
    const auto words = extended ? 2 : 1;
    for (auto i = 1; i <= 4 && n < words; ++i) {
        const auto t = s->prev(i);
        if (memWrite(t))
            w[n++] = t;
    }
    frame.valid = n == words;
    if (!frame.valid)
        return nullptr;
    if (!extended) {
        frame.pc = swapBytes(w[0]->data);
        frame.addr = w[0]->addr;
        return w[0];
    }
    const auto low = w[0]->addr < w[1]->addr ? w[0] : w[1];
    const auto high = low == w[0] ? w[1] : w[0];
    frame.pc = static_cast<uint32_t>(swapBytes(high->data)) << 16 |
               swapBytes(low->data);
    frame.addr = low->addr;
    return w[1];
}

// The push of an RST that fetched 0038H before it pushed, ahead of the
// RST 38H there that pushed |frame|: replaces |frame|, and |from| becomes
// that first vector fetch.
void findLatePush(const Signals *s, bool extended, RegsZ380::Frame &frame,
        Signals *&from) {
    const auto words = extended ? 2 : 1;
    const Signals *w[2] = {nullptr, nullptr};
    auto writes = 0;
    for (auto i = 1; i <= 10; ++i) {
        const auto t = s->prev(i);
        if (memWrite(t)) {
            // the first |words| are the second RST's
            if (++writes > words && writes <= 2 * words)
                w[writes - words - 1] = t;
            continue;
        }
        if (writes == 2 * words && memRead(t) &&
                t->addr == InstZ380::ORG_RST38) {
            RegsZ380::Frame late;
            if (extended) {
                const auto low = w[0]->addr < w[1]->addr ? w[0] : w[1];
                const auto high = low == w[0] ? w[1] : w[0];
                late.pc = static_cast<uint32_t>(swapBytes(high->data)) << 16 |
                          swapBytes(low->data);
                late.addr = low->addr;
            } else {
                late.pc = swapBytes(w[0]->data);
                late.addr = w[0]->addr;
            }
            late.valid = true;
            frame = late;
            from = const_cast<Signals *>(t);
            return;
        }
    }
}
}  // namespace

// Step one instruction: #NMI is taken at its end, and the CPU parks in
// the vector fetch with the PC pushed. #NMI is asserted after |holdOff|
// bus cycles.
bool PinsZ380::suspend(uint32_t org, uint_fast8_t holdOff) {
    const auto extended = this->regs<RegsZ380>()->extended();
    auto s = resumeCycle(org);
    if (holdOff == 0)
        assert_nmi();
    completeCycle(s);
    s = prepareCycle();
    for (uint_fast8_t n = 1; n < holdOff && !s->halt(); ++n) {
        completeCycle(s);
        s = prepareCycle();
    }
    assert_nmi();
    // Bound the wait; loop() polls the halt switch only between steps.
    auto guard = nmi_ack_cycles;
    while (guard-- && !s->halt()) {
        if (s->addr == InstZ380::ORG_NMI && memRead(s)) {
            RegsZ380::Frame native, wide;
            const auto push1 = findPush(s, false, native);
            const auto push2 = findPush(s, true, wide);
            const auto push = extended && push2 ? push2 : push1;
            if (push != nullptr) {
                negate_nmi();
                // The push and this fetch are the debugger's.
                Cycles::discard(push);
                assert_wait();
                this->regs<RegsZ380>()->parkInVector(
                        RegsZ380::NMI, s->addr, native, wide);
                return true;
            }
        }
        completeCycle(s);
        s = prepareCycle();
    }
    negate_nmi();
    // Hold the CPU wherever it is rather than let it run on.
    assert_wait();
    cli.println("?halt: no NMI acknowledge");
    return false;
}

bool PinsZ380::step(bool show) {
    // Stop before a HALT: once halted there is no boundary for #NMI. And
    // before restore(), which leaves the CPU at the program's PC, where
    // no sequence can be injected if it is odd.
    if (_mems->read_byte(_regs->nextIp() & MemsZ380::ADDR_MASK) ==
            InstZ380::HALT)
        return false;
    // An instruction whose bytes are not all fetched when #NMI is asserted
    // may not start at all: BIT 7,H (CB 7C) at an odd address was taken
    // with its own PC pushed. If nothing changed, assert it later; an
    // LDIR iteration or a DJNZ to itself changes a register.
    const auto regs = this->regs<RegsZ380>();
    const auto before = regs->fingerprint();
    for (uint_fast8_t holdOff = 0; holdOff < 3; ++holdOff) {
        Cycles::reset();
        _regs->restore();
        if (show)
            Cycles::reset();
        if (!suspend(regs->parkedAt(), holdOff))
            return false;
        _holdRing = true;  // keep the step's cycles for printCycles()
        _regs->save();
        _holdRing = false;
        if (regs->fingerprint() != before)
            break;
    }
    if (show)
        printCycles();
    return true;
}

// A restart to 0038H reports a break: a patched breakpoint or the exit
// convention. From |from| on the cycles are the debugger's.
bool PinsZ380::isRst38Break(Signals *s, Signals *&from) {
    RegsZ380::Frame frames[2];
    findPush(s, false, frames[0]);
    findPush(s, true, frames[1]);
    from = s;
    for (auto x = 0u; x < 2; ++x) {
        auto &f = frames[x];
        if (!f.valid)
            continue;
        // After an I/O write an RST fetches its vector before it pushes,
        // which the look back above misses; the RST 38H the vector holds
        // then breaks too, one push later. Take the first RST's push.
        if (f.pc == InstZ380::ORG_RST38 + 1)
            findLatePush(s, x == 1, f, from);
        // RST 38H is one byte; take whichever of the two holds the opcode.
        const auto resume = f.pc;
        f.pc = resume - 1;
        if (_mems->read_byte(f.pc & MemsZ380::ADDR_MASK) != InstZ380::RST38)
            f.pc = resume;
        // A mode 1 interrupt vectors here too, so require the opcode.
        if (_mems->read_byte(f.pc & MemsZ380::ADDR_MASK) != InstZ380::RST38)
            f.valid = false;
        else if (!isBreakPoint(f.pc) &&
                 _mems->read_byte(InstZ380::ORG_RST38) != InstZ380::RST38)
            f.valid = false;
    }
    if (!frames[0].valid && !frames[1].valid)
        return false;
    assert_wait();
    this->regs<RegsZ380>()->parkInVector(
            RegsZ380::RST, s->addr, frames[0], frames[1]);
    return true;
}

// Free-run: stepping with #NMI starves maskable interrupts and is slow.
// Returns whether the CPU stopped at a boundary its registers can be
// saved from. The ring is left holding the program's cycles only.
bool PinsZ380::loop() {
    auto s = resumeCycle(this->regs<RegsZ380>()->parkedAt());
    while (true) {
        Signals *from;
        if (s->addr == InstZ380::ORG_RST38 && memRead(s) &&
                isRst38Break(s, from)) {
            // From the vector fetch on it is the debugger's.
            Cycles::discard(from);
            return true;
        }
        if (s->halt()) {
            // Halted there is no instruction boundary to inject at, so
            // nothing can be saved: the registers stay as last saved.
            return false;
        }
        completeCycle(s);
        _devs->loop();
        s = prepareCycle();
        // Only now, with a transaction recorded: suspend() opens with
        // resumeCycle(), which reads that slot back.
        if (haltSwitch()) {
            // suspend() drops the #NMI push and vector fetch itself.
            if (suspend(s->addr))
                return true;
            // No NMI acknowledge, so there is no saved context: keep the
            // registers from the last good save.
            return false;
        }
    }
}

void PinsZ380::run() {
    _regs->restore();
    Cycles::reset();
    saveBreakInsts();
    startRunTimer();
    const auto stopped = loop();
    stopRunTimer();
    restoreBreakInsts();
    if (stopped) {
        _holdRing = true;
        _regs->save();
        _holdRing = false;
    }
    disassembleCycles();
}

void PinsZ380::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, InstZ380::RST38);
}

void PinsZ380::assertInt(uint8_t) {
    digitalWriteFast(PIN_INT0, LOW);
}

void PinsZ380::negateInt(uint8_t) {
    digitalWriteFast(PIN_INT0, HIGH);
}

void PinsZ380::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

// The program's code as the CPU fetched it: the ring's reads first, since
// the program may have written over its code since (the exit's RST 38H
// over a mode 1 vector), then a breakpoint as RST 38H.
struct PinsZ380::CodeMemory final : InstZ380::Memory {
    explicit CodeMemory(const PinsZ380 *pins) : _pins(pins), _bytes(0) {
        const auto g = Signals::get();
        const auto cycles = g->diff(Signals::put());
        for (auto i = 0u; i < cycles; ++i) {
            const auto s = g->next(i);
            if (!memRead(s))
                continue;
            const auto addr = s->addr & MemsZ380::ADDR_MASK;
            if (s->wordAccess()) {
                remember(addr & ~UINT32_C(1), s->data >> 8);
                remember(addr | 1, s->data);
            } else {
                remember(addr, (addr & 1) ? s->data : s->data >> 8);
            }
        }
    }

    uint16_t read_byte(uint32_t addr) const override {
        addr &= MemsZ380::ADDR_MASK;
        const auto i = find(addr);
        if (i < _bytes && _addr[i] == addr)
            return _byte[i];
        return _pins->isBreakPoint(addr) ? InstZ380::RST38
                                         : _pins->_mems->read_byte(addr);
    }

private:
    const PinsZ380 *const _pins;
    // Sorted by address; a later read replaces an earlier one.
    uint32_t _addr[2 * Cycles::MAX_CYCLES];
    uint8_t _byte[2 * Cycles::MAX_CYCLES];
    uint_fast16_t _bytes;

    uint_fast16_t find(uint32_t addr) const {
        uint_fast16_t lo = 0, hi = _bytes;
        while (lo < hi) {
            const auto mid = (lo + hi) / 2;
            if (_addr[mid] < addr) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return lo;
    }

    void remember(uint32_t addr, uint8_t byte) {
        const auto i = find(addr);
        if (i == _bytes || _addr[i] != addr) {
            for (auto j = _bytes; j > i; --j) {
                _addr[j] = _addr[j - 1];
                _byte[j] = _byte[j - 1];
            }
            _addr[i] = addr;
            ++_bytes;
        }
        _byte[i] = byte;
    }
};

// fetch() isn't a live bus signal here (#M1 marks only acknowledges): the
// walk of the ring to the PC marks it, so that runs first.
bool PinsZ380::walkRing(const CodeMemory &memory) {
    const auto regs = this->regs<RegsZ380>();
    return InstZ380::walk(Signals::get(), Signals::put(), memory, regs->pc(),
            regs->extended(), regs->longWord());
}

const SignalsImpl *PinsZ380::findBacktraceStart() {
    const auto g = Signals::get();
    const CodeMemory memory(this);
    return backtraceStartFrom<Signals>(
            walkRing(memory) ? g->next(MatchWalker::shared().start()) : g,
            _lineLimit);
}

// One line per instruction, then its data transfers; fetches only when
// verbose, and every cycle the walk does not explain.
void PinsZ380::printBacktrace() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    const CodeMemory memory(this);
    const auto walked = walkRing(memory);
    const auto &walker = MatchWalker::shared();
    const auto lead = walked ? walker.start() : cycles;
    for (auto i = 0u; i < lead; ++i) {
        g->next(i)->print();
        idle();
    }
    if (!walked)
        return;
    mems<MemsZ380>()->setCode(&memory);
    for (auto n = 0u; n < walker.steps(); ++n) {
        if (!walker.interrupt(n))
            _mems->disassemble(walker.addr(n) & MemsZ380::ADDR_MASK, 1);
        const auto first = walker.first(n);
        for (auto i = first; i < first + walker.cycles(n); ++i) {
            const auto s = g->next(i);
            if (walker.owner(i) == n && (s->isOperand() || Debugger.verbose()))
                s->print();
        }
        idle();
    }
    mems<MemsZ380>()->setCode(nullptr);
    for (auto i = lead; i < cycles; ++i) {
        if (walker.owner(i) == MatchWalker::NOBODY) {
            g->next(i)->print();
            idle();
        }
    }
}

}  // namespace z380
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
