#include "pins_mc68hc12.h"
#include "debugger.h"
#include "devs_mc68hc12.h"
#include "inst_mc68hc12.h"
#include "mc68hc12_init.h"
#include "mems_mc68hc12.h"
#include "queue_mc68hc12.h"
#include "regs_mc68hc12.h"
#include "signals_mc68hc12.h"

namespace debugger {
namespace mc68hc12 {

// clang-format off
/**
 * MC68HC912BD32 bus cycle, special expanded wide mode, DIVBYP=H: E is
 * EXTAL/2, so one EXTAL edge of each pair (the active one) moves E.
 *          __    __    __    __    __    __    __    __
 *  EXTAL _|a |q_|a |q_|a |q_|a |q_|a |q_|a |q_|a |q_|a |q_
 *           \_____\     \_____\     \_____\     \_____\
 *      E ___/      \_____/     \_____/     \_____/     \___
 *        __ ________ ___________ ________ ___________ ____
 *    R/W __X________X___________X________X___________X____
 *        ______    _______         _____    _______
 *     AD ______>--<_addr__>-<_R___>-----<_addr__>-<_W____>
 *                             ______                 ______
 *   #DBE ___________________/      \________________/
 *
 * - The address, R/W, #LSTRB and the queue movement are valid before the
 *   E rise; the execution start before the E fall.
 * - #DBE goes low a quarter cycle after the E rise, for an external read
 *   only: the debugger drives AD then, and only then. Internal accesses
 *   show on the bus (IVIS) but are the CPU's own.
 * - Read data is needed 30ns before the E fall; write data is valid 47ns
 *   after the E rise.
 * - Until MISC clears it, an external access stretches E high for three
 *   more cycles, #DBE low only in the last quarter.
 */
// clang-format on

namespace {

// The data sheet gives no EXTAL to E delay, so E is polled after each
// active edge, up to e_wait_ns; the chip is static, so the margins cost
// only speed.
constexpr auto extal_ns = 25;    // an EXTAL half period, at least
constexpr auto addr_ns = 60;     // quiet edge to address, R/W, #LSTRB
constexpr auto dbe_ns = 60;      // quiet edge to #DBE, write data
constexpr auto setup_ns = 40;    // read data to the E fall
constexpr auto e_poll_ns = 5;    // between polls of E
constexpr auto e_wait_ns = 200;  // active edge to E, at most

// Where the CPU waits between injected sequences, and where it stores what
// it reads for the debugger: both outside, away from any program.
constexpr uint16_t PARK = 0xFF80;

uint8_t active_level = HIGH;

inline void active_edge() {
    digitalWriteFast(PIN_EXTAL, active_level);
}

inline void quiet_edge() {
    digitalWriteFast(PIN_EXTAL, !active_level);
}

inline auto clock_e() {
    return digitalReadFast(PIN_E);
}

// Whether E reaches |level| soon after an edge.
bool wait_e(uint8_t level) {
    for (auto t = 0; t < e_wait_ns; t += e_poll_ns) {
        if (clock_e() == level)
            return true;
        delayNanoseconds(e_poll_ns);
    }
    return clock_e() == level;
}

inline auto reset_signal() {
    return digitalReadFast(PIN_RESET);
}

inline void assert_reset() {
    digitalWriteFast(PIN_RESET, LOW);
    pinMode(PIN_RESET, OUTPUT_OPENDRAIN);
}

inline void negate_reset() {
    pinMode(PIN_RESET, INPUT_PULLUP);
}

// Half an E cycle, when E runs.
void edge_pair() {
    quiet_edge();
    delayNanoseconds(extal_ns);
    const auto e = clock_e();
    active_edge();
    wait_e(!e);
}

// Finds which EXTAL level moves E and leaves the clock right after an E
// fall. Returns false when E doesn't move.
bool sync_e() {
    auto level = digitalReadFast(PIN_EXTAL);
    auto e = clock_e();
    for (auto n = 0; n < 16; ++n) {
        level = !level;
        digitalWriteFast(PIN_EXTAL, level);
        if (wait_e(!e)) {
            active_level = level;
            for (auto i = 0; clock_e() != LOW && i < 8; ++i)
                edge_pair();
            return clock_e() == LOW;
        }
    }
    return false;
}

constexpr uint8_t PINS_LOW[] = {
        PIN_EXTAL,
        PIN_BKGD,  // special mode
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_DIVBYP,  // E = EXTAL/2
        PIN_MODA,    // special expanded wide
        PIN_MODB,
        PIN_IRQ,
        PIN_XIRQ,
        PIN_RXD,
};

constexpr uint8_t PINS_INPUT[] = {
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
        PIN_DBE,
        PIN_RW,
        PIN_LSTRB,
        PIN_E,
        PIN_TXD,
        PIN_PS2,
        PIN_PS3,
        PIN_PS4,
        PIN_PS5,
        PIN_PS6,
        PIN_PS7,
        PIN_PT7,
        PIN_PT0,
};

// After reset MODA/MODB are IPIPE0/IPIPE1, and BKGD the BDM pin.
void release_mode() {
    pinMode(PIN_MODA, INPUT);
    pinMode(PIN_MODB, INPUT);
    pinMode(PIN_BKGD, INPUT_PULLDOWN);
}

}  // namespace

PinsMc68hc12::PinsMc68hc12(Mc68hc12Init &init) : _init(init), _park(PARK) {
    auto regs = new RegsMc68hc12(this, init);
    _regs = regs;
    _devs = new DevsMc68hc12(init);
    _mems = new MemsMc68hc12(regs, _devs, init);
}

void PinsMc68hc12::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    Signals::inputMode();
    _xirq = false;
    _held = false;
    _done = false;
    _stretch = true;
    _writes = 0;

