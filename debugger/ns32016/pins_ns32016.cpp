#include "pins_ns32016.h"
#include "debugger.h"
#include "devs_ns32016.h"
#include "digital_bus.h"
#include "inst_ns32016.h"
#include "mems_ns32016.h"
#include "regs_ns32016.h"

namespace debugger {
namespace ns32016 {

// clang-format off
/**
 * NS32016 bus cycle, no address translation (datasheet 3.4, Figures 3-7,
 * 3-8 and 3-9).
 *
 *       | T4  | T1  | T2  | T3  | T3  | T4  |
 *       ________________________________________  status for this cycle
 * ST    X_____X___________________________X_____
 *             __
 * #ADS  _____/  \_______________________________  (inverted: low is on)
 *                    __________________
 * #DS   ____________/                  \________  (inverted)
 *              _____       ____________
 * AD    ------<_adr_>-----<_read data__>--------  the board drives
 *              _____ _________________________
 * AD    ------<_adr_X__write data_____________>-  the CPU drives
 *                  RDY ^     ^
 *
 * CLK runs free from the PWM: the CPU is dynamic and its clock must never
 * stop. PHI1 and PHI2 come from CLK through a cross-coupled 74LVC02.
 * AD0-AD15 carry the address only around the #ADS pulse: valid 25 ns
 * before it rises and held 15 ns after, so AD is read while #ADS is
 * polled. A16-A23, the status, #DDIN and #HBE then hold to the next T1.
 *
 * RDY rests low, so every cycle waits in T3 until it is served. RDY is
 * sampled at the end of T2 and of each T3; once raised the CPU takes read
 * data at the end of the next T3 and raises #DS at T4. RDY drops again
 * at that edge, three clocks before the next cycle's first sample, and
 * read data goes off the bus with it.
 *
 * Slave cycles to the FPU strobe #SPC, not #ADS, and ignore RDY: the
 * Teensy never sees them.
 */
// clang-format on

namespace {

// 150 MHz / 16: CLK halves of 8 counts each. An odd count makes the
// halves, and so PHI1 and PHI2, differ by 6.7 ns, past the +-5 ns the
// datasheet allows.
constexpr uint32_t clk_hz = 9375000;
constexpr uint32_t clk_min_hz = 4000000;  // tCp 250 ns at most
constexpr auto clk_half_min_ns = 45;      // tCp 100 ns at least
// #RST low for 64 clocks at least.
constexpr auto reset_us = 100;
// Teensy cycles: F_CPU_ACTUAL is 600 MHz.
constexpr uint32_t cycles_per_us = 600;
// A strobe comes within a few clocks of the one before.
constexpr uint32_t strobe_timeout = 20 * cycles_per_us;
// Instructions with no bus cycles run up to some hundred clocks, and an
// FPU operation longer.
constexpr uint32_t inst_timeout = 2000 * cycles_per_us;
// While running, how long without #ADS before checking for WAIT.
constexpr uint32_t idle_timeout = 100 * cycles_per_us;
// #ADS polls while it is low: it lasts some 50 ns.
constexpr auto ads_polls = 64;
// Both are maximums from T2's start with no minimums, so #DS may fall
// long before either: write data is valid within 50 ns, and AD floats
// within 25 ns. The cycle holds in T3 meanwhile.
constexpr auto write_data_ns = 55;
constexpr auto float_ns = 30;
// Bus cycles between halt switch polls while running.
constexpr auto halt_poll_cycles = 16;
// Bound the damage: a wrong dump beats a hung debugger.
constexpr auto inject_cycles = 512;
constexpr auto trap_cycles = 64;
constexpr auto step_cycles = 1024;

constexpr uint32_t ADS_bm = CNTL_ADS << CNTL_gp;
constexpr uint32_t DS_bm = CNTL_DS << CNTL_gp;

const uint8_t PINS_LOW[] = {
        PIN_RST,
        PIN_RDY,
        PIN_ASEL,
};

const uint8_t PINS_HIGH[] = {
        PIN_INT,
        PIN_NMI,
        PIN_HOLD,
};

const uint8_t PINS_INPUT[] = {
        PIN_ADS,
        PIN_DS,
        PIN_DDIN,
        PIN_HBE,
        PIN_PFS,
        PIN_US,
        PIN_HLDA,
        PIN_ST0,
        PIN_ST1,
        PIN_ST2,
        PIN_ST3,
        PIN_AH0,
        PIN_AH1,
        PIN_AH2,
        PIN_AH3,
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
};

inline void assert_rdy() {
    digitalWriteFast(PIN_RDY, HIGH);
}

inline void negate_rdy() {
    digitalWriteFast(PIN_RDY, LOW);
}

// #NMI is taken on its falling edge: negate it once acknowledged.
void assert_nmi() {
    digitalWriteFast(PIN_NMI, LOW);
}

void negate_nmi() {
    digitalWriteFast(PIN_NMI, HIGH);
}

// Whether #DS reached |low| within the strobe timeout.
bool waitDs(bool low) {
    const auto start = ARM_DWT_CYCCNT;
    while (((GPIO9_PSR & DS_bm) == 0) != low) {
        if (ARM_DWT_CYCCNT - start >= strobe_timeout)
            return false;
    }
    return true;
}

uint8_t currentStatus() {
    return busRead(ST);
}

constexpr uint16_t lo16(uint32_t v) {
    return v;
}

constexpr uint16_t hi16(uint32_t v) {
    return v >> 16;
}

constexpr uint32_t uint32(uint16_t hi, uint16_t lo) {
    return (uint32_t(hi) << 16) | lo;
}

}  // namespace

PinsNs32016::PinsNs32016(bool fpu)
    : _fpu(fpu), _clock(clk_half_min_ns, clk_min_hz), _clockInput(clk_hz) {
    _devs = new DevsMc6800(ACIA_BASE_NS32016);
    _mems = new MemsNs32016(fpu);
    _regs = new RegsNs32016(this, mems<MemsNs32016>());
}

RegsNs32016 *PinsNs32016::regs() const {
    return Pins::regs<RegsNs32016>();
}

// The clock starts with #RST low and runs from then on. #RST must rise
// while PHI1 is high, which is CLK's low half; the CPU then fetches from
// 0 with PSR 0, in supervisor mode with interrupts off.
void PinsNs32016::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    _clock.begin(_clockInput);
    _clock.start();
    delayMicroseconds(reset_us);

