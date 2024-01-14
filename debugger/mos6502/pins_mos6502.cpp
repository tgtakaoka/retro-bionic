#include "pins_mos6502.h"
#include "debugger.h"
#include "devs_mos6502.h"
#include "digital_bus.h"
#include "inst_mos6502.h"
#include "mems_mos6502.h"
#include "mems_w65c816.h"
#include "regs_mos6502.h"
#include "signals_mos6502.h"
#include "target_mos6502.h"

namespace debugger {
namespace mos6502 {

struct PinsMos6502 Pins;

/**
 * W65C02S bus cycle.
 *         __________            __________            ______
 * PHI2  _|          |__________|          |__________|
 *       __            __________            __________
 * PHI1O   |__________|          |__________|          |_____
 *           __________            __________            ____
 * PHI2O ___|          |__________|          |__________|
 *       ________  ____________________  ____________________
 *  SYNC ________><____________________><____________________
 *                ______________________
 *   R/W --------<                      |____________________
 *                           ____               _________
 *  Data -------------------<rrrr>-------------<wwwwwwwww>---
 */

namespace {

constexpr auto phi0_lo_sync = 30;    // 500
constexpr auto phi0_lo_ns = 186;     // 500
constexpr auto phi0_lo_fetch = 118;  // 500
constexpr auto phi0_lo_loop = 30;    // 500
constexpr auto phi0_hi_read = 308;   // 500
constexpr auto phi0_hi_write = 304;  // 500
constexpr auto phi0_hi_inject = 50;
constexpr auto phi0_hi_capture = 60;

inline void phi0_hi() __attribute__((always_inline));
inline void phi0_hi() {
    digitalWriteFast(PIN_PHI0, HIGH);
}

inline void phi0_lo() __attribute__((always_inline));
inline void phi0_lo() {
    digitalWriteFast(PIN_PHI0, LOW);
}

void assert_abort() __attribute__((unused));
void assert_abort() {
    digitalWriteFast(W65C816_ABORT, LOW);
}

void negate_abort() {
    digitalWriteFast(W65C816_ABORT, HIGH);
}

void negate_nmi() {
    digitalWriteFast(PIN_NMI, HIGH);
}

void assert_irq() {
    digitalWriteFast(PIN_IRQ, LOW);
}

void negate_irq() {
    digitalWriteFast(PIN_IRQ, HIGH);
}

void assert_be() __attribute__((unused));
void assert_be() {
    digitalWriteFast(PIN_BE, HIGH);
}

void negate_be() __attribute__((unused));
void negate_be() {
    digitalWriteFast(PIN_BE, LOW);
}

void assert_rdy() {
    digitalWriteFast(PIN_RDY, HIGH);
    pinMode(PIN_RDY, INPUT_PULLUP);
}

void negate_rdy() {
    digitalWriteFast(PIN_RDY, LOW);
    pinMode(PIN_RDY, OUTPUT_OPENDRAIN);
}

void assert_reset() {
    // Drive RESET condition
    phi0_lo();
    negate_be();
    digitalWriteFast(PIN_RES, LOW);
    negate_abort();
    negate_nmi();
    negate_irq();
    assert_rdy();
}

void negate_reset() {
    // Release RESET conditions
    digitalWriteFast(PIN_RES, HIGH);
}

const uint8_t PINS_LOW[] = {
        PIN_PHI0,
        PIN_RES,
        // NC on 6502
        // BE(input) for W65Cxx
        PIN_BE,
};

const uint8_t PINS_HIGH[] = {
        PIN_NMI,
        PIN_IRQ,
};

const uint8_t PINS_PULLUP[] = {
        // RDY(input) for 6502
        // RDY(bi-directional) for W65Cxx, pullup
        PIN_RDY,
        // PHI1O(output) from 6502
        // #ABORT(input) for 65816, pullup
        PIN_PHI1O,
        // NC for 6502
        // #ML(output) from W65Cxx
        PIN_ML,
        // #SO(input) for 6502, pullup
        // #MX(output) from 65816
        PIN_SO,
};

const uint8_t PINS_PULLDOWN[] = {
        // NC on 6502
        // E(output) from 65816
        PIN_E,
};

const uint8_t PINS_INPUT[] = {
        PIN_D0,
        PIN_D1,
        PIN_D2,
        PIN_D3,
        PIN_D4,
        PIN_D5,
        PIN_D6,
        PIN_D7,
        PIN_AL0,
        PIN_AL1,
        PIN_AL2,
        PIN_AL3,
        PIN_AL4,
        PIN_AL5,
        PIN_AL6,
        PIN_AL7,
        PIN_AM8,
        PIN_AM9,
        PIN_AM10,
        PIN_AM11,
        PIN_AH12,
        PIN_AH13,
        PIN_AH14,
        PIN_AH15,
        PIN_RW,
        // SYNC(output) from 6502
        // VPA(output) from 65816
        PIN_SYNC,
        // PHI2O(output) from 6502
        // VDA(output) from 65816
        PIN_PHI2O,
        // VSS for 6502
        // #VP(output) from W65Cxx
        PIN_VP,
};

}  // namespace

bool PinsMos6502::native65816() const {
    return _type == HW_W65C816 && digitalReadFast(PIN_E) == LOW;
}

void PinsMos6502::checkHardwareType() {
    digitalWriteFast(PIN_PHI0, LOW);
    delayNanoseconds(500);
    digitalWriteFast(PIN_PHI0, HIGH);
    delayNanoseconds(500);
    if (digitalReadFast(PIN_PHI1O) == LOW) {
        // PIN_PHI1O is iverted PIN_PHI0, means not W65C816_ABORT.
        if (digitalReadFast(PIN_VP) == LOW) {
            // PIN_VP keeps LOW, means Vss of MOS6502, G65SC02, R65C02.
            _type = HW_MOS6502;
        } else {
            // PIN_VP keeps HIGH, means #VP of W65C02S.
            _type = HW_W65C02S;
            // Enable BE.
            digitalWriteFast(PIN_BE, HIGH);
        }
        // Keep PIN_SO HIGH using pullup
        Target6502.setMems(mos6502::Memory);
    } else {
        // W65C816_ABORT keeps HIGH, means W65C816S.
        _type = HW_W65C816;
        // Negate #ABORT by using pullup.
        // Enable BE.
        digitalWriteFast(PIN_BE, HIGH);
        Target6502.setMems(w65c816::Memory);
    }
}

void PinsMos6502::reset() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_PULLUP, sizeof(PINS_PULLUP), INPUT_PULLUP);
    pinsMode(PINS_PULLDOWN, sizeof(PINS_PULLDOWN), INPUT_PULLDOWN);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    assert_reset();
    checkHardwareType();
    // #RES must be held low for at lease two clock cycles.
    for (auto i = 0; i < 10; i++) {
        // there may be suprious write
        completeCycle(prepareCycle()->capture());
    }
    Signals::resetCycles();
    cycle();
    negate_reset();
    // When a positive edge is detected, there is an initalization
    // sequence lasting seven clock cycles.
    for (auto i = 0; i < 7; i++) {
        // there may be suprious write
        auto s = prepareCycle()->capture();
        if (s->vector() && s->addr == InstMos6502::VECTOR_RESET)
            break;
        completeCycle(s);
    }
    // Read Reset vector
    cycle();
    cycle();
    // printCycles() calls idle() and inject clocks.
    negate_rdy();
    // The first instruction will be saving registers, and certainly can be
    // injected.
    Registers.reset();
    Registers.save();
}

