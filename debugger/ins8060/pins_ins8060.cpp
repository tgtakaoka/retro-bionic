#include "pins_ins8060.h"
#include "debugger.h"
#include "devs_ins8060.h"
#include "inst_ins8060.h"
#include "mems_ins8060.h"
#include "regs_ins8060.h"
#include "signals_ins8060.h"

namespace debugger {
namespace ins8060 {

// clang-format off
/**
 * INS8070 External Bus cycle
 *        __    __    __    __    __    __    __    __    __    __
 *    XIN   |__| 1|__| 2|__| 3|__| 4|__| 5|__| 6|__| 7|__| 8|__|  |
 *          \ __  \ __  \ __  \ __  \ _\  \ __  \ __  \ __  \ __
 *   XOUT |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__
 *        _____                |     |  |  \           \     \  ___
 *  #BREQ      |_______________|_____|__|___|___________|______|
 *        ____                 |     |  |   |           |      ____
 * #ENOUT     |________________|_____|__|___|___________|_____|
 *        __________           |     |  |   |           |
 *  #ENIN           |__________|_____|__|___|___________|_________|
 *        _____________________|     |__|___|___________|__________
 *   #ADS                      |_____|  |   |           |
 *        ______________________________|   |           |__________
 *   #RDS                               |___|___________|
 *        __________________________________|           |__________
 *   #WDS                                   |___________|
 *                          ______________________________
 *    AD ------------------<______________________________>--------
 *                          ___________ __________________
 *    DB ------------------<_____AD____X_________W_____R__>--------
 */
// clang-format on

namespace {
//   fx: max 4.0 MHz    ; XIN frequencey
//   Tc: min 500 ns     ; 2 cycles of XIN
//  TW0: min 120 ns     ; XIN low width
//  TW1: min 120 ns     ; XIN high width
// TSAD: min Tc/2-165 ns; address setup to #ADS-
// THAD: min       50 ns; address hold from #RDS/#WDS+
// TSST: min Tc/2-150 ns; status setup to #ADS-
// THST: min       50 ns; status hold from #ADS-
// TSRD: min      175 ns; data setup to #RDS+
// THRD: min        0 ns; data hold from #RDS+
// TSWD: min Tc/2-200 ns; data setup to #WDS-
// THWD:          100 ns; data hold from #WDS+

constexpr auto xin_hi_ns = 100;      // 120
constexpr auto xin_lo_ns = 80;       // 120
constexpr auto xin_hi_ads = 80;      // 120
constexpr auto xin_lo_ads = 100;     // 120
constexpr auto xin_hi_addr = 14;     // 120
constexpr auto xin_lo_addr = 50;     // 120
constexpr auto xin_hi_wds = 100;     // 120
constexpr auto xin_hi_bus = 83;      // 120
constexpr auto xin_lo_wds = 70;      // 120
constexpr auto xin_lo_get = 55;      // 120
constexpr auto xin_hi_capture = 80;  // 120
constexpr auto xin_lo_write = 94;    // 120
constexpr auto xin_lo_inject = 85;   // 120
constexpr auto xin_hi_output = 40;   // 120
constexpr auto xin_hi_rds = 100;     // 120
constexpr auto xin_lo_rds = 70;      // 120
constexpr auto xin_lo_input = 40;    // 120
constexpr auto xin_hi_read = 80;     // 120
constexpr auto xin_bus_end = 20;     // 120
constexpr auto xin_hi_enout = 100;   // 120
constexpr auto xin_lo_enout = 70;    // 120

inline auto signal_breq() {
    return digitalReadFast(PIN_BREQ);
}

inline auto signal_enout() {
    return digitalReadFast(PIN_ENOUT);
}

inline auto signal_ads() {
    return digitalReadFast(PIN_ADS);
}

inline auto signal_wds() {
    return digitalReadFast(PIN_WDS);
}

inline auto signal_rds() {
    return digitalReadFast(PIN_RDS);
}

void assert_sense_a() {
    digitalWriteFast(PIN_SENSEA, HIGH);
}

void negate_sense_a() {
    digitalWriteFast(PIN_SENSEA, LOW);
}

void assert_enin() {
    digitalWriteFast(PIN_ENIN, LOW);
}

void negate_enin() {
    digitalWriteFast(PIN_ENIN, HIGH);
}

void negate_reset() {
    digitalWriteFast(PIN_RST, HIGH);
}

constexpr uint8_t PINS_LOW[] = {
        PIN_XIN,
        PIN_RST,
        PIN_SENSEA,
        PIN_SENSEB,
        PIN_SIN,
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_CONT,
        PIN_ENIN,
};

constexpr uint8_t PINS_PULLUP[] = {
        PIN_BREQ,
        PIN_HOLD,
};

constexpr uint8_t PINS_INPUT[] = {
        PIN_DB0,
        PIN_DB1,
        PIN_DB2,
        PIN_DB3,
        PIN_DB4,
        PIN_DB5,
        PIN_DB6,
        PIN_DB7,
        PIN_ADL00,
        PIN_ADL01,
        PIN_ADL02,
        PIN_ADL03,
        PIN_ADL04,
        PIN_ADL05,
        PIN_ADL06,
        PIN_ADL07,
        PIN_ADM08,
        PIN_ADM09,
        PIN_ADM10,
        PIN_ADM11,
        PIN_FLAG0,
        PIN_FLAG1,
        PIN_FLAG2,
        PIN_RDS,
        PIN_WDS,
        PIN_ADS,
        PIN_ENOUT,
        PIN_SOUT,
};

inline void xin_hi() {
    digitalWriteFast(PIN_XIN, HIGH);
}

inline void xin_cycle_lo() {
    digitalWriteFast(PIN_XIN, HIGH);
    delayNanoseconds(xin_hi_ns);
    digitalWriteFast(PIN_XIN, LOW);
}

inline void xin_cycle() {
    xin_cycle_lo();
    delayNanoseconds(xin_lo_ns);
}

}  // namespace

PinsIns8060::PinsIns8060() {
    _regs = new RegsIns8060(this);
    _devs = new DevsIns8060();
    _mems = new MemsIns8060(_devs);
}

void PinsIns8060::xin_lo() const {
    digitalWriteFast(PIN_XIN, LOW);
    devs<DevsIns8060>()->sciLoop();
}

void PinsIns8060::resetPins() {
    // Assert reset condition
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_PULLUP, sizeof(PINS_PULLUP), INPUT_PULLUP);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    Signals::resetCycles();
    // #RST must remain low for a minimum of 4 Tc.
    for (auto i = 0; i < 2 * 4; i++)
        xin_cycle();
    negate_enin();
    negate_reset();
    // The #BREQ output goes low, indicating the start of execution;
    // this occurs at a time whithin 13 Tc after #RST is set high.
    _regs->save();
}

Signals *PinsIns8060::prepareCycle() const {
    // XIN=L
    auto s = Signals::put();
    noInterrupts();
    while (true) {
        xin_hi();
        if (signal_ads() == LOW) {
            // assert_debug();
            s->getAddr();
            // negate_debug();
            delayNanoseconds(xin_hi_addr);
            xin_lo();
            if (!s->fetch())
                delayNanoseconds(xin_lo_addr);
            // XIN=L
            interrupts();
            return s;
        }
        delayNanoseconds(xin_hi_ads);
        xin_lo();
        delayNanoseconds(xin_lo_ads);
    }
}

Signals *PinsIns8060::completeCycle(Signals *s) const {
    // XIN=L
    xin_hi();
    delayNanoseconds(xin_hi_bus);
    if (s->write()) {
        while (true) {
            xin_lo();
            if (signal_wds() == LOW)
                break;
            delayNanoseconds(xin_lo_wds);
            xin_hi();
            delayNanoseconds(xin_hi_wds);
        }
        // assert_debug();
        s->getData();
        // negate_debug();
        delayNanoseconds(xin_lo_get);
        xin_hi();
        if (s->writeMemory()) {
            _mems->write(s->addr, s->data);
        } else {
            delayNanoseconds(xin_hi_capture);
        }
        xin_lo();
        delayNanoseconds(xin_lo_write);
    } else {
        xin_lo();
        if (s->readMemory()) {
            s->data = _mems->read(s->addr);
        } else {
            delayNanoseconds(xin_lo_inject);
        }
        xin_hi();
        // assert_debug();
        s->outData();
        delayNanoseconds(xin_hi_output);
        // negate_debug();
        while (true) {
            xin_lo();
            if (signal_rds() != LOW) {
                delayNanoseconds(xin_lo_input);
                // assert_debug();
                Signals::inputMode();
                // negate_debug();
                break;
            }
            delayNanoseconds(xin_lo_rds);
            xin_hi();
            delayNanoseconds(xin_hi_rds);
        }
    }
    // XIN=L
    xin_hi();
    Signals::nextCycle();
    delayNanoseconds(xin_bus_end);
    if (s->read()) {
        // Read-Modify-Write bus cycle keeps #BREQ low.
        for (uint_fast8_t i = 0; i < 3; i++) {
            xin_lo();
            // assert_debug();
            if (signal_enout() == LOW)
                break;
            // negate_debug();
            delayNanoseconds(xin_lo_enout);
            xin_hi();
            delayNanoseconds(xin_hi_enout);
        }
        // negate_debug();
    }
    xin_lo();
    // XIN=L
    return s;
}

Signals *PinsIns8060::inject(uint8_t data) const {
    const auto s = prepareCycle()->inject(data);
    return completeCycle(s);
}

void PinsIns8060::execInst(const uint8_t *inst, uint8_t len) const {
    execute(inst, len, nullptr, nullptr, 0);
}

void PinsIns8060::captureWrites(const uint8_t *inst, uint8_t len,
        uint16_t *addr, uint8_t *buf, uint8_t max) const {
    execute(inst, len, addr, buf, max);
}

void PinsIns8060::execute(const uint8_t *inst, uint8_t len, uint16_t *addr,
        uint8_t *buf, uint8_t max) const {
    uint8_t inj = 0;
    uint8_t cap = 0;
    assert_enin();
    while (inj < len || cap < max) {
        auto s = prepareCycle();
        if (inj < len)
            s->inject(inst[inj]);
        if (cap < max)
            s->capture();
        completeCycle(s);
        if (s->read()) {
            if (inj < len)
                ++inj;
        } else if (s->write()) {
            if (cap == 0 && addr)
                *addr = s->addr;
            if (buf && cap < max)
                buf[cap++] = s->data;
        }
    }
    suspend();
}

void PinsIns8060::idle() {
    // #ENIN is HIGH and bus cycle is suspened.
    xin_cycle_lo();
    delayNanoseconds(0);
}

void PinsIns8060::loop() {
    while (true) {
        _devs->loop();
        auto s = prepareCycle();
        if (s->fetch()) {
            const auto inst = _mems->read_byte(s->addr);
            const auto len = InstIns8060::instLen(inst);
            if (len == 0 || inst == InstIns8060::HALT) {
                completeCycle(s->inject(InstIns8060::JMP));
                inject(InstIns8060::JMP_HERE);
                negate_enin();
                Signals::discard(s);
                return;
            }
            s->inject(inst);
        }
        completeCycle(s);
        if (haltSwitch()) {
            suspend();
            return;
        }
    }
}

void PinsIns8060::suspend() const {
    while (true) {
        auto s = prepareCycle();
        if (s->fetch()) {
            completeCycle(s->inject(InstIns8060::JMP));
            inject(InstIns8060::JMP_HERE);
            negate_enin();
            Signals::discard(s);
            return;
        }
        completeCycle(s);
    }
}

void PinsIns8060::run() {
    _regs->restore();
    Signals::resetCycles();
    saveBreakInsts();
    assert_enin();
    loop();
    restoreBreakInsts();
    disassembleCycles();
    _regs->save();
}

bool PinsIns8060::rawStep() const {
    assert_enin();
    auto s = prepareCycle();
    const auto inst = _mems->read_byte(s->addr);
    const auto len = InstIns8060::instLen(inst);
    if (len == 0 || inst == InstIns8060::HALT) {
        // HALT or unknown instruction
        completeCycle(s->inject(InstIns8060::JMP));
        inject(InstIns8060::JMP_HERE);
        Signals::discard(s);
        return false;
    }
    completeCycle(s->inject(inst));
    suspend();
    return true;
}

bool PinsIns8060::step(bool show) {
    Signals::resetCycles();
    _regs->restore();
    if (show)
        Signals::resetCycles();
    if (rawStep()) {
        if (show)
            printCycles();
        _regs->save();
        return true;
    }
    return false;
}

void PinsIns8060::assertInt(uint8_t name) {
    assert_sense_a();
}

void PinsIns8060::negateInt(uint8_t name) {
    negate_sense_a();
}

void PinsIns8060::setBreakInst(uint32_t addr) const {
    _mems->put_inst(addr, InstIns8060::HALT);
}

void PinsIns8060::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

void PinsIns8060::disassembleCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles;) {
        const auto s = g->next(i);
        if (s->fetch()) {
            const auto len = _mems->disassemble(s->addr, 1) - s->addr;
            i += len;
        } else {
            s->print();
            ++i;
        }
        idle();
    }
}

}  // namespace ins8060
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
