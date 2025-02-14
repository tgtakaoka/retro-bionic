#include "pins_tms99105.h"
#include "debugger.h"
#include "devs_tms9900.h"
#include "digital_bus.h"
#include "inst_tms9900.h"
#include "mems_tms99105.h"
#include "regs_tms99105.h"
#include "signals_tms99105.h"

#define DEBUG(e)
// #define DEBUG(e) e

namespace debugger {
namespace tms99105 {

// clang-format off
/**
 * TMS99105 bus cycle
 *    PHI    |  1  |  2  |  3  |  4  |  1  |
 *            __    __    __    __    __    __
 *  CLKIN |__|  |__|  |__|  |__|  |__|  |__|  |
 *        ___\     \     \ __________\
 * CLKOUT     |___________|           |________
 *                   _____
 * ALATCH __________/     \___________________/
 *        __________                        ___
 *   #MEM __________\______________________/___
 *        ________________             ________
 *   R/#W                 \___________/
 *        ________________             ________
 *    #RD                 \___________/
 *        ________________             ________
 *    #WE                 \___________/
 *        ________________ ____________________
 *   DATA ________________X___________R________
 *                         ___________
 *    BST ________________/           \________
 *        _____________________________________
 *  READY ___________________________/  \______
 */
// clang-format on

// fext: min 12MHz, max 24 MHz
//  tc1: min  41.25 ns ; 1/fext
//  tc2: min 165 ns ; 4tc1 cycle of CLKOUT
//  tWH: min  82 ns ; CLKOUT high pulse width
//  tWL: min  82 ns ; CLKOUT low pulse width

namespace {

constexpr auto clkin_lo_ns = 5;  // 21
constexpr auto clkin_hi_ns = 5;  // 21

const uint8_t PINS_LOW[] = {
        PIN_CLKIN,
        PIN_RESET,
        PIN_READY,
};

const uint8_t PINS_HIGH[] = {
        PIN_NMI,
        PIN_INTREQ,
        PIN_APP,
        PIN_HOLD,
        PIN_IC3,
        PIN_IC2,
        PIN_IC1,
        PIN_IC0,
};

const uint8_t PINS_INPUT[] = {
        PIN_CLKOUT,
        PIN_MEM,
        PIN_RW,
        PIN_WE,
        PIN_ALATCH,
        PIN_RD,
        PIN_D15,
        PIN_AD14,
        PIN_AD13,
        PIN_AD12,
        PIN_AD11,
        PIN_AD10,
        PIN_AD9,
        PIN_AD8,
        PIN_AD7,
        PIN_AD6,
        PIN_AD5,
        PIN_AD4,
        PIN_AD3,
        PIN_AD2,
        PIN_AD1,
        PIN_AD0,
        PIN_BST1,
        PIN_BST2,
        PIN_BST3,
};

inline void clkin_lo() {
    digitalWriteFast(PIN_CLKIN, LOW);
}

inline void clkin_hi() {
    digitalWriteFast(PIN_CLKIN, HIGH);
}

auto signal_clkout() {
    return digitalReadFast(PIN_CLKOUT);
}

void clkin_cycle_lo() {
    clkin_hi();
    delayNanoseconds(clkin_hi_ns);
    clkin_lo();
}

void clkin_cycle() {
    clkin_cycle_lo();
    delayNanoseconds(clkin_lo_ns);
}

void system_cycle() {
    clkin_cycle();
    clkin_cycle();
    clkin_cycle();
    clkin_cycle();
}

void negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
}

void assert_ready() {
    digitalWriteFast(PIN_READY, HIGH);
}

auto ready_asserted() {
    return digitalReadFast(PIN_READY) != LOW;
}

}  // namespace

PinsTms99105::PinsTms99105() {
    _devs = new tms9900::DevsTms9900();
    _mems = new MemsTms99105(_devs);
    _regs = new RegsTms99105(this, _mems);
}

void PinsTms99105::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    // Synchronize CLKIN to CLKOUT
    assert_debug();
    while (signal_clkout() == LOW)
        clkin_cycle();
    while (signal_clkout() != LOW)
        clkin_cycle();
    negate_debug();
    // phi1

    const auto macro = regs<RegsTms99105>()->macroMode();
    if (macro != MacroMode::MACRO_STANDARD)
        digitalWriteFast(PIN_APP, LOW);
    for (auto i = 0; i < 10; ++i)
        system_cycle();
    negate_reset();

    Signals::resetCycles();
    pauseCycle();
    _regs->save();
    if (macro != MacroMode::MACRO_BASELINE)
        digitalWriteFast(PIN_APP, HIGH);
    checkCpuType();
    _regs->reset();
}

