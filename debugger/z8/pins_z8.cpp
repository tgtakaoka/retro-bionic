#include "pins_z8.h"
#include <type_traits>
#include "debugger.h"
#include "devs_z8.h"
#include "digital_bus.h"
#include "inst_z8.h"
#include "mems_z8.h"
#include "pins_z88.h"
#include "regs_z8.h"
#include "z86_sio_handler.h"
#include "z88_uart_handler.h"

namespace debugger {
namespace z8 {

namespace {
void inline assert_irq0() {
    digitalWriteFast(PIN_IRQ0, LOW);
}

void inline negate_irq0() {
    digitalWriteFast(PIN_IRQ0, HIGH);
}

void inline assert_irq1() {
    digitalWriteFast(PIN_IRQ1, LOW);
}

void inline negate_irq1() {
    digitalWriteFast(PIN_IRQ1, HIGH);
}

void inline assert_irq2() {
    digitalWriteFast(PIN_IRQ2, LOW);
}

void inline negate_irq2() {
    digitalWriteFast(PIN_IRQ2, HIGH);
}

void inline negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
}

inline auto signal_as() {
    return digitalReadFast(PIN_AS);
}

inline auto signal_ds() {
    return digitalReadFast(PIN_DS);
}

constexpr uint8_t PINS_LOW[] = {
        PIN_XTAL1,
        PIN_RESET,
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_IRQ0,
        PIN_IRQ1,
        PIN_IRQ2,
};

constexpr uint8_t PINS_PULLDOWN[] = {
        PIN_ADDR8,
        PIN_ADDR9,
        PIN_ADDR10,
        PIN_ADDR11,
        PIN_ADDR12,
        PIN_ADDR13,
        PIN_ADDR14,
        PIN_ADDR15,
};

constexpr uint8_t PINS_INPUT[] = {
        PIN_DATA0,
        PIN_DATA1,
        PIN_DATA2,
        PIN_DATA3,
        PIN_DATA4,
        PIN_DATA5,
        PIN_DATA6,
        PIN_DATA7,
        PIN_AS,
        PIN_DS,
        PIN_RW,
        PIN_DM,
};

}  // namespace

// The Z86 bus cycle starts on XTAL1 falling.
template <>
struct PinsZ8::Traits<VAR_Z86> {
    static constexpr bool fetchVectorAfterContextSave = true;
    using Serial = z86::Z86SioHandler;
    // XTAL1~10MHz
    static constexpr uint8_t xtal1_first = LOW;
    static constexpr uint8_t xtal1_second = HIGH;
};

// The Super8 bus cycle starts on XTAL1 rising.
template <>
struct PinsZ8::Traits<VAR_Z88> {
    static constexpr bool fetchVectorAfterContextSave = false;
    using Serial = z88::Z88UartHandler;
    // XTAL1~10MHz
    static constexpr uint8_t xtal1_first = HIGH;
    static constexpr uint8_t xtal1_second = LOW;
};

// Delays by Variant, in ns: each XTAL1 phase, 50 ns at 10 MHz.
constexpr uint16_t DELAYS[][DELAY_COUNT] = {
        {20, 8},  // VAR_Z86
        {20, 8},  // VAR_Z88
};

template <Variant E, Delay F>
inline void PinsZ8::delay() const {
    if constexpr (DELAYS[E][F] != 0)
        delayNanoseconds(DELAYS[E][F]);
}

// Calls |f| with the engine as a compile-time constant.
template <typename F>
auto PinsZ8::withEngine(F f) {
    switch (_engine) {
    case VAR_Z88:
        return f(std::integral_constant<Variant, VAR_Z88>());
    default:
        return f(std::integral_constant<Variant, VAR_Z86>());
    }
}

template <Variant E>
inline void PinsZ8::xtal1_cycle() const {
    digitalWriteFast(PIN_XTAL1, Traits<E>::xtal1_first);
    delay<E, xtal1_first_ns>();
#if defined(ENABLE_SERIAL_HANDLER)
    static_cast<typename Traits<E>::Serial *>(_serial)->loop();
#endif
    digitalWriteFast(PIN_XTAL1, Traits<E>::xtal1_second);
    delay<E, xtal1_second_ns>();
}

void PinsZ8::resetPins() {
    // Assert reset condition
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_PULLDOWN, sizeof(PINS_PULLDOWN), INPUT_PULLDOWN);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    withEngine([&](auto e) { resetCpu<decltype(e)::value>(); });
}

