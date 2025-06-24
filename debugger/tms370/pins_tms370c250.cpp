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

constexpr auto clkin_hi_ns = 100;   // 100
constexpr auto clkin_lo_ns = 100;   // 100
constexpr auto clkin_lo_dir = 40;   // 100
constexpr auto clkin_hi_addr = 40;  // 100

inline void clkin_hi() {
    digitalWriteFast(PIN_CLKIN, HIGH);
}

inline void clkin_lo() {
    digitalWriteFast(PIN_CLKIN, LOW);
}

inline void assert_reset() {
    digitalWriteFast(PIN_RESET, LOW);
    pinMode(PIN_RESET, OUTPUT_OPENDRAIN);
}

inline void negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
    pinMode(PIN_RESET, INPUT_PULLUP);
}

inline bool eds_asserted() {
    return digitalReadFast(PIN_EDS) == LOW;
}

const uint8_t PINS_HIGH[] = {
        PIN_CLKIN,
        PIN_INT1,
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

void clkin_cycle_lo() {
    clkin_hi();
    delayNanoseconds(clkin_hi_ns);
    clkin_lo();
}

void clkin_cycle() {
    clkin_cycle_lo();
    delayNanoseconds(clkin_lo_ns);
}

}  // namespace

PinsTms370C250::PinsTms370C250() {
    auto regs = new tms370::RegsTms370(this);
    _regs = regs;
    _devs = new tms370::DevsTms370();
    _mems = new MemsTms370C250(regs, this, _devs);
}

void PinsTms370C250::resetPins() {
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    SignalsTms370C250::resetCycles();
    resetCpu(true);
}

void PinsTms370C250::resetCpu(bool save) {
    assert_reset();
    for (uint_fast8_t i = 0; i < 10; i++)
        clkin_cycle();
    negate_reset();
    auto s = SignalsTms370C250::put();
    _regs->reset();
    if (save) {
        _regs->save();
        _regs->setIp(_mems->read16(InstTms370::VEC_RESET));
    } else {
        SignalsTms370C250::discard(s);
    }
}

Signals *PinsTms370C250::prepareCycle() {
    auto s = SignalsTms370C250::put();
    while (!eds_asserted()) {
        delayNanoseconds(clkin_lo_ns);
        clkin_cycle_lo();
    }
    s->getDirection();
    delayNanoseconds(clkin_lo_dir);
    assert_debug();
    clkin_hi();
    s->getAddr();
    delayNanoseconds(clkin_hi_addr);
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
    }
    while (eds_asserted()) {
        delayNanoseconds(clkin_lo_ns);
        clkin_cycle_lo();
    }
    return s;
}

void PinsTms370C250::pauseCpu() {
    digitalWriteFast(PIN_WAIT, LOW);
}

void PinsTms370C250::resumeCpu() {
    if (digitalReadFast(PIN_WAIT) == LOW) {
        // CPU may halt because of oscillator fault
        uint_fast8_t sysclk = 0;
        for (uint_fast8_t i = 0; i < 8; i++) {
            clkin_cycle();
            if (digitalReadFast(PIN_SYSCLK) != LOW)
                sysclk++;
        }
        if (sysclk < 2)
            resetCpu();
    }
    digitalWriteFast(PIN_WAIT, HIGH);
}

void PinsTms370C250::injectReads(const uint8_t *data, uint_fast8_t len) {
    resumeCpu();
    uint_fast8_t inj = 0;
    while (inj < len) {
        auto s = prepareCycle();
        if (inj < len)
            s->inject(data[inj]);
        completeCycle(s);
        if (s->read())
            inj++;
    }
    while (true) {
        auto s = prepareCycle();
        if (s->fetch())
            break;
        completeCycle(s);
    }
    pauseCpu();
}

uint16_t PinsTms370C250::captureWrites(
        const uint8_t *data, uint_fast8_t len, uint8_t *buf, uint_fast8_t max) {
    resumeCpu();
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
        if (s->read() && inj < len) {
            if (inj++ == 0)
                addr = s->addr;
        } else if (s->write() && cap < max) {
            buf[cap++] = s->data;
        }
    }
    while (true) {
        auto s = prepareCycle();
        if (s->fetch())
            break;
        completeCycle(s);
    }
    pauseCpu();
    return addr;
}

void PinsTms370C250::idle() {
    // #WAIT is asserted
    clkin_cycle_lo();
}

void PinsTms370C250::loop() {
    _regs->restore();
    SignalsTms370C250::resetCycles();
    resumeCpu();
    while (true) {
        auto s = prepareCycle();
        if (s->fetch()) {
            if (haltSwitch()) {
            stop:
                _regs->save();
                SignalsTms370C250::discard(s);
                return;
            }
            const auto inst = _mems->read(s->addr);
            if (inst == InstTms370::TRAP15 &&
                    _mems->read16(InstTms370::VEC_TRAP15) ==
                            InstTms370::VEC_TRAP15) {
                goto stop;
            }
            s->inject(inst);
        }
        completeCycle(s);
        _devs->loop();
    }
}

void PinsTms370C250::run() {
    saveBreakInsts();
    loop();
    restoreBreakInsts();
    disassembleCycles();
}

bool PinsTms370C250::rawStep() {
    const auto inst = _mems->read(_regs->nextIp());
    if (inst == InstTms370::TRAP15)
        return false;
    resumeCpu();
    auto s = prepareCycle();
    completeCycle(s->inject(inst));
    while (true) {
        s = prepareCycle();
        if (s->fetch()) {
            pauseCpu();
            return true;
        }
        completeCycle(s);
    }
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
    case tms370::INTR_INT1:
        digitalWriteFast(PIN_INT1, LOW);
        break;
    case tms370::INTR_INT2:
        digitalWriteFast(PIN_INT2, LOW);
        break;
    case tms370::INTR_INT3:
        digitalWriteFast(PIN_INT3, LOW);
        break;
    }
}

void PinsTms370C250::negateInt(uint8_t name) {
    switch (name) {
    case tms370::INTR_INT1:
        digitalWriteFast(PIN_INT1, HIGH);
        break;
    case tms370::INTR_INT2:
        digitalWriteFast(PIN_INT2, HIGH);
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
