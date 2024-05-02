#include "pins_ins8060.h"
#include "debugger.h"
#include "devs_ins8060.h"
#include "digital_bus.h"
#include "ins8060_sci_handler.h"
#include "inst_ins8060.h"
#include "mems_ins8060.h"
#include "regs_ins8060.h"
#include "signals_ins8060.h"

namespace debugger {
namespace ins8060 {

struct PinsIns8060 Pins;

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
//  fx: max 4.0 MHz           ; XIN frequencey
//  Tc: min 500 ns            ; 2 cycles of XIN
// TW0: min 120 ns            ; XIN low width
// TW1: min 120 ns            ; XIN high width
//  TH: min 100 ns, max 225 ns; XOUT trailing to #ADS trailing
// ADS: min TC/2-50 ns        ; #ADS width
// RDS  min TC+50 ns          ; #RDS width
// WDS: min TC-50 ns          ; #WDS width

constexpr auto xin_hi_ns = 105;       // 120
constexpr auto xin_lo_ns = 80;        // 120
constexpr auto xin_hi_enout = 84;     // 120
constexpr auto xin_lo_enout = 82;     // 120
constexpr auto xin_lo_ads = 80;       // 120
constexpr auto xin_hi_ads = 85;       // 120
constexpr auto xin_write_begin = 72;  // 120
constexpr auto xin_hi_wds = 82;       // 120
constexpr auto xin_lo_wds = 84;       // 120
constexpr auto xin_lo_write = 52;     // 120
constexpr auto xin_hi_capture = 94;   // 120
constexpr auto xin_write_end = 30;    // 120
constexpr auto xin_read_begin = 100;  // 120
constexpr auto xin_lo_inject = 50;    // 120
constexpr auto xin_read_out = 20;     // 120
constexpr auto xin_hi_rds = 100;      // 120
constexpr auto xin_lo_rds = 100;      // 120
constexpr auto xin_lo_read = 90;      // 120
constexpr auto xin_hi_read = 80;      // 120
constexpr auto xin_read_end = 30;     // 120

inline uint8_t signal_breq() {
    return digitalReadFast(PIN_BREQ);
}

inline uint8_t signal_enout() {
    return digitalReadFast(PIN_ENOUT);
}

inline uint8_t signal_ads() {
    return digitalReadFast(PIN_ADS);
}

inline uint8_t signal_wds() {
    return digitalReadFast(PIN_WDS);
}

inline uint8_t signal_rds() {
    return digitalReadFast(PIN_RDS);
}

void assert_sense_a() {
    digitalWriteFast(PIN_SENSEA, HIGH);
}

void negate_sense_a() {
    digitalWriteFast(PIN_SENSEA, LOW);
}

void assert_cont() {
    digitalWriteFast(PIN_CONT, HIGH);
}

void negate_hold() {
    digitalWriteFast(PIN_HOLD, HIGH);
}

void assert_enin() {
    digitalWriteFast(PIN_ENIN, LOW);
}

void negate_enin() {
    digitalWriteFast(PIN_ENIN, HIGH);
}

void assert_reset() {
    // Drive RESET condition
    digitalWriteFast(PIN_RST, LOW);
    negate_sense_a();
    assert_cont();
    negate_hold();
    negate_enin();
}

void negate_reset() {
    // Release RESET conditions
    digitalWriteFast(PIN_RST, HIGH);
}

const uint8_t PINS_LOW[] = {
        PIN_XIN,
        PIN_RST,
        PIN_SENSEA,
        PIN_SENSEB,
        PIN_SIN,
};

const uint8_t PINS_HIGH[] = {
        PIN_CONT,
        PIN_ENIN,
};

const uint8_t PINS_INPUT[] = {
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

const uint8_t PINS_PULLUP[] = {
        PIN_BREQ,
        PIN_HOLD,
};

inline void xin_hi() {
    digitalWriteFast(PIN_XIN, HIGH);
}

inline void xin_lo() {
    digitalWriteFast(PIN_XIN, LOW);
    SciH.loop();
}

inline void xin_cycle() {
    digitalWriteFast(PIN_XIN, HIGH);
    delayNanoseconds(xin_hi_ns);
    digitalWriteFast(PIN_XIN, LOW);
    delayNanoseconds(xin_lo_ns);
}

}  // namespace

void PinsIns8060::reset() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_PULLUP, sizeof(PINS_PULLUP), INPUT_PULLUP);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    assert_reset();
    // #RST must remain low for a minimum of 4 Tc.
    for (auto i = 0; i < 2 * 4; i++)
        xin_cycle();
    negate_enin();
    negate_reset();
    // The #BREQ output goes low, indicating the start of execution;
    // this occurs at a time whithin 13 Tc after #RST is set high.
    Regs.save();
}