template <Variant E>
void PinsZ8::resetCpu() {
    // #RESET must remain low for a minimum of 16 clocks; the loop gives
    // 16 T states, two XTAL1 cycles each.
    for (auto i = 0; i < 16 * 2; i++) {
        xtal1_cycle<E>();
        delayNanoseconds(1);
    }
    negate_reset();
    Cycles::reset();
    // Wait for #AS pulse with #DS=H
    while (signal_ds() == LOW) {
        while (signal_as() == LOW)
            xtal1_cycle<E>();
        while (signal_as() != LOW)
            xtal1_cycle<E>();
    }

    _regs->reset();
    _regs->save();
}

template <Variant E>
Signals *PinsZ8::prepareCycle() {
    auto s = Signals::put();
    s->fetch() = false;
    // Wait for bus cycle
    while (signal_ds() != LOW) {
        if (signal_as() == LOW) {
            // Read address and bus status
            s->getAddr();
        }
        xtal1_cycle<E>();
    }
    // #DS is low
    return s;
}

template <Variant E>
Signals *PinsZ8::completeCycle(Signals *s) {
    // #DS is low
    const auto mems = this->mems<MemsZ8>();
    if (s->write()) {
        ++_writes;
        s->getData();
        if (s->writeMemory()) {
            mems->write(s->addr, s->data);
        } else {
            ;  // capture data to s->data
        }
    } else {
        _writes = 0;
        if (s->readMemory()) {
            s->data = mems->read(s->addr);
        } else {
            ;  // inject data from s->data
        }
        xtal1_cycle<E>();
        s->outData();
    }
    while (signal_ds() == LOW) {
        xtal1_cycle<E>();
    }
    // #DS is high
    Cycles::next();
    busMode(DATA, INPUT);
    return s;
}

template <Variant E>
Signals *PinsZ8::cycle() {
    return completeCycle<E>(prepareCycle<E>());
}

template <Variant E>
Signals *PinsZ8::cycle(uint8_t inst) {
    return completeCycle<E>(prepareCycle<E>()->inject(inst));
}

#if defined(Z88_FAST_INTERRUPT)
// A fast interrupt discards an opcode fetch and reads the byte after it,
// then fetches at IP.
bool PinsZ8::fastEntry(const Signals *s) const {
    if (s->addr != _fastFetch || !s->read())
        return false;
    const auto next = s->prev();
    const auto discarded = s->prev(2);
    return next->read() && discarded->read() &&
           next->addr == uint16_t(discarded->addr + 1);
}
#endif

// The Super8 vectors end here; no code runs below it.
constexpr uint16_t SUPER8_VECTOR_END = 0x0020;

uint16_t PinsZ8::nextFetch() {
    return withEngine([&](auto e) { return nextFetch<decltype(e)::value>(); });
}

template <Variant E>
uint16_t PinsZ8::nextFetch() {
    constexpr auto fetchVectorAfterContextSave =
            Traits<E>::fetchVectorAfterContextSave;
    // An interrupt already taken here shows as a discarded fetch and read,
    // then the Super8 fetches its vector, before the pushes.
    uint16_t pc = 0;
    Signals *s = nullptr;
    for (auto n = 0;; ++n) {
        // Three reads with no push or vector fetch: no acknowledge is
        // under way, and the first read was the next fetch.
        if (n == 3)
            return pc;
        s = prepareCycle<E>();
        if (s->write() ||
                (!fetchVectorAfterContextSave && s->addr < SUPER8_VECTOR_END))
            break;
        if (n == 0)
            pc = s->addr;
#if defined(Z88_FAST_INTERRUPT)
        if (E == VAR_Z88 && n == 2 && fastEntry(s)) {
            // The save starts at the handler; IP and FIS hold the return.
            pc = s->addr;
            completeCycle<E>(s->inject(InstZ8::NOP));
            return pc;
        }
#endif
        completeCycle<E>(s->inject(InstZ8::NOP));
    }
    // Let the acknowledge fetch its vector and finish its pushes.
    while (!s->write()) {
        completeCycle<E>(s);
        s = prepareCycle<E>();
    }
    while (s->write()) {
        completeCycle<E>(s);
        s = prepareCycle<E>();
    }
    if (fetchVectorAfterContextSave) {
        completeCycle<E>(s);
        s = cycle<E>();
        intrAck(s->prev(6));
        s = prepareCycle<E>();
    } else {
        intrAck(s->prev(7));
    }
    pc = s->addr;
    completeCycle<E>(s->inject(InstZ8::NOP));
    return pc;
}

