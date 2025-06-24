#include "pins_tms370c250.h"
#include "debugger.h"
#include "devs_tms370.h"
#include "inst_tms370.h"
#include "mems_tms370c250.h"
#include "regs_tms370.h"
#include "signals_tms370c250.h"

namespace debugger {
namespace tms370c250 {

using tms370::InstTms370;

// clang-format off
/**
 */
// clang-format on

namespace {

constexpr auto clkin_hi_ns = 500; // 100
constexpr auto clkin_lo_ns = 500; // 100

inline void clkin_hi() {
    digitalWriteFast(PIN_CLKIN, HIGH);
}

inline void clkin_lo() {
    digitalWriteFast(PIN_CLKIN, LOW);
}

inline void negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
    pinMode(PIN_RESET, INPUT_PULLUP);
}

inline void assert_wait() {
    digitalWriteFast(PIN_WAIT, LOW);
}

inline void negate_wait() {
    digitalWriteFast(PIN_WAIT, HIGH);
}

inline bool eds_asserted() {
    return digitalReadFast(PIN_EDS) == LOW;
}

const uint8_t PINS_OPENDRAIN[] = {
        PIN_RESET,
};

const uint8_t PINS_HIGH[] = {
        PIN_CLKIN,
        PIN_NMI,
        PIN_INT2,
        PIN_INT3,
        PIN_WAIT,
};

const uint8_t PINS_INPUT[] = {
        PIN_DATA0,
        PIN_DATA1,
        PIN_DATA2,
        PIN_DATA3,
        PIN_DATA4,
        PIN_DATA5,
        PIN_DATA6,
        PIN_DATA7,
        PIN_DATA0,
        PIN_ADDR1,
        PIN_ADDR2,
        PIN_ADDR3,
        PIN_ADDR4,
        PIN_ADDR5,
        PIN_ADDR6,
        PIN_ADDR7,
        PIN_ADDR8,
        PIN_ADDR9,
        PIN_ADDR10,
        PIN_ADDR11,
        PIN_ADDR12,
        PIN_ADDR13,
        PIN_ADDR14,
        PIN_ADDR15,
        PIN_SYSCLK,
        PIN_EDS,
        PIN_RW,
        PIN_OCF,
};

inline void clkin_cycle() {
    clkin_hi();
    delayNanoseconds(clkin_hi_ns);
    clkin_lo();
    delayNanoseconds(clkin_lo_ns);
}

}  // namespace

PinsTms370C250::PinsTms370C250() {
    auto regs = new tms370::RegsTms370(this);
    _regs = regs;
    _devs = new tms370::DevsTms370();
    _mems = new MemsTms370C250(regs, _devs);
}

void PinsTms370C250::resetPins() {
    pinsMode(PINS_OPENDRAIN, sizeof(PINS_OPENDRAIN), OUTPUT_OPENDRAIN, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    for (uint_fast8_t i = 0; i < 100; i++)
        clkin_cycle();
    SignalsTms370C250::resetCycles();
    negate_reset();
    _regs->reset();
    _regs->save();
    _regs->setIp(_mems->read16(InstTms370::VEC_RESET));
}

Signals *PinsTms370C250::prepareCycle() {
    auto s = SignalsTms370C250::put();
    while (!eds_asserted()) {
        clkin_cycle();
    }
    clkin_hi();
    delayNanoseconds(clkin_hi_ns);
    s->getDirection();
    assert_debug();
    clkin_lo();
    delayNanoseconds(clkin_lo_ns);
    s->getAddrLow();
    clkin_hi();
    delayNanoseconds(clkin_hi_ns);
    s->getAddrMid();
    clkin_lo();
    delayNanoseconds(clkin_lo_ns);
    s->getAddrHigh();
    clkin_hi();
    delayNanoseconds(clkin_hi_ns);
    negate_debug();
    return s;
}

Signals *PinsTms370C250::completeCycle(Signals *_s) {
    clkin_lo();
    delayNanoseconds(clkin_lo_ns);
    auto s = static_cast<SignalsTms370C250 *>(_s);
    if (s->read()) {
        clkin_hi();
        if (s->readMemory()) {
            s->data = _mems->read(s->addr);
        } else {
            delayNanoseconds(clkin_hi_ns);
        }
        clkin_lo();
        assert_debug();
        s->outData();
        negate_debug();
        delayNanoseconds(clkin_lo_ns);
        clkin_hi();
        SignalsTms370C250::nextCycle();
        delayNanoseconds(clkin_hi_ns);
        clkin_lo();
        s->inputMode();
        delayNanoseconds(clkin_lo_ns);
        clkin_hi();
    } else {
        clkin_hi();
        delayNanoseconds(clkin_hi_ns);
        assert_debug();
        s->getData();
        negate_debug();
        clkin_lo();
        if (s->writeMemory()) {
            _mems->write(s->addr, s->data);
        } else {
            delayNanoseconds(clkin_lo_ns);
        }
        clkin_hi();
        SignalsTms370C250::nextCycle();
        delayNanoseconds(clkin_hi_ns);
        clkin_lo();
        delayNanoseconds(clkin_lo_ns);
        clkin_hi();
    }
    delayNanoseconds(clkin_hi_ns);
    while (eds_asserted())
        clkin_cycle();
    return s;
}

void PinsTms370C250::injectReads(
        const uint8_t *data, uint_fast8_t len, uint_fast8_t extra) {
    extra += len;
    for (uint_fast8_t inj = 0; inj < extra; inj++) {
        auto s = prepareCycle();
        if (inj < len)
            s->inject(data[inj]);
        completeCycle(s);
    }
}

uint16_t PinsTms370C250::captureWrites(
        const uint8_t *data, uint_fast8_t len, uint8_t *buf, uint_fast8_t max) {
    uint16_t addr = 0;
    uint_fast8_t inj = 0;
    uint_fast8_t cap = 0;
    while (inj < len || cap < max) {
        auto s = prepareCycle();
        if (inj < len)
            s->inject(data[inj]);
        if (cap < max)
            s->capture();
        completeCycle(s);
        if (s->read()) {
            if (inj < len)
                inj++;
        } else if (s->write()) {
            if (cap == 0)
                addr = s->addr;
            buf[cap++] = s->data;
        }
    }
    return addr;
}

void PinsTms370C250::idle() {
    // #WAIT is asserted
    clkin_cycle();
    clkin_cycle();
}

void PinsTms370C250::loop() {
    while (true) {
        _devs->loop();
        if (!rawStep() || haltSwitch())
            return;
    }
}

void PinsTms370C250::run() {
    _regs->restore();
    SignalsTms370C250::resetCycles();
    saveBreakInsts();
    loop();
    restoreBreakInsts();
    disassembleCycles();
    _regs->save();
}

bool PinsTms370C250::rawStep() {
    const auto inst = _mems->read(_regs->nextIp());
    if (inst == InstTms370::TRAP15)
        return false;
    negate_wait();
    auto s = prepareCycle();
    completeCycle(s->inject(inst));
    // TODO
    return true;
}

bool PinsTms370C250::step(bool show) {
    SignalsTms370C250::resetCycles();
    _regs->restore();
    if (show)
        SignalsTms370C250::resetCycles();
    if (rawStep()) {
        if (show)
            printCycles();
        _regs->save();
        return true;
    }
    return false;
}

void PinsTms370C250::assertInt(uint8_t name) {
    switch (name) {
    case tms370::INTR_NMI:
        digitalWriteFast(PIN_NMI, LOW);
        break;
    case tms370::INTR_INT3:
        digitalWriteFast(PIN_INT3, LOW);
        break;
    }
}

void PinsTms370C250::negateInt(uint8_t name) {
    switch (name) {
    case tms370::INTR_NMI:
        digitalWriteFast(PIN_NMI, HIGH);
        break;
    case tms370::INTR_INT3:
        digitalWriteFast(PIN_INT3, HIGH);
        break;
    }
}

}  // namespace tms370c250
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