    Cycles::reset();
    noInterrupts();
    // Past the high half's counts and the NOR's delay, well before the
    // next rise.
    const auto period = _clock.period();
    const auto start = ARM_DWT_CYCCNT;
    while (true) {
        const auto count = _clock.count();
        if (count >= period * 5 / 8 && count < period * 6 / 8)
            break;
        if (ARM_DWT_CYCCNT - start >= strobe_timeout)
            break;
    }
    digitalWriteFast(PIN_RST, HIGH);
    auto s = prepareCycle(strobe_timeout);
    interrupts();
    if (s == nullptr || !s->fetch() || s->addr != 0) {
        cli.print("?reset: fetch ");
        if (s) {
            s->print();
        } else {
            cli.println();
        }
        return;
    }
    park(s);
    regs()->reset();
}

// The CPU waits in the parked cycle, RDY low, while the debugger works.
void PinsNs32016::park(Signals *s) {
    _parked.copyControl(s);
    interrupts();
}

Signals *PinsNs32016::resumeCycle() {
    auto s = Signals::put();
    s->clear();
    s->copyControl(&_parked);
    noInterrupts();
    return s;
}

// The next cycle the CPU strobes #ADS for, null after |timeoutCycles|
// without one. Called with interrupts masked: one taken while #ADS is
// low would lose the address.
Signals *PinsNs32016::prepareCycle(uint32_t timeoutCycles) {
    auto s = Signals::put();
    s->clear();
    const auto start = ARM_DWT_CYCCNT;
    while (GPIO9_PSR & ADS_bm) {
        if (ARM_DWT_CYCCNT - start >= timeoutCycles)
            return nullptr;
    }
    // The last read while #ADS is low, or the first after it rose, within
    // the address's 15 ns hold.
    uint32_t gpio6 = GPIO6_PSR;
    for (auto guard = ads_polls; guard && (GPIO9_PSR & ADS_bm) == 0; --guard)
        gpio6 = GPIO6_PSR;
    s->getAddr(gpio6);
    s->getControl();
    return s;
}

Signals *PinsNs32016::prepareCycle() {
    return prepareCycle(inst_timeout);
}

// Serves |s| and ends it, leaving interrupts masked for the next
// prepareCycle(): RDY drops the moment #DS rises, three clocks before the
// next cycle samples it.
Signals *PinsNs32016::completeCycle(Signals *s) {
    const auto read = s->read();
    if (read) {
        serveRead(s);
        waitDs(true);
        delayNanoseconds(float_ns);
        s->outData();
    } else {
        waitDs(true);
        delayNanoseconds(write_data_ns);
        s->getData();
        storeWrite(s);
    }
    noInterrupts();
    assert_rdy();
    const auto ended = waitDs(false);
    negate_rdy();
    if (read)
        s->inputMode();
    if (!ended) {
        interrupts();
        cli.println("?bus: #DS stuck low");
    }
    Cycles::next();
    return s;
}

// Memory, the ACIA, or an acknowledge's byte on AD0-AD7: 0 is NVI's
// vector, ignored for NMI, and a non-negative byte for an end of
// interrupt, so no cascade follows. A read gets the whole aligned word;
// the CPU takes the lanes it enabled.
void PinsNs32016::serveRead(Signals *s) {
    if (!s->readMemory())
        return;
    if (s->intAck() || s->eoi()) {
        s->data = 0;
    } else if (_devs->isSelected(s->addr)) {
        const uint16_t v = _devs->read(s->addr) & 0xFF;
        s->data = v | (v << 8);
    } else {
        s->data = mems<MemsNs32016>()->read_bus(s->addr);
    }
}

void PinsNs32016::storeWrite(Signals *s) {
    if (!s->writeMemory() || !s->memory())
        return;
    const auto mems = this->mems<MemsNs32016>();
    if (_devs->isSelected(s->addr)) {
        _devs->write(s->addr, s->lowLane() ? lo(s->data) : hi(s->data));
    } else if (s->wordAccess()) {
        mems->write_bus(s->addr, s->data);
    } else if (s->lowLane()) {
        mems->write_byte(s->addr, lo(s->data));
    } else {
        mems->write_byte(s->addr, hi(s->data));
    }
}

uint_fast8_t PinsNs32016::execute(const uint8_t *inst, uint_fast8_t len,
        uint8_t *buf, uint_fast8_t max, uint32_t &org, uint32_t exit,
        const Keyed *keys, uint_fast8_t nkeys) {
    const auto base = org;
    const auto byteAt = [inst, len, base](uint32_t a) -> uint16_t {
        const uint32_t off = a - base;
        return off < len ? inst[off] : InstNs32016::NOP;
    };
    uint_fast8_t cap = 0;
    auto s = resumeCycle();
    for (auto guard = inject_cycles; s && guard; --guard) {
        // The parked cycle is a fetch of |org|; any later non-sequential
        // fetch of |exit| ends the sequence.
        if (guard != inject_cycles && s->fetch() && (s->addr ^ exit) <= 1) {
            park(s);
            org = exit;
            return cap;
        }
        const uint32_t a = s->addr & ~1;
        if (s->read()) {
            uint16_t word = 0;
            if (s->fetching()) {
                word = byteAt(a) | (byteAt(a + 1) << 8);
            } else {
                for (uint_fast8_t i = 0; i < nkeys; ++i) {
                    if (keys[i].addr == a) {
                        word = keys[i].data;
                        break;
                    }
                }
            }
            s->inject(word);
        } else {
            s->capture();
        }
        completeCycle(s);
        if (s->write() && buf) {
            if (s->lowLane() && cap < max)
                buf[cap++] = lo(s->data);
            if (s->highLane() && cap < max)
                buf[cap++] = hi(s->data);
        }
        s = prepareCycle();
    }
    if (s)
        park(s);
    interrupts();
    cli.println("?inject: no exit");
    return cap;
}

void PinsNs32016::execInst(const uint8_t *inst, uint_fast8_t len,
        uint32_t &org, uint32_t exit, const Keyed *keys, uint_fast8_t nkeys) {
    execute(inst, len, nullptr, 0, org, exit, keys, nkeys);
}

uint_fast8_t PinsNs32016::captureWrites(const uint8_t *inst, uint_fast8_t len,
        uint8_t *buf, uint_fast8_t max, uint32_t &org, uint32_t exit) {
    return execute(inst, len, buf, max, org, exit, nullptr, 0);
}

namespace {
// A word read the CPU does for itself, not a fetch.
bool dataRead(const Signals *s) {
    return s->memory() && s->read() && !s->fetching();
}

// The lower of two word reads of one dword, either order.
const Signals *lowWord(const Signals *x, const Signals *y) {
    if (y->addr == x->addr + 2)
        return x;
    if (x->addr == y->addr + 2)
        return y;
    return nullptr;
}
}  // namespace

// Whether |s| reads the program base of a trap or interrupt entry: after
// the PSR push, the dispatch entry's dword (MOD low, offset high), then
// SB at that module and its program base at MOD+8 (datasheet 3.8.7,
// Figure 3-28). No INTBASE needed: the program may have moved it.
// ATTENTION: CXP and RXP read a link entry, SB and a program base in the
// same shape, with pushes around them; a program calling external
// procedures may be taken for a trap. Unbenched.
bool PinsNs32016::trapEntry(const Signals *s, Entry &entry) const {
    if (!dataRead(s) || Signals::get()->diff(s) < 5)
        return false;
    const auto push = s->prev(5);
    if (push->read() || !push->memory())
        return false;
    for (uint_fast8_t i = 1; i <= 4; ++i) {
        if (!dataRead(s->prev(i)))
            return false;
    }
    const auto module = lowWord(s->prev(4), s->prev(3));
    const auto sb = lowWord(s->prev(2), s->prev(1));
    if (module == nullptr || sb == nullptr)
        return false;
    const auto offset = module == s->prev(4) ? s->prev(3) : s->prev(4);
    const uint32_t mod = module->data;
    if (sb->addr != mod || ((s->addr & ~1) != mod + 8 && (s->addr & ~1) != mod + 10))
        return false;
    entry.push = push;
    entry.mod = mod;
    entry.offset = offset->data;
    const auto ack = Signals::get()->diff(push) >= 1 ? push->prev() : nullptr;
    entry.ack = (ack && ack->intAck()) ? ack : nullptr;
    return true;
}

// |s| reads a trap's program base, after |entry|. It is answered so the
// handler lands on the park, which is BR to itself, the MOD and return PC
// pushes below the PSR are stored as the trap does, and the CPU parks on
// the handler's fetch once they are; the ring drops the trap. |frame|
// gets the program's PC, MOD and PSR.
bool PinsNs32016::takeTrap(Signals *s, const Entry &entry, Frame &frame) {
    const uint16_t psr = entry.push->data;
    const uint32_t psrAt = entry.push->addr & ~1;
    const uint32_t baseAt = entry.mod + 8;
    const auto mems = this->mems<MemsNs32016>();
    _handler = mems->read32(baseAt) + entry.offset;
    const uint32_t base = InstNs32016::PARK_PC - entry.offset;
    const auto from = entry.ack ? entry.ack : entry.push;
    constexpr auto park_pc = InstNs32016::PARK_PC;
    uint16_t mod = 0, pcLo = 0, pcHi = 0;
    uint_fast8_t pushes = 0;
    for (auto guard = trap_cycles; s && guard; --guard) {
        const uint32_t a = s->addr & ~1;
        if (s->read()) {
            if (s->fetching()) {
                if (s->fetch() && a == park_pc && pushes >= 3) {
                    park(s);
                    Cycles::discard(from);
                    frame = Frame{uint32(pcHi, pcLo), mod, psr};
                    return true;
                }
                s->inject(a == park_pc ? InstNs32016::BR : uint16(
                                  InstNs32016::NOP, InstNs32016::NOP));
            } else if (a == baseAt) {
                s->inject(lo16(base));
            } else if (a == baseAt + 2) {
                s->inject(hi16(base));
            }
        }
        completeCycle(s);
        if (s->write()) {
            if (a == psrAt - 2) {
                mod = s->data;
                ++pushes;
            } else if (a == psrAt - 6) {
                pcLo = s->data;
                ++pushes;
            } else if (a == psrAt - 4) {
                pcHi = s->data;
                ++pushes;
            }
        }
        s = prepareCycle();
    }
    if (s)
        park(s);
    interrupts();
    cli.println("?trap: no park");
    return false;
}

// Parked at a trap's handler: on into the program's own, as the trap
// would have gone. MOD and SB are already the handler's.
void PinsNs32016::forwardTrap() {
    uint8_t jump[6] = {0x7F, 0xAA};  // JUMP @handler
    const uint32_t d = 0xC0000000 | _handler;
    jump[2] = d >> 24;
    jump[3] = d >> 16;
    jump[4] = d >> 8;
    jump[5] = d;
    uint32_t org = InstNs32016::PARK_PC;
    execInst(jump, sizeof(jump), org, _handler);
}

// Takes #NMI on |s|, a prepared cycle, or on a CPU idle in WAIT: the
// program runs to the end of its instruction, then the acknowledge, the
// PSR push and the dispatch. A trap the program takes first, a BPT, stops
// it there; the program's own interrupts run on.
bool PinsNs32016::suspend(Signals *s) {
    noInterrupts();
    _nmiPending = true;
    assert_nmi();
    for (auto guard = step_cycles; guard; --guard) {
        if (s == nullptr)
            s = prepareCycle();
        if (s == nullptr)
            break;
        Entry entry;
        if (trapEntry(s, entry) && (entry.ack == nullptr || nmiAck(entry.ack))) {
            negate_nmi();
            if (entry.ack)
                _nmiPending = false;
            Frame frame;
            if (!takeTrap(s, entry, frame))
                return false;
            regs()->parkInTrap(frame.pc, frame.mod, frame.psr);
            return true;
        }
        if (s->intAck() && nmiAck(s))
            negate_nmi();
        completeCycle(s);
        s = nullptr;
    }
    negate_nmi();
    interrupts();
    cli.println("?halt: no NMI acknowledge");
    return false;
}

// NMI's acknowledge reads FFFF00, INT's FFFE00 (datasheet Table 3-3).
bool PinsNs32016::nmiAck(const Signals *ack) const {
    return (ack->addr & ~0xFF) == 0xFFFF00;
}

bool PinsNs32016::step(bool show) {
    const auto regs = this->regs();
    // WAIT stops for an interrupt; stepping it would hang.
    if (_mems->read(regs->nextIp()) == InstNs32016::WAIT)
        return false;
    Cycles::reset();
    regs->restore(true);
    if (show)
        Cycles::reset();
    // The trace trap, or whatever the instruction took first.
    auto s = resumeCycle();
    for (auto guard = step_cycles; s && guard; --guard) {
        Entry entry;
        if (trapEntry(s, entry)) {
            Frame frame;
            if (!takeTrap(s, entry, frame))
                return false;
            regs->parkInTrap(frame.pc, frame.mod, frame.psr);
            if (show)
                printCycles();
            regs->save();
            return true;
        }
        completeCycle(s);
        s = prepareCycle();
    }
    interrupts();
    return false;
}

// Runs until a BPT, the halt switch, or an NMI left pending by an earlier
// halt. Other traps and the program's interrupts go to the program's
// handlers. Returns whether the CPU parked in a debugger trap.
bool PinsNs32016::loop() {
    auto s = resumeCycle();
    for (uint_fast8_t n = 0;; ++n) {
        Entry entry;
        if (trapEntry(s, entry)) {
            const auto nmi = entry.ack && _nmiPending && nmiAck(entry.ack);
            if (entry.ack == nullptr || nmi) {
                Frame frame;
                if (!takeTrap(s, entry, frame))
                    return false;
                if (nmi || _mems->read(frame.pc) == InstNs32016::BPT) {
                    _nmiPending = false;
                    regs()->parkInTrap(frame.pc, frame.mod, frame.psr);
                    return true;
                }
                forwardTrap();
                s = resumeCycle();
                continue;
            }
        }
        // The CPU waits in this cycle: safe to take interrupts.
        if ((n % halt_poll_cycles) == 0) {
            interrupts();
            _devs->loop();
            if (haltSwitch())
                return suspend(s);
        }
        completeCycle(s);
        s = prepareCycle(idle_timeout);
        while (s == nullptr) {
            // Only in WAIT can no cycle start while interrupts run: an
            // instruction without bus cycles, or one waiting for the FPU,
            // may strobe #ADS any time.
            if (currentStatus() == ST_WAIT) {
                interrupts();
                if (haltSwitch())
                    return suspend(nullptr);
                _devs->loop();
                noInterrupts();
            }
            s = prepareCycle(idle_timeout);
        }
    }
}

void PinsNs32016::run() {
    const auto regs = this->regs();
    // A BPT returns to itself: step off a breakpoint before planting it.
    if (isBreakPoint(regs->nextIp()) && !step(false))
        return;
    regs->restore(false);
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

void PinsNs32016::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, InstNs32016::BPT);
}