template <Variant E>
bool PinsZ8::rawStep() {
    constexpr auto fetchVectorAfterContextSave =
            Traits<E>::fetchVectorAfterContextSave;
    auto s = prepareCycle<E>();
    // The Super8 reads its vector before the pushes, so an acknowledge
    // begun in the last instruction's cycles may still be reading it.
    while (!fetchVectorAfterContextSave && s->read() &&
            s->addr < SUPER8_VECTOR_END) {
        completeCycle<E>(s);
        s = prepareCycle<E>();
    }
    if (s->write()) {
        // An interrupt acknowledge is under way; let it push PC and FLAGS.
        while (s->write()) {
            completeCycle<E>(s);
            s = prepareCycle<E>();
        }
        if (fetchVectorAfterContextSave) {
            completeCycle<E>(s);
            s = cycle<E>();
            intrAck(s->prev(6));
            s = prepareCycle<E>();
        } else {
            intrAck(s->prev(7));
        }
    }
#if defined(Z88_FAST_INTERRUPT)
    // Only the Super8 has a fast interrupt.
    if constexpr (E == VAR_Z88) {
        if (fastEntry(s))
            static_cast<z88::PinsZ88 *>(this)->takeFast(s);
    }
#endif
    const auto inst = _mems->read_byte(s->addr);
    auto cycles = _inst.busCycles(inst);
    if (_inst.isBreak(inst) || cycles == 0) {
        // Stop on a JR to itself instead of the break.
        completeCycle<E>(s->inject(InstZ8::JR));
        cycle<E>(InstZ8::JR_HERE);
        Cycles::discard(s);
        return false;
    }
#if defined(Z88_FAST_INTERRUPT)
    if constexpr (E == VAR_Z88)
        cycles = static_cast<z88::PinsZ88 *>(this)->fastCycles(
                s->addr, inst, cycles);
#endif
    auto fetch = s;
    completeCycle<E>(s);
    for (auto c = 1; c < cycles; ++c) {
#if defined(Z88_FAST_INTERRUPT)
        s = prepareCycle<E>();
        // Left pending, the handler's fetch is the next rawStep()'s, since
        // prepareCycle() returns a cycle under way again.
        if (E == VAR_Z88 && fastEntry(s))
            return true;
        completeCycle<E>(s);
#else
        s = cycle<E>();
#endif
        // Three writes in a row are an acknowledge's PC and FLAGS pushes.
        if (_writes == 3) {
            if (fetchVectorAfterContextSave) {
                cycle<E>();
                s = cycle<E>();
            }
            intrAck(s->prev(6));
            return true;
        }
    }
    fetch->fetch() = true;
    return true;
}

void PinsZ8::execInst(const uint8_t *inst, uint8_t len) {
    execute(inst, len, nullptr, nullptr, 0);
}

void PinsZ8::captureWrites(const uint8_t *inst, uint8_t len, uint16_t *addr,
        uint8_t *buf, uint8_t max) {
    execute(inst, len, addr, buf, max);
}

void PinsZ8::execute(const uint8_t *inst, uint8_t len, uint16_t *addr,
        uint8_t *buf, uint8_t max) {
    withEngine([&](auto e) {
        execute<decltype(e)::value>(inst, len, addr, buf, max);
    });
}

template <Variant E>
void PinsZ8::execute(const uint8_t *inst, uint8_t len, uint16_t *addr,
        uint8_t *buf, uint8_t max) {
    uint8_t inj = 0;
    uint8_t cap = 0;
    while (inj < len || cap < max) {
        Signals *s = prepareCycle<E>();
        if (cap < max)
            s->capture();
        if (inj < len)
            s->inject(inst[inj]);
        completeCycle<E>(s);
        if (s->read()) {
            if (inj < len) {
                inj++;
            }
        } else {
            if (cap == 0 && addr)
                *addr = s->addr;
            if (buf && cap < max) {
                buf[cap++] = s->data;
            }
        }
    }
}

// A JR to itself keeps the CPU spinning in place.
void PinsZ8::idle() {
    withEngine([&](auto e) { idle<decltype(e)::value>(); });
}

template <Variant E>
void PinsZ8::idle() {
    const auto s = cycle<E>(InstZ8::JR);
    cycle<E>(InstZ8::JR_HERE);
    Cycles::discard(s);
}

#ifdef PROFILE_CYCLES
// prepareCycle(), giving up when #DS stays high for |clocks| XTAL1 cycles.
template <Variant E>
Signals *PinsZ8::awaitCycle(uint16_t clocks) {
    auto s = Signals::put();
    s->fetch() = false;
    for (uint16_t n = 0; signal_ds() != LOW; ++n) {
        if (n == clocks)
            return nullptr;
        if (signal_as() == LOW)
            s->getAddr();
        xtal1_cycle<E>();
    }
    return s;
}