Signals *PinsMos6502::rawPrepareCycle() {
    auto s = Signals::put();
    s->getAddr();
    if (hardwareType() == HW_W65C816) {
        s->addr |= static_cast<uint32_t>(busRead(D)) << 16;
        const auto vpa = digitalReadFast(W65C816_VPA);
        const auto vda = digitalReadFast(W65C816_VDA);
        s->fetch() = (vpa != LOW) && (vda != LOW);
    } else {
        delayNanoseconds(phi0_lo_sync);
        s->fetch() = digitalReadFast(PIN_SYNC) != LOW;
    }
    return s;
}

Signals *PinsMos6502::prepareCycle() {
    delayNanoseconds(phi0_lo_ns);
    return rawPrepareCycle();
}

Signals *PinsMos6502::completeCycle(Signals *s) {
    phi0_hi();
    if (s->write()) {
        delayNanoseconds(phi0_hi_write);
        s->getData();
        if (s->writeMemory()) {
            Target6502.mems().write(s->addr, s->data);
        } else {
            delayNanoseconds(phi0_hi_capture);
        }
        Signals::nextCycle();
    } else {
        if (s->readMemory()) {
            s->data = Target6502.mems().read(s->addr);
        } else {
            delayNanoseconds(phi0_hi_inject);
        }
        busMode(D, OUTPUT);
        busWrite(D, s->data);
        Signals::nextCycle();
        delayNanoseconds(phi0_hi_read);
    }
    phi0_lo();
    busMode(D, INPUT);

    return s;
}

