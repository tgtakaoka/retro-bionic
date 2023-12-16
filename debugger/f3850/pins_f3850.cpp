#include "pins_f3850.h"
#include "debugger.h"
#include "devs_f3850.h"
#include "digital_bus.h"
#include "i8251.h"
#include "inst_f3850.h"
#include "mems_f3850.h"
#include "regs_f3850.h"
#include "signals_f3850.h"

namespace debugger {
namespace f3850 {

struct PinsF3850 Pins;

// clang-format off
/**
 * F3850 bus cycle
 *        _    __    __    __    __    __    __    __    __    __    __    __    __
 *   XTLY  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \
 *        _|    __    __    __    __    __    __    __    __    __    __    __    __
 *    PHI   \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/
 *           \_____\                 \_____\                             \_____\
 *  WRITE ___/      \________________/      \____________________________/      \___
 *        _______________ _______________________ __________________________________
 *   ROMC _______________X_______________________X__________________________________
 *        _______________________ _______
 *  DB RD _______________________X_______
 *        _________ _______________________ ___________________________________ ____
 *     DB _________X_|_____________________X_|_________________________________X_|__
 */
// clang-format on

namespace {
//  tx1: max 250 ns, typ 160 ns; XTLY- to PHI- delay
//  tx2: max 250 ns, typ 150 ns; XTLY+ to PHI+ delay
//  PWx: min 200 ns; XTLY pulse width
//   Px: min 500 ns; XTLY period (2MHz)
//  PW2: min PHI-100 ns; WRITE pulse width
//  td1: max 250 ns; PHI- to WRITE+ delay
//  td2: max 250 ns; PHI- to WRITE- delay
//  td3: max 550 ns; WRITE to ROMC delay
// tdb3: min 200 ns; DATA bus setup to WRITE- IR
// tdb4: min 500 ns; DATA bus setup to WRITE- AM
// tdb5: min 500 ns; DATA bus setup to WRITE- LR K,P etc.
// tdb6: min 500 ns; DATA bus setup to WRITE- LM

constexpr auto xtly_lo_ns = 224;         // 250
constexpr auto xtly_hi_ns = 210;         // 250
constexpr auto xtly_hi_input = 170;      // 250
constexpr auto xtly_read_romc = 170;     // 250
constexpr auto xtly_hi_read = 170;       // 250
constexpr auto xtly_hi_noread = 210;     // 250
constexpr auto xtly_lo_next = 200;       // 250
constexpr auto xtly_nowrite_romc = 144;  // 250
constexpr auto xtly_hi_write = 110;      // 250
constexpr auto xtly_idle_ns = 25;        // 250

inline void xtly_hi() {
    digitalWriteFast(PIN_XTLY, HIGH);
}

inline void xtly_lo() {
    digitalWriteFast(PIN_XTLY, LOW);
}

inline uint8_t signal_phi() {
    return digitalReadFast(PIN_PHI);
}

inline uint8_t signal_write() {
    return digitalReadFast(PIN_WRITE);
}

void assert_intreq() {
    digitalWriteFast(PIN_INTREQ, LOW);
}

void negate_intreq() {
    digitalWriteFast(PIN_INTREQ, HIGH);
}

void assert_extres() {
    negate_intreq();
    digitalWriteFast(PIN_EXTRES, LOW);
}

void negate_extres() {
    digitalWriteFast(PIN_EXTRES, HIGH);
}

const uint8_t PINS_LOW[] = {
        PIN_EXTRES,
        PIN_XTLY,
};

const uint8_t PINS_HIGH[] = {
        PIN_INTREQ,
        PIN_IO1L0,
        PIN_IO1L1,
        PIN_IO1L2,
        PIN_IO1L3,
        PIN_IO1H4,
        PIN_IO1H5,
        PIN_IO1H6,
        PIN_IO1H7,
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
        PIN_ROMC0,
        PIN_ROMC1,
        PIN_ROMC2,
        PIN_ROMC3,
        PIN_ROMC4,
        PIN_WRITE,
        PIN_PHI,
        PIN_ICB,
        PIN_IO00,
        PIN_IO01,
        PIN_IO02,
        PIN_IO03,
        PIN_IO04,
        PIN_IO05,
        PIN_IO06,
        PIN_IO07,
};

inline void xtly_cycle() {
    delayNanoseconds(xtly_hi_ns);
    xtly_lo();
    delayNanoseconds(xtly_lo_ns);
    xtly_hi();
}

inline void xtly_cycle_hi() {
    xtly_lo();
    delayNanoseconds(xtly_lo_ns);
    xtly_hi();
}

}  // namespace

void PinsF3850::reset() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    Signals::resetCycles();
    assert_extres();
    for (auto i = 0; i < 10 * 4; ++i)
        xtly_cycle();
    while (signal_write() == LOW)
        xtly_cycle();
    // WRITE=H
    negate_extres();
    delayNanoseconds(xtly_hi_ns);