// A rate the CPU takes, as an even count of the bus clock, from the next
// reset: retuning the running clock of a dynamic CPU could cut a pulse.
bool PinsNs32016::setClockInput(uint32_t hz) {
    if (hz == 0)
        return false;
    uint32_t div = (F_BUS_ACTUAL + hz / 2) / hz;
    div += div & 1;
    const uint32_t rate = F_BUS_ACTUAL / div;
    if (rate < clk_min_hz ||
            uint64_t(div / 2) * 1000000000 < uint64_t(clk_half_min_ns) * F_BUS_ACTUAL)
        return false;
    _clockInput = rate;
    return true;
}

// #INT is level-sensitive; with CFG.I clear it takes vector 0.
void PinsNs32016::assertInt(uint8_t) {
    digitalWriteFast(PIN_INT, LOW);
}

void PinsNs32016::negateInt(uint8_t) {
    digitalWriteFast(PIN_INT, HIGH);
}

void PinsNs32016::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; ++i)
        g->next(i)->print();
}

// A non-sequential fetch starts each branch target; sequential fetches
// run ahead into the queue, so they mark no instruction.
const SignalsImpl *PinsNs32016::findBacktraceStart() {
    return backtraceStartByFetchCount<Signals>(_lineLimit);
}

// The data cycles, with the code at each branch target; every fetch when
// verbose.
void PinsNs32016::printBacktrace() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        const auto s = g->next(i);
        if (s->fetch()) {
            if (Debugger.verbose())
                s->print();
            _mems->disassemble(s->addr, 1);
        } else if (!s->fetching() || Debugger.verbose()) {
            s->print();
        }
    }
}

}  // namespace ns32016
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