Signals *PinsMos6502::cycle() {
    return completeCycle(prepareCycle());
}

Signals *PinsMos6502::cycle(uint8_t data) {
    return completeCycle(prepareCycle()->inject(data));
}

void PinsMos6502::execInst(const uint8_t *inst, uint8_t len) {
    execute(inst, len, nullptr, nullptr, 0);
}

void PinsMos6502::captureWrites(const uint8_t *inst, uint8_t len,
        uint16_t *addr, uint8_t *buf, uint8_t max) {
    execute(inst, len, addr, buf, max);
}

void PinsMos6502::execute(const uint8_t *inst, uint8_t len, uint16_t *addr,
        uint8_t *buf, uint8_t max) {
    assert_rdy();
    uint8_t inj = 0;
    uint8_t cap = 0;
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
        } else {
            if (cap == 0 && addr)
                *addr = s->addr;
            if (cap < max && buf)
                buf[cap++] = s->data;
        }
    }
    negate_rdy();
}

void PinsMos6502::idle() {
    // CPU is stopping with RDY=L
    delayNanoseconds(404);
    phi0_hi();
    delayNanoseconds(446);
    phi0_lo();
}

void PinsMos6502::loop() {
    while (true) {
        Devices.loop();
        delayNanoseconds(phi0_lo_fetch);
        auto s = rawPrepareCycle();
        if (s->fetch()) {
            const auto inst = Target6502.mems().raw_read(s->addr);
            if (inst == InstMos6502::BRK) {
                const auto opr = Target6502.mems().raw_read(s->addr + 1);
                if (opr == 0 || isBreakPoint(s->addr)) {
                    suspend();
                    return;
                }
            }
        } else {
            delayNanoseconds(phi0_lo_loop);
        }
        completeCycle(s);
        if (haltSwitch()) {
            suspend();
            return;
        }
    }
}

void PinsMos6502::run() {
    Registers.restore();
    Signals::resetCycles();
    saveBreakInsts();
    assert_rdy();
    loop();
    negate_rdy();
    restoreBreakInsts();
    disassembleCycles();
    Registers.save();
}

void PinsMos6502::suspend() {
    while (true) {
        auto s = prepareCycle();
        if (s->fetch())
            return;
        completeCycle(s);
    }
}

bool PinsMos6502::rawStep() {
    auto s = rawPrepareCycle();
    const auto inst = Target6502.mems().raw_read(s->addr);
    if (InstMos6502::isStop(inst))
        return false;
    if (inst == InstMos6502::BRK) {
        const auto opr = Target6502.mems().raw_read(s->addr + 1);
        if (opr == 0 || isBreakPoint(s->addr))
            return false;
    }
    assert_rdy();
    completeCycle(s);
    while (true) {
        auto s = prepareCycle();
        if (s->fetch())
            break;
        completeCycle(s);
    }
    negate_rdy();
    return true;
}

bool PinsMos6502::step(bool show) {
    Registers.restore();
    Signals::resetCycles();
    if (rawStep()) {
        if (show)
            printCycles();
        Registers.save();
        return true;
    }
    return false;
}

void PinsMos6502::assertInt(uint8_t name) {
    assert_irq();
}

void PinsMos6502::negateInt(uint8_t name) {
    negate_irq();
}

void PinsMos6502::setBreakInst(uint32_t addr) const {
    Target6502.mems().put_inst(addr, InstMos6502::BRK);
}

void PinsMos6502::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles; i++) {
        g->next(i)->print();
        idle();
    }
}

void PinsMos6502::disassembleCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles;) {
        const auto s = g->next(i);
        if (s->fetch()) {
            const auto len =
                    Target6502.mems().disassemble(s->addr, 1) - s->addr;
            i += len;
        } else {
            s->print();
            ++i;
        }
        idle();
    }
}

}  // namespace mos6502
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