    // E runs in reset: find its phase there, before any bus cycle.
    assert_reset();
    const auto synced = sync_e();
    // #RESET low for a while, then the MCU may hold it longer (a power-on
    // reset takes 4096 cycles). Each edge pair is an E half cycle.
    for (auto i = 0; i < 64 * 2; ++i)
        edge_pair();
    negate_reset();
    for (auto i = 0; reset_signal() == LOW && i < 20000; ++i)
        edge_pair();
    // The mode pins are latched at the #RESET rise; MODA/MODB turn into
    // outputs right after it.
    release_mode();
    if (!synced && !sync_e())
        cli.println("?no E clock");
    if (clock_e() != LOW)
        edge_pair();
    Cycles::reset();

    // The reset vector (VfPPP) points at the park.
    static constexpr uint8_t BRA_HERE[] = {
            InstMc68hc12::BRA, InstMc68hc12::BRA_HERE};
    const uint8_t vec[] = {hi(PARK), lo(PARK)};
    const Window win{InstMc68hc12::VEC_RESET, vec, sizeof(vec)};
    execute(PARK, BRA_HERE, sizeof(BRA_HERE), nullptr, 0, EXIT_PARK, &win,
            false);
    _init.configSystem(regs<RegsMc68hc12>());
    _stretch = false;
    _regs->reset();
    _regs->save();
    _regs->setIp(_mems->read16(InstMc68hc12::VEC_RESET));
}

Signals *PinsMc68hc12::prepareCycle() {
    auto s = Signals::put();
    if (_held) {
        _held = false;
        *s = _heldSignals;
        return s;
    }
    // E low
    quiet_edge();
    delayNanoseconds(addr_ns);
    s->getAddr();
    active_edge();
    if (!wait_e(HIGH)) {
        // STOP freezes E until an interrupt; walk until it comes back.
        for (auto n = 0; clock_e() == LOW; ++n) {
            if (n >= 1000)
                return noBusCycle(s);
            edge_pair();
        }
        // It came back high: wait for the next cycle.
        for (auto n = 0; clock_e() != LOW && n < 8; ++n)
            edge_pair();
        return prepareCycle();
    }
    // E high
    delayNanoseconds(extal_ns);
    quiet_edge();
    delayNanoseconds(dbe_ns);
    s->getDbe();
    if (_stretch && s->read() && !s->external()) {
        // A stretched read asserts #DBE late; a free or internal one not
        // at all, and ends with E.
        for (auto n = 0; n < 8; ++n) {
            s->getStart();
            active_edge();
            if (wait_e(LOW)) {
                _done = true;
                return s;
            }
            quiet_edge();
            delayNanoseconds(dbe_ns);
            s->getDbe();
            if (s->external())
                break;
        }
    }
    return s;
}

Signals *PinsMc68hc12::completeCycle(Signals *s) {
    if (_done) {
        _done = false;
    } else if (!s->none()) {
        const auto drive = s->external();
        if (drive) {
            if (s->readMemory())
                s->data = readBus(s);
            s->outData();
            delayNanoseconds(setup_ns);
        } else if (s->write()) {
            s->getData();
        }
        s->getStart();
        active_edge();
        // A stretched cycle keeps E high; the data stays until E falls.
        for (auto n = 0; !wait_e(LOW) && n < 8; ++n) {
            quiet_edge();
            delayNanoseconds(dbe_ns);
            if (s->write())
                s->getData();
            s->getStart();
            active_edge();
        }
        if (drive)
            Signals::inputMode();
        if (s->write() && s->writeMemory())
            writeBus(s);
    }
    Cycles::next();
    return s;
}

// Gives up on a cycle rather than wedge the board, leaving the halt
// switch to stop the run.
Signals *PinsMc68hc12::noBusCycle(Signals *s) {
    cli.println("?halt: no bus cycle");
    s->noCycle();
    return s;
}

// Keeps the CPU waiting in the read |s|, at |park|; the next
// prepareCycle() takes it up.
void PinsMc68hc12::hold(const Signals *s, uint16_t park) {
    _heldSignals = *s;
    _heldSignals.clear();
    _held = true;
    _park = park;
}

// Only the bytes the cycle moves are read: a device may count reads.
uint16_t PinsMc68hc12::readBus(const Signals *s) const {
    uint16_t word = 0;
    for (uint_fast8_t i = 0; i < s->bytes(); ++i) {
        const uint16_t addr = s->addr + i;
        const uint8_t b = _mems->read(addr);
        word |= (addr & 1) ? b : (b << 8);
    }
    return word;
}

void PinsMc68hc12::writeBus(const Signals *s) const {
    for (uint_fast8_t i = 0; i < s->bytes(); ++i) {
        const uint16_t addr = s->addr + i;
        _mems->write(addr, s->byteAt(addr));
    }
}

uint16_t PinsMc68hc12::Capture::frame(uint8_t *buf, uint8_t len) const {
    uint16_t base = UINT16_MAX;
    for (uint_fast8_t i = 0; i < n; ++i) {
        if (addr[i] < base)
            base = addr[i];
    }
    for (uint_fast8_t i = 0; i < len; ++i)
        buf[i] = 0;
    for (uint_fast8_t i = 0; i < n; ++i) {
        const uint16_t off = addr[i] - base;
        if (off < len)
            buf[off] = data[i];
    }
    return base;
}

void PinsMc68hc12::execute(uint16_t org, const uint8_t *inst, uint8_t len,
        Capture *cap, uint8_t max, uint32_t exit, const Window *win,
        bool vector) {
    const uint16_t leaves = exit == EXIT_PARK ? org : exit;
    // The exit counts once the window's last byte was read: a sequence
    // that branches back to its origin reads it first.
    bool whole = len == 0;
    // Once a vector was read, the CPU fetches at |leaves|, not |inst|.
    bool vectored = false;
    // A window inside is read by the CPU itself, never on the bus.
    bool seen = win == nullptr || _init.is_internal(win->at);
    uint16_t seenBits = 0;
    uint_fast8_t caps = 0;
    if (cap)
        cap->n = 0;
    // Bound the damage: a lost CPU never reads the exit.
    for (auto guard = 0; guard < 300; ++guard) {
        auto s = prepareCycle();
        if (s->none())
            break;
        if (s->external()) {
            const uint16_t from = s->addr;
            const uint16_t to = from + s->bytes();
            if (whole && seen && caps >= max &&
                    static_cast<uint16_t>(leaves - from) <
                            static_cast<uint16_t>(to - from)) {
                hold(s, leaves);
                return;
            }
            uint16_t word = 0;
            for (uint_fast8_t i = 0; i < s->bytes(); ++i) {
                const uint16_t addr = from + i;
                const uint16_t off = addr - org;
                const uint16_t w = win ? addr - win->at : UINT16_MAX;
                const uint16_t tail = addr - leaves;
                uint8_t b = InstMc68hc12::NOP;  // a fetch ahead
                if (!vectored && off < len) {
                    b = inst[off];
                    if (off == len - 1u)
                        whole = true;
                } else if (win && w < win->len) {
                    b = win->bytes[w];
                    seenBits |= 1 << w;
                    if (seenBits == (1 << win->len) - 1)
                        seen = true;
                    if (vector)
                        vectored = whole = true;
                } else if (tail < 2) {
                    b = tail == 0 ? InstMc68hc12::BRA : InstMc68hc12::BRA_HERE;
                }
                word |= (addr & 1) ? b : (b << 8);
            }
            s->inject(word);
            completeCycle(s);
        } else if (s->write()) {
            s->capture();
            completeCycle(s);
            for (uint_fast8_t i = 0; i < s->bytes(); ++i, ++caps) {
                if (cap && cap->n < Capture::MAX) {
                    const uint16_t addr = s->addr + i;
                    cap->addr[cap->n] = addr;
                    cap->data[cap->n] = s->byteAt(addr);
                    cap->n++;
                }
            }
        } else {
            completeCycle(s);
        }
    }
    cli.print("?lost the CPU running at ");
    cli.printHex(org, 4);
    cli.print(" to ");
    cli.printlnHex(leaves, 4);
    _held = false;
}

void PinsMc68hc12::execInst(const uint8_t *inst, uint8_t len, Capture *cap,
        uint8_t max, uint32_t exit, const Window *win) {
    const auto vector = win && win->at >= InstMc68hc12::VEC_IRQ;
    execute(_park, inst, len, cap, max, exit, win, vector);
}

// |s| reads an exception vector: answer the park, capture the context the
// CPU stacks, and park there. |stacked| gets the stacked bytes.
void PinsMc68hc12::exception(Signals *s, bool breakTrap, Capture *stacked) {
    static constexpr uint8_t BRA_HERE[] = {
            InstMc68hc12::BRA, InstMc68hc12::BRA_HERE};
    const uint8_t vec[] = {hi(PARK), lo(PARK)};
    const Window win{static_cast<uint16_t>(s->addr), vec, sizeof(vec)};
    uint8_t frame[RegsMc68hc12::FRAME];
    uint16_t sp;
    hold(s, s->addr);
    if (_writes >= RegsMc68hc12::FRAME) {
        // WAI stacked the context before it waited.
        sp = _stacked.frame(frame, sizeof(frame));
        if (stacked)
            *stacked = _stacked;
        execute(PARK, BRA_HERE, sizeof(BRA_HERE), nullptr, 0, EXIT_PARK, &win,
                false);
    } else {
        Capture cap;
        execute(PARK, BRA_HERE, sizeof(BRA_HERE), &cap, RegsMc68hc12::FRAME,
                EXIT_PARK, &win, false);
        sp = cap.frame(frame, sizeof(frame));
        if (stacked)
            *stacked = cap;
    }
    regs<RegsMc68hc12>()->capture(sp, frame, breakTrap);
}

// Remembers the writes since the last external read: WAI's stacking.
void PinsMc68hc12::track(const Signals *s) {
    if (s->external()) {
        _writes = 0;
        _stacked.n = 0;
    } else if (s->write()) {
        for (uint_fast8_t i = 0; i < s->bytes(); ++i) {
            const uint16_t addr = s->addr + i;
            if (_stacked.n < Capture::MAX) {
                _stacked.addr[_stacked.n] = addr;
                _stacked.data[_stacked.n] = s->byteAt(addr);
                _stacked.n++;
            }
            ++_writes;
        }
    }
}

const Signals *PinsMc68hc12::loop() {
    _writes = 0;
    _stacked.n = 0;
    uint16_t tryHalt = 0;
    while (true) {
        auto s = prepareCycle();
        if (s->none())
            return s;
        if (s->external()) {
            if (s->addr == InstMc68hc12::VEC_SWI) {
                // The queue may fetch a breakpoint's SWI long before it runs,
                // so every SWI stops, and the stacked PC tells which it was.
                Capture stacked;
                exception(s, true, &stacked);
                const auto handler = _mems->read16(InstMc68hc12::VEC_SWI);
                if (isBreakPoint(_regs->nextIp()) ||
                        handler == InstMc68hc12::VEC_SWI)
                    return s;
                // The program's own: put back what it stacked, and go on in
                // its handler.
                for (uint_fast8_t i = 0; i < stacked.n; ++i)
                    _mems->write(stacked.addr[i], stacked.data[i]);
                regs<RegsMc68hc12>()->vectored(handler);
                _regs->restore();
                continue;
            }
            if (tryHalt && s->addr == InstMc68hc12::VEC_XIRQ) {
                negate_xirq();
                exception(s, false);
                return s;
            }
        }
        completeCycle(s);
        track(s);
        _devs->loop();
        if (tryHalt == 0) {
            if (haltSwitch()) {
                assert_xirq();
                tryHalt = 1;
            }
        } else if (++tryHalt >= 5000) {
            cli.println("?halt: no #XIRQ");
            negate_xirq();
            resetPins();
            return Signals::put();
        }
    }
}

void PinsMc68hc12::run() {
    _regs->restore();
    Cycles::reset();
    saveBreakInsts();
    startRunTimer();
    const auto s = loop();
    stopRunTimer();
    Cycles::discard(s);
    restoreBreakInsts();
    disassembleCycles();
}

// #XIRQ taken after one instruction: restore() cleared CCR.X, and RTI has
// passed its pending interrupt check by the time it fetches the PC.
bool PinsMc68hc12::rawStep() {
    _regs->restore();
    Cycles::reset();
    _writes = 0;
    _stacked.n = 0;
    assert_xirq();
    for (auto n = 0; n < 200; ++n) {
        auto s = prepareCycle();
        if (s->none())
            break;
        if (s->external() && s->addr == InstMc68hc12::VEC_XIRQ) {
            negate_xirq();
            exception(s, false);
            Cycles::discard(s);
            return true;
        }
        completeCycle(s);
        track(s);
    }
    negate_xirq();
    cli.println("?step: no #XIRQ");
    return false;
}

bool PinsMc68hc12::step(bool show) {
    if (rawStep()) {
        if (show)
            printCycles();
        return true;
    }
    return false;
}

void PinsMc68hc12::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, InstMc68hc12::SWI);
}

