#include "pins_tms320c15.h"
#include "debugger.h"
#include "devs_tms320.h"
#include "digital_bus.h"
#include "inst_tms3201x.h"
#include "mems_tms320c15.h"
#include "regs_tms3201x.h"
#include "signals_tms320c15.h"

namespace debugger {
namespace tms320c15 {

// clang-format off
/**
 * TMS320C15 bus cycle
 *           ___     ___     ___     ___     ___     ___     __
 *  CLKIN __|   |___|   |___|   |___|   |___|   |___|   |___|
 *        ___\           \__________\           \ __________\
 * CLKOUT    |___________|           |___________|           |__
 *        ___ _______                 _______                 __
 *   #MEM ___X       |_______________|       |_______________|
 */

// tMC: nom:  50 ns; CLKIN cycle time
//  tc: nom: 200 ns; CLKOUT cycle time
// clang-format on

namespace {

constexpr auto clkin_lo_ns = 100;  // 25 ns
constexpr auto clkin_hi_ns = 100;  // 25 ns

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
    _mems = new MemsTms320C15();
    _regs = new tms3201x::RegsTms3201X(this);
}

void PinsTms320C15::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    for (auto i = 0; i < 100; i++)
        clkin_cycle();
    Signals::resetCycles();
    negate_reset();
    _regs->save();
}

void PinsTms320C15::idle() {
    auto s = completeCycle(prepareCycle()->inject(tms3201x::InstTms3201X::B));
    const auto addr = s->addr;
    Signals::discard(s);
    Signals::discard(completeCycle(prepareCycle()->inject(addr)));
}

Signals *PinsTms320C15::prepareCycle() {
    auto s = Signals::put();
    assert_debug();
    while (!s->getControl()) {
        negate_debug();
        delayNanoseconds(clkin_hi_ns);
        clkin_cycle_hi();
    }
    s->getAddr();
    negate_debug();
    return s;
}

Signals *PinsTms320C15::completeCycle(Signals *s) {
    if (s->fetch()) {
        if (s->readMemory()) {
            s->data = _mems->read(s->addr);
        }
        assert_debug();
        s->outData();
        negate_debug();
        clkin_cycle_hi();
        s->inputMode();
    } else if (s->read()) {
        if (s->readMemory()) {
            s->data = _devs->read(s->addr & 7);  // IN
        }
        s->outData();
        clkin_cycle_hi();
        s->inputMode();
    } else if (s->write()) {
        clkin_cycle_hi();
        negate_debug();
        s->getData();
        negate_debug();
        if (s->writeMemory()) {
            if (s->addr >= 8) {
                _mems->write(s->addr, s->data);  // TBLR
            } else {
                _devs->write(s->addr & 7, s->data);  // OUT
            }
        }
    }
    Signals::nextCycle();
    auto n = Signals::put();
    do {
        clkin_cycle_hi();
    } while (n->getControl());
    return s;
}

uint16_t PinsTms320C15::injectRead(uint16_t data) {
    auto s = prepareCycle();
    completeCycle(s->inject(data));
    return s->addr;
}

uint16_t PinsTms320C15::captureWrite(const uint16_t *data, uint_fast8_t len) {
    for (uint_fast8_t i = 0; i < len; i++)
        injectRead(data[i]);
    const auto s = completeCycle(prepareCycle()->capture());
    return s->data;
}

bool PinsTms320C15::step(bool show) {
    return false;
}

void PinsTms320C15::run() {
    ;
}

void PinsTms320C15::setBreakInst(uint32_t addr) const {
    ;
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
    for (auto i = 0; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

#if 0
void PinsTms320C15::disassembleCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles;) {
        const auto s = g->next(i);
        if (s->fetch()) {
            const auto len = _mems->disassemble(s->addr, 1) - s->addr;
            // print bus cycles other than instruction bytes
            for (uint_fast8_t j = 1; j < len / 2; j++) {
                const auto t = s->next(j);
                if (t->addr < s->addr || t->addr >= s->addr + len)
                    t->print();
            }
            i += len / 2;
        } else {
            s->print();
            ++i;
        }
        idle();
    }
}
#endif

}  // namespace tms320c15
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
