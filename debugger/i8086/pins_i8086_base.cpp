#include "pins_i8086_base.h"
#include "debugger.h"
#include "inst_i8086.h"
#include "mems_i8086.h"
#include "qs_decode_i8086.h"
#include "regs_i8086.h"

namespace debugger {
namespace i8086 {

namespace {

// Bus cycles allowed for an injected sequence and for a step.
constexpr auto exec_cycles = 512;
constexpr auto step_cycles = 1024;

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
    return s->code() && InstI8086::vector(s->addr);
}

// A fetch the queue makes from PARK on.
bool atPark(const Signals *s) {
    return s->code() && ((s->addr - PARK) & 0xFFFFF) < 16;
}

}  // namespace

// Takes up a cycle waiting at |addr|, the fetch every park is.
Signals *PinsI8086Base::resumeCycle(uint32_t addr) {
    auto s = Signals::put();
    s->addr = addr;
    s->setFetch(_bus8);
    s->setStatus(Signals::STATUS_CODE);
    return s;
}


void PinsI8086Base::Overlay::put(uint32_t addr, uint8_t data) {
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

int PinsI8086Base::Overlay::find(uint32_t addr) const {
    for (uint_fast8_t i = 0; i < _num; ++i) {
        if (_addr[i] == addr)
            return i;
    }
    return -1;
}

bool PinsI8086Base::Overlay::get(uint32_t addr, uint8_t &data) const {
    const auto i = find(addr);
    if (i < 0)
        return false;
    data = _data[i];
    return true;
}

uint16_t PinsI8086Base::Overlay::wordAt(uint32_t addr) const {
    uint8_t l = 0, h = 0;
    get(addr, l);
    get((addr + 1) & 0xFFFFF, h);
    return uint16(h, l);
}

void PinsI8086Base::Overlay::markRead(uint32_t addr) {
    const auto i = find(addr);
    if (i >= 0)
        _read[i] = true;
}

bool PinsI8086Base::Overlay::allRead() const {
    for (uint_fast8_t i = 0; i < _num; ++i) {
        if (!_read[i])
            return false;
    }
    return true;
}


// A read's data from memory, a device, or an acknowledge's vector.
void PinsI8086Base::retrieveData(Signals *s) {
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
void PinsI8086Base::storeData(Signals *s) {
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

void PinsI8086Base::injectRead(
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

void PinsI8086Base::execInst(
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
void PinsI8086Base::recordFrame(const Signals *s) {
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
bool PinsI8086Base::parkInTrap(Signals *s) {
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
        if (isVector(t) || (atPark(t) && t != s) ||
                (t->write() && t->memory() && ((t->addr - base) & 0xFFFFF) < 6))
            from = t;
    }
    Cycles::discard(from);
    auto regs = this->regs<RegsI8086>();
    const uint16_t parkIp = InstI8086::PARK_IP + ((s->addr - PARK) & 0xFFFFF);
    regs->parkInTrap(cs, ip, flags, _vector, parkIp);
    return true;
}

// Completes cycles from |s| until the CPU takes one of the debugger's
// vectors and fetches at PARK. A halted CPU, or the halt switch while
// running, gets NMI.
bool PinsI8086Base::untilTrap(Signals *s, Mode mode) {
    _vector = 0;
    _frameBytes = 0;
    auto nmi = false;
    uint32_t guard = mode == STEP ? step_cycles : UINT32_MAX;
    for (uint32_t n = 0; guard; ++n, --guard) {
        if (s == nullptr) {
            if (nmi)
                break;
            assertNmi();
            nmi = true;
            s = prepareCycle();
            continue;
        }
        if (isVector(s)) {
            // The frame is what the trap pushes from here on.
            if (_vector == 0)
                _frameBytes = 0;
            _vector = s->addr >> 2;
            const auto b = parkVector(s->addr);
            if (s->wordAccess()) {
                s->inject(uint16(parkVector(s->addr + 1), b));
            } else {
                s->inject(_bus8 ? b : uint16(b, b));
            }
            if (_vector == InstI8086::VEC_NMI)
                negateNmi();
        } else if (_vector && atPark(s)) {
            if (_frameBytes == 6) {
                negateNmi();
                return parkInTrap(s);
            }
            // The CPU fetches at PARK before it pushes IP; the queue gets
            // NOPs until the frame is whole, and it parks at the next fetch.
            s->inject(uint16(InstI8086::NOP, InstI8086::NOP));
        } else if (s->halt() && !nmi) {
            assertNmi();
            nmi = true;
        }
        completeCycle(s);
        if (s->write() && s->memory() && !s->coproc())
            recordFrame(s);
        if (mode == RUN) {
            if (_devs->loop() && !nmi && haltSwitch()) {
                assertNmi();
                nmi = true;
            }
        }
        s = prepareCycle();
    }
    negateNmi();
    return false;
}


// DIAG: the ring raw, for a host replay of the decoder: each cycle's time
// before the latest sample, in CPU cycles, and the cycle; then the queue
// status as far back as the oldest cycle, oldest first.
void PinsI8086Base::dumpQueue() {
    const auto g = Signals::get();
    const int n = g->diff(Signals::put());
    cli.print("QSDUMP period=");
    cli.printDec(clockCycles());
    cli.print(" delay=");
    cli.printDec(_qs.delay());
    cli.print(" cycles=");
    cli.printDec(n);
    const auto mems = this->mems<MemsI8086>();
    if (mems->hasEmulationArea()) {
        cli.print(" area=");
        cli.printHex(mems->protectArea()->begin(), 5);
        cli.print('-');
        cli.printHex(mems->protectArea()->end(), 5);
    }
    cli.print(" emu=");
    cli.printDec(this->regs<RegsI8086>()->emulation());
    cli.println();
    for (int i = 0; i < n; ++i) {
        const auto s = g->next(i);
        cli.printDec(_qs.stopCycle() - _seenAt[s->pos()]);
        cli.print(' ');
        s->print();
    }
    const int first =
            n ? (_qs.stopCycle() - _seenAt[g->pos()]) / clockCycles() : 0;
    cli.print("QS");
    for (int b = first < int(QsTracker::SAMPLES) ? first : QsTracker::SAMPLES - 1,
             k = 0;
            b >= 0; --b, ++k) {
        if (k % 64 == 0)
            cli.println();
        cli.print(char('0' + _qs.sampleAt(b)));
    }
    cli.println();
    cli.println("QSEND");
}


bool PinsI8086Base::step(bool show) {
    auto regs = this->regs<RegsI8086>();
    const auto emu = regs->emulation();
    // A HLT waits for an interrupt; stepping into it would hang.
    if (_mems->read(regs->nextIp()) ==
            (emu ? InstI8086::HLT_8080 : InstI8086::HLT))
        return false;
    Cycles::reset();
    _qs.thaw();
    {
        Cycles::Hold hold;
        regs->restore(true);
    }
    const auto trapped = untilTrap(_next, STEP);
    freezeQs();
    if (trapped) {
        if (show)
            printCycles();
        Cycles::Hold hold;
        regs->save();
        return true;
    }
    return false;
}

void PinsI8086Base::run() {
    Cycles::reset();
    {
        Cycles::Hold hold;
        _regs->restore();
    }
    saveBreakInsts();
    _qs.thaw();
    startRunTimer();
    const auto stopped = untilTrap(_next, RUN);
    stopRunTimer();
    freezeQs();
    restoreBreakInsts();
    if (stopped) {
        Cycles::Hold hold;
        _regs->save();
    }
    disassembleCycles();
}

// In 8080 code INT 3 (CC) is CZ: a HLT there stops the CPU, and the halt's
// NMI leaves emulation for the debugger.
void PinsI8086Base::setBreakInst(uint32_t addr) const {
    const auto emu = mems<MemsI8086>()->emulationAt(addr);
    _mems->put_prog(addr, emu ? InstI8086::HLT_8080 : InstI8086::INT3);
}


void PinsI8086Base::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

// No status tells a fetch from a prefetch, so the backtrace prints every
// cycle.
const SignalsImpl *PinsI8086Base::findBacktraceStart() {
    return Cycles::tail();
}

void PinsI8086Base::printBacktrace() {
    if (_qsMode != QS_RAW && _qs.frozen() && printExecuted(false))
        return;
    printCycles();
}

namespace {

// The ring and the frozen queue status, as QsDecoder reads them.
struct QsSource {
    const Signals *g;
    int n;
    const uint32_t *seenAt;
    const QsTracker &tracker;
    uint32_t period;
    int first;  // the samples back from the stop that the oldest cycle has
    const MemsI8086 *mems;

    int cycles() const { return n; }
    uint32_t cycleTime(int i) const { return seenAt[g->next(i)->pos()]; }
    bool fetch(int i, uint32_t &first, uint32_t &next) const {
        const auto s = g->next(i);
        if (!s->code() || InstI8086::vector(s->addr))
            return false;
        first = s->covers(s->addr) ? s->addr : (s->addr + 1) & 0xFFFFF;
        next = (s->addr + (s->wordAccess() ? 2 : 1)) & 0xFFFFF;
        return true;
    }
    int samples() const { return first + 1; }
    uint32_t sampleTime(int k) const {
        return tracker.stopCycle() - uint32_t(first - k) * period;
    }
    uint8_t qs(int k) const { return tracker.sampleAt(first - k); }
    uint32_t insn(uint32_t addr, bool emu) const {
        return mems->disassembleAs(addr, emu);
    }
    uint8_t length(uint32_t addr, bool emu) const {
        return mems->insnLength(addr, emu);
    }
    bool vector(int i) const {
        const auto s = g->next(i);
        return s->memory() && s->read() && InstI8086::vector(s->addr);
    }
    // The MD flag (bit 15) a data read brings: its byte at an odd address.
    int popMode(int i) const {
        const auto s = g->next(i);
        if (s->code())
            return -2;
        if (!s->memory() || !s->read())
            return s->memory() ? -1 : -2;
        const auto odd = s->addr | 1;
        return s->covers(odd) ? (s->byteAt(odd) >> 7) : -1;
    }
    uint8_t code(uint32_t addr) const { return mems->read_byte(addr); }
    int areaMode(uint32_t addr) const {
        if (!mems->hasEmulationArea())
            return -1;
        return mems->emulationAt(addr);
    }
    void cycle(int i) const { g->next(i)->print(); }
};

}  // namespace

// The instructions the CPU executed, from the frozen queue status. False
// when the ring holds no instruction to show.
bool PinsI8086Base::printExecuted(bool stats) {
    const auto g = Signals::get();
    const int n = g->diff(Signals::put());
    if (n == 0)
        return false;
    const int first = (_qs.stopCycle() - _seenAt[g->pos()]) / clockCycles();
    if (first >= int(QsTracker::SAMPLES))
        return false;
    QsSource src{g, n, _seenAt, _qs, clockCycles(), first,
            static_cast<const MemsI8086 *>(_mems)};
    const auto pairs = _qsMode == QS_PAIR;
    const auto regs = this->regs<RegsI8086>();
    QsDecoder<QsSource, Cycles::MAX_CYCLES * 2> dec(
            src, pairs, regs->nextIp(), regs->emulation());
    if (!dec.decode(_lineLimit))
        return false;
    if (pairs && stats) {
        cli.print("QS starts good=");
        cli.printDec(dec.good);
        cli.print(" bad=");
        cli.printDec(dec.bad);
        cli.print(" delay=");
        cli.printDec(_qs.delay());
        cli.println();
    }
    return true;
}

}  // namespace i8086
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