    cycle();  // IDLE
    delayNanoseconds(xtly_idle_ns);
    cycle();  // RESET PC0

    Regs.save();
}

Signals *PinsF3850::cycle() {
    // XTLY=H
    auto signals = Signals::put();
    xtly_cycle_hi();
    while (signal_write() != LOW)
        xtly_cycle();
    // WRITE=L
    busMode(DB, INPUT);
    delayNanoseconds(xtly_hi_input);
    xtly_cycle_hi();
    delayNanoseconds(xtly_read_romc);
    const auto read = Regs.romc_read(signals->getRomc());
    xtly_cycle_hi();
    if (read) {
        delayNanoseconds(xtly_hi_read);
        signals->outData();
    } else {
        delayNanoseconds(xtly_hi_noread);
    }
    xtly_lo();
    Signals::nextCycle();
    delayNanoseconds(xtly_lo_next);
    xtly_hi();
    while (signal_write() == LOW)
        xtly_cycle();
    // WRITE=H
    if (read) {
        delayNanoseconds(xtly_nowrite_romc);
    } else {
        if (Regs.romc_write(signals->getData()))
            delayNanoseconds(xtly_hi_write);
    }
    // XTLY=H
    return signals;
}

Signals *PinsF3850::cycle(uint8_t data) {
    Signals::put()->inject(data);
    return cycle();
}

void PinsF3850::execInst(const uint8_t *inst, uint8_t len) {
    execute(inst, len, nullptr, 0);
}

uint8_t PinsF3850::captureWrites(
        const uint8_t *inst, uint8_t len, uint8_t *buf, uint8_t max) {
    return execute(inst, len, buf, max);
}

uint8_t PinsF3850::execute(
        const uint8_t *inst, uint8_t len, uint8_t *buf, uint8_t max) {
    uint8_t inj = 0;
    uint8_t cap = 0;
    while (inj < len || cap < max) {
        auto signals = Signals::put();
        if (cap < max)
            signals->capture();
        if (inj < len)
            signals->inject(inst[inj]);
        cycle();
        if (signals->read()) {
            if (inj < len)
                inj++;
        } else if (signals->write()) {
            if (buf && cap < max)
                buf[cap++] = signals->data;
        }
    }
    return cap;
}

void PinsF3850::idle() {
    // Inject "BR $"
    const auto &s = cycle(InstF3850::BR);  // ROMC=0x00
    delayNanoseconds(xtly_idle_ns);
    cycle(InstF3850::BR_HERE);  // ROMC=0x1C
    delayNanoseconds(xtly_idle_ns);
    cycle(0xFF);  // ROMC=0x01
    Signals::discard(s);
}

void PinsF3850::loop() {
    while (true) {
        Devs.loop();
        if (!rawStep() || haltSwitch())
            return;
    }
}

void PinsF3850::run() {
    Regs.restore();
    Signals::resetCycles();
    saveBreakInsts();
    loop();
    restoreBreakInsts();
    disassembleCycles();
    Regs.save();
}

bool PinsF3850::rawStep() {
    // Program counter is not in CPU but in Regs.
    const auto opc = Memory.read(Regs.nextIp());
    if (InstF3850::instLength(opc) == 0) {
        // Unknown instruction. Just return.
        return false;
    }
    auto inst = cycle();
    while (!inst->fetch()) {
        // may be interrupt acknowledging.
        inst = cycle();
    }
    const auto len = InstF3850::instLength(inst->data);
    const auto busCycles = InstF3850::busCycles(inst->data) + len;
    for (auto i = 1; i < busCycles; ++i)
        cycle();
    return true;
}

bool PinsF3850::step(bool show) {
    Regs.restore();
    Signals::resetCycles();
    if (rawStep()) {
        if (show)
            printCycles();
        Regs.save();
        return true;
    }
    return false;
}

void PinsF3850::assertInt(uint8_t name) {
    (void)name;
    assert_intreq();
}

void PinsF3850::negateInt(uint8_t name) {
    (void)name;
    negate_intreq();
}

void PinsF3850::setBreakInst(uint32_t addr) const {
    Memory.put_inst(addr, InstF3850::BREAK);
}

void PinsF3850::printCycles() const {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles; ++i) {
        g->next(i)->print();
    }
}

void PinsF3850::disassembleCycles() const {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles;) {
        const auto s = g->next(i);
        const auto len = InstF3850::instLength(s->data);
        if (s->fetch() && len) {
            Memory.disassemble(s->addr, 1);
            const auto cycles = InstF3850::busCycles(s->data);
            for (auto j = 0; j < len; ++j)
                s->next(j)->print();
            for (auto j = 0; j < cycles; ++j)
                s->next(len + j)->print();
            i += len + cycles;
        } else {
            s->print();
            ++i;
        }
    }
}

}  // namespace f3850
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
