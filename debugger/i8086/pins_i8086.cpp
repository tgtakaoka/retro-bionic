#include "pins_i8086.h"
#include "debugger.h"
#include "devs_i8086.h"
#include "inst_i8086.h"
#include "mems_i8086.h"
#include "regs_i8086.h"
#include "signals_i8086.h"

namespace debugger {
namespace i8086 {

// clang-format off
/**
 * 8086 minimum mode bus cycle (8086 datasheet, Figure 13).
 *
 * The board drives CLK; everything the CPU drives follows its edges. A
 * T state starts with a falling edge. The debugger parks a cycle in T2
 * with the clock low, and serves it from there.
 *
 *         |     T1      |     T2      |     T3      |     T4      |
 *         \_______      \_______      \_______      \_______
 * CLK      |      /       |      /       |      /       |      /
 *              ____
 * ALE     ____/    \_______________________________________________
 *         ______________________________________________________
 * M/#IO   X_____________________________________________________X
 *          __________    ________________________________________
 * A16-19  X___addr___>--<_______S3-S6 status___________________>-
 *          __________                ____________________
 * AD      X___addr___>--------------<_____read data______>-------  board
 *          __________    _______________________________
 * AD      X___addr___>--<_______write data_____________>--------  CPU
 *         ________________                         ________________
 * #RD/#WR                 \_______________________/
 *
 * The address is valid from T1's falling edge, ALE falls after T1's
 * rising edge, and the address holds until T2 starts. #RD falls in T2,
 * once AD has floated, and rises in T4; the CPU takes read data on T4's
 * falling edge. Write data is valid from T2. READY stays high: no waits.
 *
 * An interrupt acknowledge is two such cycles with #INTA in place of #RD,
 * the vector on AD0-AD7 in the second. HLT shows one ALE with no strobe.
 */
// clang-format on

namespace {

// 80C86A-2: CLK high 44ns and low 68ns at least; the V20/V30 are slower.
constexpr auto clock_hi_ns = 50;
constexpr auto clock_lo_ns = 70;
// ALE and the address follow a falling edge by 60ns at most (110ns on the
// NMOS parts).
constexpr auto ale_delay_ns = 100;
// The strobes follow T2's falling edge by 100ns at most (165ns NMOS).
constexpr auto strobe_delay_ns = 120;
// #RD rises 80ns at most after T4's falling edge (150ns NMOS).
constexpr auto rd_release_ns = 120;
// Clocks without ALE before the CPU counts as halted; DIV takes about 190.
constexpr auto no_ale_clocks = 1024;
// Bus cycles between halt switch polls while running.
constexpr auto halt_poll_cycles = 16;
// Bus cycles allowed for an injected sequence and for a step.
constexpr auto exec_cycles = 512;
constexpr auto step_cycles = 1024;

const uint8_t PINS_LOW[] = {
        PIN_CLK,
        PIN_NMI,
        PIN_INTR,
        PIN_HOLD,
        PIN_TEST,  // lets WAIT fall through
};

const uint8_t PINS_HIGH[] = {
        PIN_RESET,
        PIN_READY,
        PIN_MNMX,  // minimum mode
};

const uint8_t PINS_INPUT[] = {
        PIN_ALE,
        PIN_DTR,
        PIN_DEN,
        PIN_RD,
        PIN_WR,
        PIN_INTA,
        PIN_BHE,
        PIN_MIO,
        PIN_HLDA,
        PIN_INT,
        PIN_A16S,
        PIN_A17S,
        PIN_A18S,
        PIN_A19S,
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

inline void clock_hi() {
    digitalWriteFast(PIN_CLK, HIGH);
}

inline void clock_lo() {
    digitalWriteFast(PIN_CLK, LOW);
}

// A whole period, from a falling edge; it ends high.
void clock_cycle() {
    clock_lo();
    delayNanoseconds(clock_lo_ns);
    clock_hi();
    delayNanoseconds(clock_hi_ns);
}

void assert_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
}

void negate_reset() {
    digitalWriteFast(PIN_RESET, LOW);
}

// NMI takes a rising edge, latched; negate once its vector was read.
void assert_nmi() {
    digitalWriteFast(PIN_NMI, HIGH);
}

void negate_nmi() {
    digitalWriteFast(PIN_NMI, LOW);
}

// digitalReadFast() may return the port bit, not 1: compare with LOW.
inline bool ale_asserted() {
    return digitalReadFast(PIN_ALE) != LOW;
}

constexpr uint32_t PARK =
        InstI8086::physical(InstI8086::PARK_CS, InstI8086::PARK_IP);

// The PARK vector's byte at |addr|, IP then CS.
uint8_t parkVector(uint32_t addr) {
    switch (addr & 3) {
    case 0:
        return uint8_t(InstI8086::PARK_IP);
    case 1:
        return uint8_t(InstI8086::PARK_IP >> 8);
    case 2:
        return uint8_t(InstI8086::PARK_CS);
    default:
        return uint8_t(InstI8086::PARK_CS >> 8);
    }
}

bool isVector(const Signals *s) {
    return s->code() && s->addr >= InstI8086::VECTORS &&
           s->addr < InstI8086::VECTORS_END;
}

}  // namespace

PinsI8086::PinsI8086() : PinsI8086(new DevsI8080(), false) {}

PinsI8086::PinsI8086(Devs *devs, bool fpu) {
    _devs = devs;
    _mems = new MemsI8086(_bus8, fpu);
    _regs = new RegsI8086(this);
}

void PinsI8086::Overlay::put(uint32_t addr, uint8_t data) {
    auto i = find(addr);
    if (i < 0) {
        if (_num >= MAX)
            return;
        i = _num++;
        _addr[i] = addr;
    }
    _data[i] = data;
    _read[i] = false;
}

int PinsI8086::Overlay::find(uint32_t addr) const {
    for (uint_fast8_t i = 0; i < _num; ++i) {
        if (_addr[i] == addr)
            return i;
    }
    return -1;
}

bool PinsI8086::Overlay::get(uint32_t addr, uint8_t &data) const {
    const auto i = find(addr);
    if (i < 0)
        return false;
    data = _data[i];
    return true;
}

uint16_t PinsI8086::Overlay::wordAt(uint32_t addr) const {
    uint8_t l = 0, h = 0;
    get(addr, l);
    get((addr + 1) & 0xFFFFF, h);
    return uint16(h, l);
}

void PinsI8086::Overlay::markRead(uint32_t addr) {
    const auto i = find(addr);
    if (i >= 0)
        _read[i] = true;
}

bool PinsI8086::Overlay::allRead() const {
    for (uint_fast8_t i = 0; i < _num; ++i) {
        if (!_read[i])
            return false;
    }
    return true;
}

// Reset starts at FFFF0H with no vector to read; the CPU parks in that
// fetch, and the registers are read there.
void PinsI8086::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    // RESET must be high 4 clocks at least, MN/#MX set before.
    assert_reset();
    for (auto i = 0; i < 16; i++)
        clock_cycle();
    Cycles::reset();
    negate_reset();
    // Read as an 8086's: M/#IO, the 8088's IO/#M, is high for memory.
    _bus8 = false;
    auto s = prepareCycle();
    parkAtReset(s, s && !s->memory());
}

bool PinsI8086::parkAtReset(Signals *s, bool bus8) {
    const auto reset =
            InstI8086::physical(InstI8086::RESET_CS, InstI8086::RESET_IP);
    _bus8 = bus8;
    if (s)
        s->setBus8(bus8);
    if (s == nullptr || !s->read() || !s->memory() || s->addr != reset) {
        cli.print("?reset: ");
        if (s)
            s->print();
        else
            cli.println();
        return false;
    }
    mems<MemsI8086>()->setBus8(bus8);
    auto regs = this->regs<RegsI8086>();
    regs->parkAt(InstI8086::RESET_CS, InstI8086::RESET_IP);
    regs->save();
    _nec = regs->probeVSeries();
    return true;
}

// Clocks to the next bus cycle and parks it in T2, its strobes asserted.
// Null when the CPU shows no ALE: halted.
Signals *PinsI8086::prepareCycle() {
    for (auto guard = no_ale_clocks; guard; --guard) {
        clock_lo();
        delayNanoseconds(ale_delay_ns);
        if (ale_asserted()) {
            auto s = Signals::put();
            s->getAddr(_bus8);  // T1
            clock_hi();         // ALE falls; the address holds
            delayNanoseconds(clock_hi_ns);
            clock_lo();  // T2
            delayNanoseconds(strobe_delay_ns);
            s->getControl();
            return s;
        }
        clock_hi();
        delayNanoseconds(clock_hi_ns);
    }
    return nullptr;
}

// Takes up a cycle parked at |addr|, the fetch every park is.
Signals *PinsI8086::resumeCycle(uint32_t addr) {
    auto s = Signals::put();
    s->addr = addr;
    s->setFetch(_bus8);
    return s;
}

Signals *PinsI8086::completeCycle(Signals *s) {
    if (s->halt()) {
        Cycles::next();
        return s;
    }
    if (s->read() || s->intAck()) {
        if (s->readMemory())
            retrieveData(s);
        s->outData();
        clock_hi();  // T2
        delayNanoseconds(clock_hi_ns);
        clock_lo();  // T3
        delayNanoseconds(clock_lo_ns);
        clock_hi();
        delayNanoseconds(clock_hi_ns);
        clock_lo();  // T4: the CPU takes the data
        delayNanoseconds(rd_release_ns);
        s->inputMode();
    } else {
        clock_hi();  // T2
        delayNanoseconds(clock_hi_ns);
        clock_lo();  // T3: the write data has settled
        delayNanoseconds(clock_lo_ns);
        s->getData();
        if (s->writeMemory())
            storeData(s);
        clock_hi();
        delayNanoseconds(clock_hi_ns);
        clock_lo();  // T4
        delayNanoseconds(clock_lo_ns);
    }
    clock_hi();
    delayNanoseconds(clock_hi_ns);
    Cycles::next();
    return s;
}

// A read's data from memory, a device, or an acknowledge's vector.
void PinsI8086::retrieveData(Signals *s) {
    if (s->intAck()) {
        s->data = _devs->vector();
    } else if (s->memory()) {
        if (_bus8) {
            s->data = _mems->read_byte(s->addr);
        } else {
            // Both lanes: the CPU takes the one #BHE and A0 pick.
            s->data = _mems->read16(s->addr & ~1);
        }
    } else if (_devs->isSelected(s->addr)) {
        const uint16_t v = _devs->read(s->addr);
        s->data = (_bus8 || s->wordAccess()) ? v : (v | (v << 8));
    } else {
        s->data = 0xFFFF;
    }
}

// A write's data to memory or a device.
void PinsI8086::storeData(Signals *s) {
    if (s->memory()) {
        if (s->wordAccess()) {
            _mems->write16(s->addr, s->data);
        } else {
            _mems->write_byte(s->addr, s->byteAt(s->addr));
        }
    } else if (_devs->isSelected(s->addr)) {
        _devs->write(s->addr, s->wordAccess() ? s->data : s->byteAt(s->addr));
    }
}

void PinsI8086::injectRead(
        Signals *s, const uint8_t *inst, uint_fast8_t len, uint32_t org) {
    auto injected = false;
    auto byteAt = [&](uint32_t a) -> uint8_t {
        const uint32_t off = (a - org) & 0xFFFFF;
        if (off < len) {
            injected = true;
            return inst[off];
        }
        uint8_t b;
        if (_overlay.get(a, b)) {
            injected = true;
            _overlay.markRead(a);
            return b;
        }
        return _mems->read_byte(a);
    };
    uint16_t data;
    if (s->wordAccess()) {
        const auto l = byteAt(s->addr);
        data = uint16(byteAt((s->addr + 1) & 0xFFFFF), l);
    } else {
        const auto b = byteAt(s->addr);
        data = _bus8 ? b : uint16(b, b);
    }
    // Memory alone is left to retrieveData().
    if (injected)
        s->inject(data);
}

void PinsI8086::execInst(
        const uint8_t *inst, uint_fast8_t len, uint32_t &org, uint32_t exit) {
    const uint32_t last = (org + len - 1) & 0xFFFFF;
    const auto start = org;
    auto whole = false;
    auto s = resumeCycle(org);
    for (auto guard = exec_cycles; s && guard; --guard) {
        if (whole && (exit == EXIT_OVERLAY ? _overlay.allRead()
                                           : (s->code() && s->addr == exit)))
            break;
        if (s->coproc()) {
            // The 8087's own operand: memory's, as when running.
        } else if (s->memory() && s->read()) {
            injectRead(s, inst, len, start);
            if (s->covers(last))
                whole = true;
        } else if (s->write()) {
            s->capture();
        }
        completeCycle(s);
        if (s->write() && s->memory() && !s->coproc()) {
            for (uint_fast8_t i = 0; i < 2; ++i) {
                const auto a = (s->addr + i) & 0xFFFFF;
                if (s->covers(a))
                    _overlay.put(a, s->byteAt(a));
            }
        }
        s = prepareCycle();
    }
    _next = s;
    if (s)
        org = s->addr;
}

// Keeps the last 6 bytes written: a trap's frame, once it parks.
void PinsI8086::recordFrame(const Signals *s) {
    for (uint_fast8_t i = 0; i < 2; ++i) {
        const auto a = (s->addr + i) & 0xFFFFF;
        if (!s->covers(a))
            continue;
        for (uint_fast8_t j = 1; j < 6; ++j) {
            _frameAt[j - 1] = _frameAt[j];
            _frame[j - 1] = _frame[j];
        }
        _frameAt[5] = a;
        _frame[5] = s->byteAt(a);
        if (_frameBytes < 6)
            _frameBytes++;
    }
}

// |s| is the fetch at PARK after a trap's vector: the frame below holds
// the program's IP, CS and FLAGS.
bool PinsI8086::parkInTrap(Signals *s) {
    if (_frameBytes < 6)
        return false;
    uint32_t base = _frameAt[0];
    for (uint_fast8_t i = 1; i < 6; ++i) {
        if (((_frameAt[i] - base) & 0xFFFFF) > 0x80000)
            base = _frameAt[i];
    }
    uint8_t bytes[6];
    for (uint_fast8_t k = 0; k < 6; ++k) {
        const auto a = (base + k) & 0xFFFFF;
        auto found = false;
        for (uint_fast8_t i = 0; i < 6; ++i) {
            if (_frameAt[i] == a) {
                bytes[k] = _frame[i];
                found = true;
            }
        }
        if (!found)
            return false;
    }
    const auto ip = uint16(bytes[1], bytes[0]);
    const auto cs = uint16(bytes[3], bytes[2]);
    const auto flags = uint16(bytes[5], bytes[4]);
    // Drop the debugger's part: the pushes and the vector reads.
    auto from = s;
    for (uint_fast8_t i = 1; i <= 16 && i <= Signals::get()->diff(s); ++i) {
        const auto t = s->prev(i);
        if (isVector(t) ||
                (t->write() && t->memory() && ((t->addr - base) & 0xFFFFF) < 6))
            from = t;
    }
    Cycles::discard(from);
    auto regs = this->regs<RegsI8086>();
    regs->parkInTrap(cs, ip, flags, _vector);
    return true;
}

// Completes cycles from |s| until the CPU takes one of the debugger's
// vectors and fetches at PARK. A halted CPU, or the halt switch while
// running, gets NMI.
bool PinsI8086::untilTrap(Signals *s, Mode mode) {
    _vector = 0;
    _frameBytes = 0;
    auto nmi = false;
    uint32_t guard = mode == STEP ? step_cycles : UINT32_MAX;
    for (uint32_t n = 0; guard; ++n, --guard) {
        if (s == nullptr) {
            if (nmi)
                break;
            assert_nmi();
            nmi = true;
            s = prepareCycle();
            continue;
        }
        if (isVector(s)) {
            _vector = s->addr >> 2;
            const auto b = parkVector(s->addr);
            if (s->wordAccess()) {
                s->inject(uint16(parkVector(s->addr + 1), b));
            } else {
                s->inject(_bus8 ? b : uint16(b, b));
            }
            if (_vector == InstI8086::VEC_NMI)
                negate_nmi();
        } else if (_vector && s->code() && s->addr == PARK) {
            negate_nmi();
            return parkInTrap(s);
        } else if (s->halt() && !nmi) {
            assert_nmi();
            nmi = true;
        }
        completeCycle(s);
        if (s->write() && s->memory() && !s->coproc())
            recordFrame(s);
        if (mode == RUN) {
            _devs->loop();
            if ((n % halt_poll_cycles) == 0 && !nmi && haltSwitch()) {
                assert_nmi();
                nmi = true;
            }
        }
        s = prepareCycle();
    }
    negate_nmi();
    return false;
}

bool PinsI8086::step(bool show) {
    // A HLT waits for an interrupt; stepping into it would hang.
    auto regs = this->regs<RegsI8086>();
    if (_mems->read(regs->nextIp()) == InstI8086::HLT)
        return false;
    Cycles::reset();
    {
        Cycles::Hold hold;
        regs->restore(true);
    }
    if (untilTrap(_next, STEP)) {
        if (show)
            printCycles();
        Cycles::Hold hold;
        regs->save();
        return true;
    }
    return false;
}

void PinsI8086::run() {
    Cycles::reset();
    {
        Cycles::Hold hold;
        _regs->restore();
    }
    saveBreakInsts();
    startRunTimer();
    const auto stopped = untilTrap(_next, RUN);
    stopRunTimer();
    restoreBreakInsts();
    if (stopped) {
        Cycles::Hold hold;
        _regs->save();
    }
    disassembleCycles();
}

void PinsI8086::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, InstI8086::INT3);
}

// Type 2 raises NMI, an edge the CPU latches and the trap negates; any
// other type raises INTR, high while a device's vector() is pending.
void PinsI8086::assertInt(uint8_t name) {
    if (name == InstI8086::VEC_NMI) {
        assert_nmi();
    } else {
        digitalWriteFast(PIN_INTR, HIGH);
    }
}

void PinsI8086::negateInt(uint8_t name) {
    if (name != InstI8086::VEC_NMI)
        digitalWriteFast(PIN_INTR, _devs->vector() ? HIGH : LOW);
}

void PinsI8086::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

// No status tells a fetch from a prefetch, so the backtrace prints every
// cycle.
const SignalsImpl *PinsI8086::findBacktraceStart() {
    return Cycles::tail();
}

void PinsI8086::printBacktrace() {
    printCycles();
}

}  // namespace i8086
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
