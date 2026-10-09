#include "pins_tms320c15.h"
#include "debugger.h"
#include "devs_tms320.h"
#include "inst_tms3201x.h"
#include "mems_tms320c15.h"
#include "regs_tms3201x.h"
#include "signals_tms320c15.h"

namespace debugger {
namespace tms320c15 {

using tms3201x::InstTms3201X;

// clang-format off
/**
 * TMS320C15 bus cycle
 *           ___     ___     ___     ___
 *  CLKIN __|   |___|   |___|   |___|   |___
 *        ___\           \__________\
 * CLKOUT    |___________|           |______
 *        ___ _______                 ______
 *   #MEM ___X       |_______________|
 *        ___ _______                 ______
 *   #DEN ___X       |_______________|
 *        ___ ___________             ______
 *    #WE ___X           |___________|
 */

// tMC: nom:  50 ns; CLKIN cycle time
//  tc: nom: 200 ns; CLKOUT cycle time
// clang-format on

namespace {

constexpr auto clkin_lo_ns = 20;      // 25 ns
constexpr auto clkin_hi_ns = 20;      // 25 ns
constexpr auto clkin_hi_cntl = 20;    // 25 ns
constexpr auto clkin_hi_input = 5;    // 25 ns
constexpr auto clkin_hi_inject = 10;  // 25 ns
// One debug pin toggle's worth, before a sample and after a drive.
constexpr auto debug_ns = 10;

const uint8_t PINS_LOW[] = {
        PIN_RS,
        PIN_CLKIN,
};

const uint8_t PINS_HIGH[] = {
        PIN_BIO,
        PIN_INT,
};

const uint8_t PINS_INPUT[] = {
        PIN_CLKOUT,
        PIN_MEM,
        PIN_WE,
        PIN_DEN,
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
        PIN_AM4,
        PIN_AM5,
        PIN_AM6,
        PIN_AM7,
        PIN_AH8,
        PIN_AH9,
        PIN_AH10,
        PIN_AH11,
};

inline void clkin_lo() {
    digitalWriteFast(PIN_CLKIN, LOW);
}

inline void clkin_hi() {
    digitalWriteFast(PIN_CLKIN, HIGH);
}

void clkin_cycle_hi() {
    clkin_lo();
    delayNanoseconds(clkin_lo_ns);
    clkin_hi();
}

void clkin_cycle() {
    clkin_cycle_hi();
    delayNanoseconds(clkin_hi_ns);
}

void negate_reset() {
    digitalWriteFast(PIN_RS, HIGH);
}

}  // namespace

PinsTms320C15::PinsTms320C15() {
    _devs = new tms320::DevsTms320();
    auto regs = new tms3201x::RegsTms3201X(this);
    _regs = regs;
    _mems = new MemsTms320C15(regs);
}

void PinsTms320C15::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    for (auto i = 0; i < 10 * 2; i++)
        clkin_cycle();
    Cycles::reset();
    negate_reset();
    _regs->save();
}

void PinsTms320C15::idle() {
    auto s = completeCycle(prepareCycle()->inject(InstTms3201X::B));
    const auto addr = s->addr;
    Cycles::discard(s);
    Cycles::discard(completeCycle(prepareCycle()->inject(addr)));
}

// CLKIN periods to wait for a strobe: a running chip strobes #MEN, #DEN
// or #WE every machine cycle of 4, one held in reset or missing never.
constexpr auto strobe_clocks = 1000;

Signals *PinsTms320C15::prepareCycle() {
    auto s = Signals::put();
    for (auto n = 0; !s->getControl(); ++n) {
        if (n >= strobe_clocks) {
            // Give up rather than wedge the board, leaving the halt switch
            // to stop the run.
            cli.println("?halt: no bus cycle");
            return s;
        }
        clkin_cycle_hi();
        delayNanoseconds(clkin_hi_cntl);
    }
    clkin_lo();
    // assert_debug();
    delayNanoseconds(debug_ns);
    s->getAddr();
    // negate_debug();
    clkin_hi();
    return s;
}

Signals *PinsTms320C15::completeCycle(Signals *s) {
    clkin_lo();
    if (s->fetch()) {
        if (s->readMemory()) {
            s->data = _mems->read(s->addr);
        } else {
            delayNanoseconds(clkin_hi_inject);
        }
        clkin_hi();
        // assert_debug();
        s->outData();
        // negate_debug();
        delayNanoseconds(debug_ns);
        clkin_lo();
        Cycles::next();
        clkin_hi();
        delayNanoseconds(clkin_hi_input);
        s->inputMode();
        clkin_cycle_hi();
    } else if (s->read()) {
        if (s->readMemory()) {
            s->data = _devs->read(s->addr & 7);  // IN
        } else {
            delayNanoseconds(clkin_hi_inject);
        }
        clkin_hi();
        // assert_debug();
        s->outData();
        // negate_debug();
        delayNanoseconds(debug_ns);
        clkin_lo();
        Cycles::next();
        clkin_hi();
        delayNanoseconds(clkin_hi_input);
        s->inputMode();
        clkin_cycle_hi();
    } else if (s->write()) {
        // assert_debug();
        delayNanoseconds(debug_ns);
        s->getData();
        // negate_debug();
        clkin_hi();
        Cycles::next();
        clkin_lo();
        if (s->writeMemory()) {
            if (s->addr >= 8) {
                _mems->write(s->addr, s->data);  // TBLW
            } else {
                _devs->write(s->addr & 7, s->data);  // OUT
            }
        }
        clkin_hi();
    }
    return s;
}

void PinsTms320C15::setBio(bool high) {
    digitalWriteFast(PIN_BIO, high ? HIGH : LOW);
}

uint16_t PinsTms320C15::injectRead(uint16_t data) {
    auto s = prepareCycle();
    completeCycle(s->inject(data));
    return s->addr;
}

uint16_t PinsTms320C15::captureWrite() {
    const auto s = completeCycle(prepareCycle()->capture());
    return s->data;
}

bool PinsTms320C15::rawStep() {
    auto s = prepareCycle();
interrupt:
    const auto inst = _mems->read(s->addr);
    const auto cycles = InstTms3201X::cycles(inst);
    if (cycles == 0) {
        completeCycle(s->inject(InstTms3201X::B));
        completeCycle(prepareCycle()->inject(s->addr));
        Cycles::discard(s);
        return false;
    }
    completeCycle(s->inject(inst));
    for (uint_fast8_t i = 1; i < cycles; i++) {
        s = prepareCycle();
        if (s->fetch() && s->addr == InstTms3201X::ORG_INT) {
            goto interrupt;
        }
        completeCycle(s);
    }
    return true;
}

bool PinsTms320C15::step(bool show) {
    Cycles::reset();
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

#ifdef PROFILE_CYCLES
// For scripts/record-cycles.py: run until the trap ends the pattern, keep
// every cycle of it, and give up well before the ring wraps; the debug pin
// frames the run, to trigger a capture on. The trap is an OUT: only OUT and
// TBLW strobe #WE, and OUT always at a port address (0-7), as TBLW is only
// with ACC below 8, which the patterns never give it; so the first such
// write past the pattern's first instruction is the trap's, and the cycle
// before it the trap's fetch. The stop doesn't consult the tables. IN and
// OUT stay off the devices and TBLW off memory, so any operand is safe.
void PinsTms320C15::loop() {
    constexpr auto MAX_CYCLES = 96;
    const auto first = Signals::put();
    _profileEnd = nullptr;
    assert_debug();
    for (auto n = 0;; ++n) {
        _devs->loop();
        auto s = prepareCycle();
        if (s->read()) {
            s->inject(0);
        } else if (s->write()) {
            s->capture();
        }
        completeCycle(s);
        if (s->write() && s->addr < 8 && s->prev() != first) {
            negate_debug();
            _profileEnd = s->prev();
            s = Signals::put();
            Cycles::Hold hold;
            _regs->save();
            Cycles::discard(s);
            return;
        }
        if (n >= MAX_CYCLES || haltSwitch()) {
            negate_debug();
            cli.println(n >= MAX_CYCLES ? "?cycles" : "?halt");
            // Mid-instruction: let an I/O cycle go by and branch to self,
            // so idle() can't inject into a write; keep the last good save.
            const auto end = Signals::put();
            for (auto i = 0; i < 2; ++i) {
                s = prepareCycle();
                if (s->fetch()) {
                    completeCycle(s->inject(InstTms3201X::B));
                    completeCycle(prepareCycle()->inject(s->addr));
                    break;
                }
                completeCycle(s->read() ? s->inject(0) : s->capture());
            }
            Cycles::discard(end);
            return;
        }
    }
}
#else
void PinsTms320C15::loop() {
    while (true) {
        if (!rawStep() || haltSwitch()) {
            auto s = Signals::put();
            Cycles::Hold hold;
            _regs->save();
            Cycles::discard(s);
            return;
        }
        _devs->loop();
    }
}
#endif

void PinsTms320C15::run() {
    _regs->restore();
    Cycles::reset();
    saveBreakInsts();
    startRunTimer();
    loop();
    stopRunTimer();
    restoreBreakInsts();
    disassembleCycles();
}

void PinsTms320C15::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, 0xA000);
}

