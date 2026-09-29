#include "pins_mc68hc08az0.h"
#include "debugger.h"
#include "devs_mc6805.h"
#include "inst_mc68hc08.h"
#include "mems_mc6805.h"
#include "regs_mc68hc08.h"
#include "signals_mc68hc08az0.h"

#define DEBUG(e) e
// #define DEBUG(e)

namespace debugger {
namespace mc68hc08az0 {

using mc68hc08::InstMc68HC08;

/**
 * MC68HC08AZ0 bus cycle.
 *       |--c1-|--c2-|--c3-|--c4-|--c1-|--c2-|--c3-|--c4-|--c1-|
 *      _|   __|   __|   __|   __|   __|   __|   __|   __|   __|
 * OSC1  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|
 *          \ ____\  \           \  \ ____\  \           \  \ __
 * MCLK _____|     |_________________|     |_________________|
 *      ______________             _____________________________
 *  REB               |___________|
 *      ______________________________________             _____
 *  WEB                                       |___________|
 *      ______ _______________________ _______________________ _
 *  EAB ______X_______________________X_______________________X_
 *              ______        ____      ______   ___________   _
 *  EDB -------<__
 */

namespace {

//   fOSC: min  DC, max 16MHz

constexpr auto osc1_hi_ns = 10;  // 32
constexpr auto osc1_lo_ns = 20;  // 32

inline void osc1_hi() {
    digitalWriteFast(PIN_OSC1, HIGH);
}

inline void osc1_lo() {
    digitalWriteFast(PIN_OSC1, LOW);
}

void osc1_cycle_lo() {
    osc1_hi();
    delayNanoseconds(osc1_hi_ns);
    osc1_lo();
}

void osc1_cycle() {
    osc1_cycle_lo();
    delayNanoseconds(osc1_lo_ns);
}

void negate_reset() {
    pinMode(PIN_RST, INPUT_PULLUP);
    digitalWriteFast(PIN_RST, HIGH);
}

auto reset_asserted() {
    return digitalReadFast(PIN_RST) == LOW;
}

constexpr uint8_t PINS_OPENDRAIN[] = {
        PIN_RST,  // bi-directional
};

constexpr uint8_t PINS_LOW[] = {
        PIN_OSC1,
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_IRQ,
};

constexpr uint8_t PINS_INPUT[] = {
        PIN_ED0,
        PIN_ED1,
        PIN_ED2,
        PIN_ED3,
        PIN_ED4,
        PIN_ED5,
        PIN_ED6,
        PIN_ED7,
        PIN_EA0,
        PIN_EA1,
        PIN_EA2,
        PIN_EA3,
        PIN_EA4,
        PIN_EA5,
        PIN_EA6,
        PIN_EA7,
        PIN_EA8,
        PIN_EA9,
        PIN_EA10,
        PIN_EA11,
        PIN_EA12,
        PIN_EA13,
        PIN_EA14,
        PIN_EA15,
        PIN_REB,
        PIN_WEB,
        PIN_MCLK,
};

}  // namespace

PinsMc68HC08AZ0::PinsMc68HC08AZ0() {
    auto regs = new mc68hc08::RegsMc68HC08(this);
    _regs = regs;
    _devs = new mc6805::DevsMc6805(ACIA_BASE);
    _mems = new mc6805::MemsMc6805(this, regs, _devs, 16);
    _inst = new InstMc68HC08(_mems);
}

void PinsMc68HC08AZ0::resetCpu() {
    // Assert reset condition
    pinsMode(PINS_OPENDRAIN, sizeof(PINS_OPENDRAIN), OUTPUT_OPENDRAIN, LOW);
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    // #RESET input for a period of one and one-half machine cycles.
    for (uint_fast8_t i = 0; i < 4 * 10; ++i)
        osc1_cycle();
    negate_reset();
    // Wait for finishing power on reset.
    while (reset_asserted())
        osc1_cycle();
    Cycles::reset();
    prepareCycle();
    // Inject dummy reset vector and wait for the first instruction fetch.
    _regs->reset();
}

void PinsMc68HC08AZ0::idle() {
    // MC68HC08AZ0 is fully static, so we can stop clock safely.
}

Signals *PinsMc68HC08AZ0::currCycle(uint16_t pc) const {
    auto s = SignalsMc68HC08AZ0::put();
    s->getControl();
    s->addr = pc ? pc : _addr;
    return s;
}

Signals *PinsMc68HC08AZ0::rawPrepareCycle() {
    auto s = SignalsMc68HC08AZ0::put();
    s->clearFetch();
    while (true) {
        if (s->getControl()) {
            s->getAddr();
            _addr = s->addr;
            return s;
        }
        osc1_cycle_lo();
    }
}

// A bus cycle within `limit` OSC1 cycles, or nullptr.
Signals *PinsMc68HC08AZ0::awaitCycle(uint_fast16_t limit) {
    auto s = SignalsMc68HC08AZ0::put();
    s->clearFetch();
    for (uint_fast16_t n = 0; n < limit; ++n) {
        if (s->getControl()) {
            s->getAddr();
            _addr = s->addr;
            return s;
        }
        osc1_cycle_lo();
    }
    return nullptr;
}

// WAIT and STOP make no bus cycle until an interrupt or a reset: keep the
// devices running so one can come. nullptr on a halt.
Signals *PinsMc68HC08AZ0::sleep() {
    while (true) {
        _devs->loop();
        if (haltSwitch())
            return nullptr;
        auto s = awaitCycle(256);
        if (s)
            return s;
    }
}

// The next bus cycle, or nullptr on a halt while the CPU sleeps in WAIT or
// STOP. Eight bus cycles of silence tell that, or a reset, which is silent
// for 64 OSC1 cycles before reading its vector.
Signals *PinsMc68HC08AZ0::nextCycle() {
    auto s = awaitCycle(32);
    if (s)
        return s;
    _slept = true;
    return sleep();
}

// WAIT and STOP both clear the I mask: take an IRQ, whose push stands in
// for save()'s SWI. STOP recovery takes 4096 OSC1 cycles.
void PinsMc68HC08AZ0::wakeUp() {
    constexpr auto WAKE_CYCLES = 4096 + 256;
    assertInt(0);
    for (auto n = 0; _writes < 5; ++n) {
        auto s = awaitCycle(WAKE_CYCLES);
        if (s == nullptr || n >= 16) {
            negateInt(0);
            cli.println("?halt: CPU didn't wake");
            return;
        }
        completeCycle(s);
    }
    auto s = prepareCycle();  // hi(vector)
    negateInt(0);
    auto r = regs<mc6805::RegsMc6805>();
    r->captureContext(s->prev(5));
    r->captureExtra(r->nextIp());
}

Signals *PinsMc68HC08AZ0::completeCycle(Signals *signals) {
    auto s = static_cast<SignalsMc68HC08AZ0 *>(signals);
    if (s->write()) {
        s->getData();
        if (s->writeMemory()) {
            _mems->write(s->addr, s->data);
        }
        _writes++;
    } else if (is_internal(s->addr)) {
        s->getData();
        _writes = 0;
    } else {
        if (s->readMemory()) {
            s->data = _mems->read(s->addr);
        }
        s->outData();
        _writes = 0;
    }
    Cycles::next();
    s = SignalsMc68HC08AZ0::put();
    do {
        osc1_cycle_lo();
        SignalsMc68HC08AZ0::inputMode();
    } while (s->getControl());
    return s;
}

#ifdef PROFILE_CYCLES
// Every bus cycle as the chip makes it, no sequence table, up to an SWI,
// for scripts/record-cycles.py. The debug pin frames the run, to trigger a
// capture on.
void PinsMc68HC08AZ0::loop() {
    constexpr auto MAX_CYCLES = 64;
    const auto vec_swi = mems<mc6805::MemsMc6805>()->vecSwi();
    assert_debug();
    auto s = prepareCycle();
    for (auto n = 0;; ++n) {
        auto r = regs<mc6805::RegsMc6805>();
        if (s->addr == vec_swi && r->captureContext(s->prev(5))) {
            negate_debug();
            disassembleCycles();
            r->captureExtra(r->nextIp() - 1);  // offset SWI
            return;
        }
        completeCycle(s);
        s = awaitCycle(10000);
        if (s == nullptr || n >= MAX_CYCLES || haltSwitch()) {
            negate_debug();
            disassembleCycles();
            cli.println(s == nullptr ? "?no bus cycle"
                                     : n >= MAX_CYCLES ? "?cycles" : "?halt");
            resetCpu();
            return;
        }
    }
}
#else
// This only sees the halt switch and breakpoints where its cycle sequences
// expect an instruction boundary. The COP is a mask option here (MORA $1F,
// COPD clear): a program that doesn't service it is reset.
void PinsMc68HC08AZ0::loop() {
    constexpr uint8_t WAIT = 0x8F;
    constexpr uint16_t VEC_RESET = 0xFFFE;
    const auto vec_swi = mems<mc6805::MemsMc6805>()->vecSwi();
    auto s = prepareCycle();
    // run() has completed the first opcode fetch.
    auto fetch = s->prev();
    // Only WAIT and STOP leave the bus silent: any other gap is no sleep.
    auto asleep = false;
    while (true) {
        _devs->loop();
        if (s->addr == VEC_RESET && !s->write()) {
            // A reset turns off what reset() set up, internal read
            // visibility among it: set it up again, then go to the vector.
            const auto vec = uint16(
                    _mems->read(VEC_RESET), _mems->read(VEC_RESET + 1));
            _regs->reset();
            const uint8_t JMP[] = {
                    0xCC, hi(vec), lo(vec),  // JMP vec; 0:2:3:N
            };
            injectReads(JMP, sizeof(JMP), 0);
            Cycles::reset();
            s = fetch = currCycle(0);
        }
        while (s->write() && _writes < 5) {
            // acknowledge interrupt
            completeCycle(s);
            if (_writes == 5) {
                cycle();  // read hi(vector)
                cycle();  // read lo(vector)
            }
            s = fetch = prepareCycle();
        }
        auto *seq = inst<InstMc68HC08>()->sequence(fetch);
        if (*seq == 0) {
            // No such opcode: the fetch tracking has slipped.
            fetch->markFetch();
            disassembleCycles();
            cli.print("?halt: unknown opcode at ");
            cli.printlnHex(fetch->addr, 4);
            resetCpu();
            _regs->setIp(fetch->addr);
            return;
        }
        if (fetch != s && *seq)
            seq++;
        auto prefetch = s;
        _slept = false;
        // WAIT and STOP sleep after their prefetch.
        const auto sleeps = fetch->data == WAIT || _inst->isStop(fetch->data);
        while (*seq) {
            if (seq[1] == 0 && *seq == 'N') {
                // last cycle is prefetch; an SWI injected there would never
                // run after WAIT or STOP.
                if (!sleeps && haltSwitch()) {
                    fetch->markFetch();
                    if (is_internal(s->addr)) {
                        // Can't inject instruction for context save
                        resetCpu();
                        _regs->setIp(s->addr);
                    } else {
                        disassembleCycles();
                        _regs->save();
                    }
                    return;
                }
                prefetch = s;
                break;
            }
            if (*seq == 'V' && s->addr == vec_swi && checkBreakPoint(s))
                return;
            completeCycle(s);
            if (_writes == 5 && seq[1] != 'V') {
                // An interrupt stacked over the prefetched opcode, whose
                // sequence absorbed the stacking; only SWI writes five.
                cycle();  // read hi(vector)
                cycle();  // read lo(vector)
                s = prefetch = prepareCycle();
                break;
            }
            s = asleep ? nextCycle() : prepareCycle();
            asleep = false;
            if (s == nullptr) {
                fetch->markFetch();
                disassembleCycles();
                wakeUp();
                return;
            }
            if (_slept || (s->addr == VEC_RESET && !s->write())) {
                // woken by an interrupt's stacking, or a reset mid-sequence:
                // the top of the loop takes its vector read
                prefetch = s;
                break;
            }
            seq++;
            if (*seq == 'N')
                prefetch = s;
        }
        fetch->markFetch();
        fetch = prefetch;
        asleep = asleep || sleeps;
    }
}
#endif

bool PinsMc68HC08AZ0::rawStep() {
    const auto pc = _regs->nextIp();
    if (is_internal(pc))
        return false;
    // Not WAIT or STOP: waking the CPU isn't part of a step's sequence.
    const auto opc = _mems->read(pc);
    constexpr uint8_t WAIT = 0x8F;
    if (opc == WAIT || _inst->isStop(opc))
        return false;
    auto s = currCycle(pc);
    auto *seq = inst<InstMc68HC08>()->sequence(s);
    if (*seq == 0)
        return false;
    // MUL and DIV read the prefetch address again before using it.
    auto prefetched = false;
    uint16_t next = 0;
    while (*seq) {
        if (seq[1] == 0 && *seq == 'N') {
            // last cycle is prefetch
            break;
        }
        completeCycle(s);
        s = prepareCycle();
        seq++;
        if (*seq == 'N') {
            // prefetch
            prefetched = true;
            next = s->addr;
        }
        if (prefetched && s->addr == next)
            s->inject(InstMc68HC08::SWI);
    }
    return true;
}

bool PinsMc68HC08AZ0::is_internal(uint16_t addr) const {
    if (addr >= 0x0A00 && addr < 0xFE00)
        return false;
    if (addr >= 0xFF00)
        return false;
    if (addr >= 0x0450 && addr < 0x0500)
        return false;
    if (addr >= 0x0580 && addr < 0x0800)
        return false;
    if (addr >= 0xFE10 && addr < 0xFE1C)
        return false;
    return true;
}

}  // namespace mc68hc08az0
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
