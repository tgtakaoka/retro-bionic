#include "pins_tms370cx5x.h"
#include "debugger.h"
#include "devs_tms370.h"
#include "inst_tms370.h"
#include "mems_tms370cx5x.h"
#include "regs_tms370cx5x.h"
#include "signals_tms370cx5x.h"

namespace debugger {
namespace tms370cx5x {

using tms370::InstTms370;

// clang-format off
/**
 * CAVIATE: The following timing chart is when AUTO_WAIT_DISABLE is set. Unfortunately any silicon
 * chip in my hand can't response interrupt in this configuration (see RegsTms370::reset()).  The
 * current implementation use AUTO WAIT enabled, so that external memory access use 2 SYSCLK
 * cycles. Though TMS370 Family User's Guide says that external memory access use 3 SYSCLK cycles
 * when AUTO WAIT enabled (Table 4-2. Wait-State Control Bits).
 *
 *            __    __    __    __    __    __    __    __    __    __    __    __    __    __    __    __
 *  CLKIN |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__
 *        ____  \    _\___       _\___  \    _\___       _\___  \    _\___       _\___  \    _____       ____
 * SYSCLK     |_____|     |_____|     |_____|     |_____|     |_____|     |_____|     |_____|     |_____|
 *        ____________|         | |___________|         | |___________|   |     | |___________          | |__
 *   #EDS             |___________|           |___________|           |___________|           |___________|
 *        _____________________________________________________|                        |____________________
 *   R/#W ______/                                              |________________________|
 *        ______|                       |____________________________________________________________________
 *   #OCF       |_______________________|                                               \____________________
 *        _____   _____________________   ____________________   ______________________   ___________________
 *   ADDR _____>-<_____________________>-<____________________>-<______________________>-<___________________
 *                               __                       __              |_________
 *   DATA ----------------------<__>---------------------<__>-------------<_________>-------------
 *        ______________________v_______________________v_______________________v_______________________v___
 *  #WAIT _____________________/ \_____________________/ \_____________________/ \_____________________/ \__
 */
// clang-format on

namespace {

//   CLKIN: min 2.0 MHz, max 20 MHz
//  SYSCLK: min 0.5 MHz, max 5.0 MHz
//      tc: min 200 ns, max 2000 ns ; SYSCLK cycle time
// CIH-SCL: max 100 ns       ; CLKIN+ to SYSCLK-
//   SCL-A: max 0.25tc+75 ns ; address valid from SYSCLK-
//    EH-A: min 0.5tc-60 ns  ; address hold after #EDS+
//   tv(A): min 0.5tc-90 ns  ; address valid to #EDS-
//      tw: min tc-80, max tc+40 ; #EDS low width
//  EL-DVR: max tc-95 ns     ; data hold after #EDS+
//  tsu(D): 0.75tc-80 ns     ; data setup for #EDS+
//   EH-DW: 0.75tc+15 ns     ; data hold after #EDS+

constexpr auto clkin_ns = 5;

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
    pinMode(PIN_RESET, INPUT_PULLUP);
    digitalWriteFast(PIN_RESET, HIGH);
}

inline void assert_wait() {
    digitalWriteFast(PIN_WAIT, LOW);
}

inline void negate_wait() {
    digitalWriteFast(PIN_WAIT, HIGH);
}

inline bool reset_asserted() {
    return digitalReadFast(PIN_RESET) == LOW;
}

inline bool eds_asserted() {
    return digitalReadFast(PIN_EDS) == LOW;
}

inline bool ocf_asserted() {
    return digitalReadFast(PIN_OCF) == LOW;
}

inline bool wait_asserted() {
    return digitalReadFast(PIN_WAIT) == LOW;
}

const uint8_t PINS_HIGH[] = {
        PIN_CLKIN,
        PIN_WAIT,
        PIN_INT1,
        PIN_INT2,
        PIN_INT3,
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
        PIN_ADDR0,
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
    delayNanoseconds(clkin_ns);
    clkin_lo();
}

void clkin_cycle() {
    clkin_cycle_lo();
    delayNanoseconds(clkin_ns);
}

}  // namespace

PinsTms370Cx5x::PinsTms370Cx5x() {
    auto regs = new RegsTms370Cx5x(this);
    _regs = regs;
    _devs = new tms370::DevsTms370();
    _mems = new MemsTms370Cx5x(regs, this, _devs);
}

void PinsTms370Cx5x::resetPins() {
    assert_reset();
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    resetCpu();
    _regs->save();
    _regs->setIp(_mems->read16(InstTms370::VEC_RESET));
}

Signals *PinsTms370Cx5x::resetCpu() {
    SignalsTms370Cx5x::resetCycles();
    assert_reset();
    for (uint_fast8_t i = 0; i < 10; i++)
        clkin_cycle();
    negate_reset();
    while (reset_asserted())
        clkin_cycle();
    auto s = SignalsTms370Cx5x::put();
    _regs->reset();
    return s;
}

Signals *PinsTms370Cx5x::prepareCycle() {
    auto s = SignalsTms370Cx5x::put();
    while (!eds_asserted()) {
        delayNanoseconds(clkin_ns);
        clkin_cycle_lo();
    }
    // assert_debug();
    s->getDirection();
    clkin_hi();
    s->getAddr();
    // negate_debug();
    return s;
}

Signals *PinsTms370Cx5x::completeCycle(Signals *_s) {
    clkin_lo();
    delayNanoseconds(clkin_ns);
    auto s = static_cast<SignalsTms370Cx5x *>(_s);
    // assert_debug();
    if (s->read()) {
        clkin_hi();
        if (s->readMemory()) {
            s->data = _mems->read(s->addr);
        } else {
            delayNanoseconds(clkin_ns);
        }
        clkin_lo();
        // toggle_debug();
        s->outData();
        // toggle_debug();
        clkin_hi();
        SignalsTms370Cx5x::nextCycle();
        clkin_lo();
        s->inputMode();
    } else {
        clkin_hi();
        SignalsTms370Cx5x::nextCycle();
        clkin_lo();
        // toggle_debug();
        delayNanoseconds(0);
        s->getData();
        // toggle_debug();
        clkin_hi();
        if (s->writeMemory()) {
            _mems->write(s->addr, s->data);
        } else {
            delayNanoseconds(clkin_ns);
        }
        clkin_lo();
    }
    while (eds_asserted()) {
        delayNanoseconds(clkin_ns);
        clkin_cycle_lo();
    }
    // negate_debug();
    return s;
}

void PinsTms370Cx5x::pauseCpu() {
    while (ocf_asserted()) {
        delayNanoseconds(clkin_ns);
        clkin_cycle_lo();
    }
    while (!ocf_asserted()) {
        delayNanoseconds(clkin_ns);
        clkin_cycle_lo();
    }
    assert_wait();
    clkin_cycle_lo();
}

void PinsTms370Cx5x::resumeCpu() {
    if (wait_asserted()) {
        // CPU may halt because of oscillator fault
        uint_fast8_t sysclk = 0;
        for (uint_fast8_t i = 0; i < 8; i++) {
            clkin_cycle_lo();
            if (digitalReadFast(PIN_SYSCLK) == LOW) {
                negate_wait();
                return;
            }
            sysclk++;
        }
        if (sysclk < 2)
            resetCpu();
        negate_wait();
    }
}

void PinsTms370Cx5x::injectReads(const uint8_t *data, uint_fast8_t len) {
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
    pauseCpu();
}

uint16_t PinsTms370Cx5x::captureWrites(
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
    pauseCpu();
    return addr;
}

void PinsTms370Cx5x::idle() {
    // #WAIT is asserted
    clkin_cycle();
    clkin_cycle_lo();
}

void PinsTms370Cx5x::loop() {
    resetCpu();
    _regs->restore();
    resumeCpu();
    SignalsTms370Cx5x::resetCycles();
    while (true) {
        auto s = prepareCycle();
        if (s->fetch()) {
            if (haltSwitch()) {
            stop:
                _regs->save();
                SignalsTms370Cx5x::discard(s);
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

void PinsTms370Cx5x::run() {
    saveBreakInsts();
    loop();
    restoreBreakInsts();
    disassembleCycles();
}

bool PinsTms370Cx5x::rawStep() {
    const auto inst = _mems->read(_regs->nextIp());
    if (inst == InstTms370::TRAP15)
        return false;
    resumeCpu();
    auto s = prepareCycle();
    completeCycle(s->inject(inst));
    s->clearInject();
    // finish instruction fetch
    while (ocf_asserted()) {
        delayNanoseconds(clkin_ns);
        clkin_cycle_lo();
    }
    // execute bys cycles until #OCF asserted
    while (true) {
        while (!eds_asserted() && !ocf_asserted())
            clkin_cycle_lo();
        if (ocf_asserted()) {
            assert_wait();
            clkin_cycle_lo();
            return true;
        }
        s = prepareCycle();
        completeCycle(s);
    }
}

bool PinsTms370Cx5x::step(bool show) {
    resetCpu();
    _regs->restore();
    if (show)
        SignalsTms370Cx5x::resetCycles();
    if (rawStep()) {
        if (show)
            printCycles();
        _regs->save();
        return true;
    }
    return false;
}

void PinsTms370Cx5x::assertInt(uint8_t name) {
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

void PinsTms370Cx5x::negateInt(uint8_t name) {
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

}  // namespace tms370cx5x
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