Signals *PinsIns8060::prepareCycle() const {
    // XIN=L
    auto s = Signals::put();
    while (true) {
        xin_hi();
        delayNanoseconds(xin_hi_enout);
        if (signal_enout() != LOW)
            break;
        xin_lo();
        delayNanoseconds(xin_lo_enout);
    }
    // #ENOUT=H (#BREQ=LOW)
    while (signal_ads() != LOW) {
        xin_lo();
        delayNanoseconds(xin_lo_ads);
        if (signal_ads() == LOW)
            break;
        xin_hi();
        delayNanoseconds(xin_hi_ads);
    }
    s->getAddr();
    // XIN=L
    return s;
}

Signals *PinsIns8060::completeCycle(Signals *s) const {
    // XIN=H
    xin_hi();
    if (s->write()) {
        delayNanoseconds(xin_write_begin);
        while (signal_wds() != LOW) {
            xin_lo();
            delayNanoseconds(xin_lo_wds);
            xin_hi();
            delayNanoseconds(xin_hi_wds);
        }
        xin_lo();
        s->getData();
        delayNanoseconds(xin_lo_write);
        xin_hi();
        if (s->writeMemory()) {
            Memory.write(s->addr, s->data);
        } else {
            delayNanoseconds(xin_hi_capture);
        }
        xin_lo();
        Signals::nextCycle();
        delayNanoseconds(xin_write_end);
    } else {
        delayNanoseconds(xin_read_begin);
        xin_lo();
        if (s->readMemory()) {
            s->data = Memory.read(s->addr);
        } else {
            delayNanoseconds(xin_lo_inject);
        }
        while (signal_rds() != LOW) {
            xin_hi();
            delayNanoseconds(xin_hi_rds);
            xin_lo();
            delayNanoseconds(xin_lo_rds);
        }
        xin_hi();
        busMode(DB, OUTPUT);
        busWrite(DB, s->data);
        delayNanoseconds(xin_read_out);
        while (signal_rds() == LOW) {
            xin_lo();
            delayNanoseconds(xin_lo_read);
            xin_hi();
            delayNanoseconds(xin_hi_read);
        }
        xin_lo();
        Signals::nextCycle();
        delayNanoseconds(xin_read_end);
    }
    xin_hi();
    delayNanoseconds(xin_hi_enout);
    // Read-Modify-Write bus cycle keeps #BREQ low.
    for (auto i = 0; i < 3; ++i) {
        if (signal_enout() == LOW)
            break;
        xin_lo();
        delayNanoseconds(xin_lo_enout);
        xin_hi();
        delayNanoseconds(xin_hi_enout);
    }
    xin_lo();
    busMode(DB, INPUT);
    // XIN=L
    return s;
}

Signals *PinsIns8060::cycle(uint8_t data) const {
    return completeCycle(prepareCycle()->inject(data));
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
    xin_cycle();
}

void PinsIns8060::loop() {
    while (true) {
        Devs.loop();
        auto s = prepareCycle();
        if (s->fetch()) {
            const auto inst = Memory.raw_read(s->addr);
            const auto len = InstIns8060::instLen(inst);
            if (len == 0 || inst == InstIns8060::HALT) {
                completeCycle(s->inject(InstIns8060::JMP));
                cycle(InstIns8060::JMP_HERE);
                negate_enin();
                Signals::discard(s);
                return;
            }
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
            cycle(InstIns8060::JMP_HERE);
            negate_enin();
            Signals::discard(s);
            return;
        }
        completeCycle(s);
    }
}

void PinsIns8060::run() {
    Regs.restore();
    Signals::resetCycles();
    saveBreakInsts();
    assert_enin();
    loop();
    restoreBreakInsts();
    disassembleCycles();
    Regs.save();
}

bool PinsIns8060::rawStep() const {
    assert_enin();
    auto s = prepareCycle();
    const auto inst = Memory.raw_read(s->addr);
    const auto len = InstIns8060::instLen(inst);
    if (len == 0 || inst == InstIns8060::HALT) {
        // HALT or unknown instruction
        completeCycle(s->inject(InstIns8060::JMP));
        cycle(InstIns8060::JMP_HERE);
        Signals::discard(s);
        return false;
    }
    completeCycle(s);
    suspend();
    return true;
}

bool PinsIns8060::step(bool show) {
    Signals::resetCycles();
    Regs.restore();
    if (show)
        Signals::resetCycles();
    if (rawStep()) {
        if (show)
            printCycles();
        Regs.save();
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
    Memory.put_inst(addr, InstIns8060::HALT);
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
            const auto len = Memory.disassemble(s->addr, 1) - s->addr;
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