void PinsMc68hc12::assertInt(uint8_t) {
    digitalWriteFast(PIN_IRQ, LOW);
}

void PinsMc68hc12::negateInt(uint8_t) {
    digitalWriteFast(PIN_IRQ, HIGH);
}

void PinsMc68hc12::assert_xirq() {
    digitalWriteFast(PIN_XIRQ, LOW);
    _xirq = true;
}

void PinsMc68hc12::negate_xirq() {
    digitalWriteFast(PIN_XIRQ, HIGH);
    _xirq = false;
}

void PinsMc68hc12::printCycles(const Signals *end) {
    const auto g = Signals::get();
    const auto cycles = g->diff(end ? end : Signals::put());
    Queue().replay(g, cycles);
    for (auto i = 0u; i < cycles; ++i)
        g->next(i)->print();
}

// The replay needs the queue filled from the start of the run, so it runs
// once, here, before the ring is cut; disposing keeps the marks.
const SignalsImpl *PinsMc68hc12::findBacktraceStart() {
    const auto g = Signals::get();
    Queue().replay(g, g->diff(Signals::put()));
    _replayed = true;
    return backtraceStartFrom<Signals>(g, _lineLimit);
}

// An instruction prints where the queue says it starts; the program words
// it was fetched in print only when verbose.
void PinsMc68hc12::printBacktrace() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    if (!_replayed)
        Queue().replay(g, cycles);
    _replayed = false;
    for (auto i = 0u; i < cycles; ++i) {
        const auto s = g->next(i);
        if (s->fetch())
            _mems->disassemble(s->inst(), 1);
        if (!s->program() || Debugger.verbose())
            s->print();
    }
}

}  // namespace mc68hc12
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