void PinsTms320C15::assertInt(uint8_t) {
    digitalWriteFast(PIN_INT, LOW);
}

void PinsTms320C15::negateInt(uint8_t) {
    digitalWriteFast(PIN_INT, HIGH);
}

void PinsTms320C15::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

const SignalsImpl *PinsTms320C15::findBacktraceStart() {
    return backtraceStartByFetchCount<Signals>(_lineLimit);
}

#ifdef PROFILE_CYCLES
// The opcode fetches rawStep() would take, counting each instruction's
// cycles from the run's first one up to the trap's fetch.
void PinsTms320C15::markFetches(const Signals *end) {
    const auto g = Signals::get();
    const auto cycles = g->diff(end);
    for (auto i = 0u; i < cycles;) {
        const auto s = g->next(i);
        const auto n = InstTms3201X::cycles(_mems->read(s->addr));
        s->setMatched(n);
        if (n == 0)
            break;
        i += n;
    }
}
#endif

void PinsTms320C15::printBacktrace() {
#ifdef PROFILE_CYCLES
    // Every cycle, with the matcher's marks.
    markFetches(_profileEnd ? _profileEnd->next() : Signals::put());
    printCycles();
#else
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles;) {
        const auto s = g->next(i);
        // A program read that decodes to no instruction, an operand or
        // table word where the window starts, prints as a plain cycle.
        const auto cyc = s->fetch() ? InstTms3201X::cycles(s->data) : 0;
        if (cyc) {
            const auto len = _mems->disassemble(s->addr, 1) - s->addr;
            for (uint_fast8_t j = len; j < cyc; j++) {
                const auto t = s->next(j);
                t->print();
            }
            i += cyc;
        } else {
            s->print();
            ++i;
        }
        idle();
    }
#endif
}

}  // namespace tms320c15
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
