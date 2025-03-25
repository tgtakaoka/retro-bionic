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
    Signals::resetCycles();
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
    Signals::nextCycle();
    s = SignalsMc68HC08AZ0::put();
    do {
        osc1_cycle_lo();
        SignalsMc68HC08AZ0::inputMode();
    } while (s->getControl());
    return s;
}

void PinsMc68HC08AZ0::loop() {
    const auto vec_swi = mems<mc6805::MemsMc6805>()->vecSwi();
    auto s = prepareCycle();
    auto fetch = s;
    while (true) {
        _devs->loop();
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
        if (fetch != s && *seq)
            seq++;
        auto prefetch = s;
        while (*seq) {
            if (seq[1] == 0 && *seq == 'N') {
                // last cycle is prefetch
                if (haltSwitch()) {
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
            s = prepareCycle();
            seq++;
            if (*seq == 'N')
                prefetch = s;
        }
        fetch->markFetch();
        fetch = prefetch;
    }
}

bool PinsMc68HC08AZ0::rawStep() {
    const auto pc = _regs->nextIp();
    if (is_internal(pc))
        return false;
    auto s = currCycle(pc);
    auto *seq = inst<InstMc68HC08>()->sequence(s);
    if (*seq == 0)
        return false;
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
            s->inject(InstMc68HC08::SWI);
        }
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