// Marks the opcode fetches as rawStep() counts them, each with the bus
// cycles its table gives it, up to the trap.
void PinsZ8::markFetches() const {
    const auto begin = Signals::get();
    const auto cycles = begin->diff(Signals::put());
    for (auto i = 0u; i < cycles;) {
        const auto s = begin->next(i);
        const auto n = _inst.busCycles(s->data);
        if (s->write() || n == 0)
            break;
        s->fetch() = n;
        if (_inst.isBreak(s->data))
            break;
        i += n;
    }
}

// For scripts/record-cycles.py: every bus cycle as the chip makes it, up to
// the trap stopping the CPU, giving up well before the ring wraps; the
// debug pin frames the run, to trigger a capture on.
// HALT and WFI read the byte after their opcode, then stop the bus.
template <Variant E>
void PinsZ8::loop() {
    constexpr auto MAX_CYCLES = 96;
    // No instruction keeps #DS high nearly this long.
    constexpr uint16_t QUIET_CLOCKS = 256;
    // Nothing but a trap fetching in place reads one address this many
    // times in a row.
    constexpr auto SPIN_READS = 4;
    assert_debug();
    auto reads = 0;
    for (auto n = 0;; ++n) {
        _devs->loop();
        const auto s = awaitCycle<E>(QUIET_CLOCKS);
        if (s == nullptr)
            break;
        completeCycle<E>(s);
        const auto p = s->prev();
        const auto again =
                n > 0 && s->read() && p->read() && s->addr == p->addr;
        reads = again ? reads + 1 : 1;
        if (reads >= SPIN_READS)
            break;
        if (n >= MAX_CYCLES || haltSwitch()) {
            cli.println(n >= MAX_CYCLES ? "?cycles" : "?halt");
            break;
        }
    }
    negate_debug();
    markFetches();
}
#else
template <Variant E>
void PinsZ8::loop() {
    const auto devs = this->devs<DevsZ8>();
    while (true) {
        const auto serviced = devs->loop();
        if (!rawStep<E>() || (serviced && haltSwitch()))
            return;
    }
}
#endif

void PinsZ8::run() {
    _regs->restore();
    Cycles::reset();
    saveBreakInsts();
    startRunTimer();
    withEngine([&](auto e) { loop<decltype(e)::value>(); });
    stopRunTimer();
    restoreBreakInsts();
    disassembleCycles();
#ifdef PROFILE_CYCLES
    // The trap leaves the CPU halted, and only a reset brings it back.
    resetPins();
#else
    _regs->save();
#endif
}

// An acknowledge is 7 bus cycles: the discarded fetch and read, three
// pushes and two vector reads (before the pushes on the Super8).
void PinsZ8::intrAck(Signals *frame) const {
    for (auto i = 0; i < 7; ++i)
        frame->next(i)->fetch() = false;
}

bool PinsZ8::step(bool show) {
    Cycles::reset();
    _regs->restore();
    if (show)
        Cycles::reset();
    if (withEngine([&](auto e) { return rawStep<decltype(e)::value>(); })) {
        if (show)
            printCycles();
        _regs->save();
        return true;
    }
    return false;
}

void PinsZ8::assertInt(uint8_t name) {
    switch (name) {
    default:
        break;
    case INTR_IRQ0:
        assert_irq0();
        break;
    case INTR_IRQ1:
        assert_irq1();
        break;
    case INTR_IRQ2:
        assert_irq2();
        break;
    }
}

void PinsZ8::negateInt(uint8_t name) {
    switch (name) {
    default:
        break;
    case INTR_IRQ0:
        negate_irq0();
        break;
    case INTR_IRQ1:
        negate_irq1();
        break;
    case INTR_IRQ2:
        negate_irq2();
        break;
    }
}

void PinsZ8::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, _inst.breakInst());
}

void PinsZ8::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; i++) {
        g->next(i)->print();
    }
}

const SignalsImpl *PinsZ8::findBacktraceStart() {
    return backtraceStartByFetchCount<Signals>(_lineLimit);
}

void PinsZ8::printBacktrace() {
#ifdef PROFILE_CYCLES
    // Every cycle, with the marks the matcher gave them.
    printCycles();
#else
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles;) {
        const auto s = g->next(i);
        if (s->fetch()) {
            const auto len = _mems->disassemble(s->addr, 1) - s->addr;
            i += len;
        } else {
            s->print();
            ++i;
        }
    }
#endif
}

}  // namespace z8
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