tms9900::Signals *PinsTms99105::prepareCycle() const {
    auto s = Signals::put();
    // phi2
    clkin_cycle_lo();
    delayNanoseconds(clkin_lo_ns);
    s->getAddress();
    // phi3
    clkin_cycle_lo();
    return s;
}

tms9900::Signals *PinsTms99105::completeCycle(tms9900::Signals *_s) const {
    auto s = static_cast<Signals *>(_s);
    if (ready_asserted()) {
        s->getControl();
        if (s->readEnable()) {
            // phi4
            clkin_hi();
            if (s->readMemory()) {
                auto m = mems<MemsTms99105>();
                if (s->macrostore()) {
                    s->data = m->readMacro(s->addr);
                } else {
                    s->data = m->read(s->addr);
                }
            } else {
                delayNanoseconds(clkin_hi_ns);
            }
            clkin_lo();
            s->outData();
            delayNanoseconds(clkin_lo_ns);
            // phi1
            clkin_hi();
            Signals::nextCycle();
            clkin_lo();
            s->inputMode();
        } else if (s->writeEnable()) {
            // phi4
            clkin_hi();
            s->getData();
            clkin_lo();
            if (s->writeMemory()) {
                auto m = mems<MemsTms99105>();
                if (s->macrostore()) {
                    m->writeMacro(s->addr, s->data);
                } else {
                    m->write(s->addr, s->data);
                }
            } else {
                delayNanoseconds(clkin_lo_ns);
            }
            // phi1
            clkin_hi();
            Signals::nextCycle();
            clkin_lo();
        } else {
            goto nobus;
        }
    } else {
    nobus:
        // phi4
        clkin_cycle();
        // phi1
        clkin_cycle_lo();
    }
    return s;
}

tms9900::Signals *PinsTms99105::pauseCycle() {
    auto s = PinsTms9900::pauseCycle();
    _addr = s->addr;
    return s;
}

tms9900::Signals *PinsTms99105::resumeCycle(uint16_t pc) const {
    auto s = Signals::put();
    s->addr = pc ? pc : _addr;
    s->getControl();
    assert_ready();
    return s;
}

void PinsTms99105::injectReads(const uint16_t *data, uint8_t len) {
    auto s = resumeCycle();
    for (auto i = 0; i < len;) {
        completeCycle(s->inject(data[i]));
        if (s->read()) {
            i++;
        }
        s = (i < len) ? prepareCycle() : pauseCycle();
    }
}

void PinsTms99105::captureCycles(uint16_t *buf, uint8_t len, bool write) {
    auto s = resumeCycle();
    for (auto i = 0; i < len;) {
        completeCycle(s->capture());
        buf[i] = s->data;
        if (write) {
            if (s->write()) {
                ++i;
            }
        } else {
            if (s->read()) {
                ++i;
            }
        }
        s = (i < len) ? prepareCycle() : pauseCycle();
    }
}

void PinsTms99105::checkCpuType() {
    auto s = resumeCycle(_mems->read(tms9900::InstTms9900::VEC_RESET + 2));
    // Inject CIR instruction and capture all writes.
    s->inject(0x0C80);  // CIR R0
    auto assert_nmi = true;
    uint_fast8_t writes = 0;
    while (true) {
        if (s->addr == mems<MemsTms99105>()->vec_nmi())
            break;
        completeCycle(s->capture());
        if (assert_nmi && s->fetch()) {
            assertInt(tms9900::INTR_NMI);
            assert_nmi = false;
        }
        if (static_cast<tms99105::Signals *>(s)->writeEnable())
            writes++;
        DEBUG(cli.print("@@ checkCpuType: "));
        DEBUG(s->print());
        s = prepareCycle()->capture();
    }
    negateInt(tms9900::INTR_NMI);
    _regs->save();
    const auto tms99110 = (writes == 2);
    regs<RegsTms99105>()->setCpuType(tms99110);
}

void PinsTms99105::assertInt(uint8_t name_) {
    const auto name = static_cast<tms9900::IntrName>(name_);
    if (name == tms9900::INTR_NMI) {
        digitalWriteFast(PIN_NMI, LOW);
    } else {
        busWrite(IC, name_ / 4);
        digitalWriteFast(PIN_INTREQ, LOW);
    }
}

void PinsTms99105::negateInt(uint8_t name_) {
    const auto name = static_cast<tms9900::IntrName>(name_);
    if (name == tms9900::INTR_NMI) {
        digitalWriteFast(PIN_NMI, HIGH);
    } else {
        digitalWriteFast(PIN_INTREQ, HIGH);
        busWrite(IC, 0xF);
    }
}

}  // namespace tms99105
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
