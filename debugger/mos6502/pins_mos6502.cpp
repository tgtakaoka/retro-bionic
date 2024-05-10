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
//  TL0: min 480 ns; PHI0 low
//  TH0: min 460 ns; PHI0 high
// T02+: max  65 ns; PHI0- to PHI2-
// T02+: max  75 ns; PHI0+ to PHI2+
// Tads: max 225 ns; PHI2- to addr
// Trsw: max 225 ns; PHI2- to R/#W
// Tsys: max 350 ns; PHI2- to SYNC+
// Tmds: max 175 ns; PHI2+ to write data
// Tdsu: min 100 ns; read data to PHI2-

constexpr auto phi0_lo_sync = 30;       // 500
constexpr auto phi0_lo_ns = 218;        // 500
constexpr auto phi0_lo_fetch = 90;      // 500
constexpr auto phi0_lo_loop = 30;       // 500
constexpr auto phi0_lo_execute = 30;    // 500
constexpr auto phi0_hi_read_pre = 280;  // 500
constexpr auto phi0_hi_read_post = 30;  // 500
constexpr auto phi0_hi_write = 334;     // 500
constexpr auto phi0_hi_inject = 70;
constexpr auto phi0_hi_capture = 60;

inline void phi0_hi() __attribute__((always_inline));
inline void phi0_hi() {
    digitalWriteFast(PIN_PHI0, HIGH);
}

inline void phi0_lo() __attribute__((always_inline));
inline void phi0_lo() {
    digitalWriteFast(PIN_PHI0, LOW);
}

auto signal_e() {
    return digitalReadFast(PIN_E);
}

auto signal_phi1o() {
    return digitalReadFast(PIN_PHI1O);
}

auto signal_vp() {
    return digitalReadFast(PIN_VP);
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
    assert_rdy();
    digitalWriteFast(PIN_RES, LOW);
    negate_abort();
    negate_nmi();
    negate_irq();
}

void negate_reset() {
    // Release RESET conditions
    digitalWriteFast(PIN_RES, HIGH);
}

const uint8_t PINS_LOW[] = {
        PIN_PHI0,
        PIN_RES,
};

const uint8_t PINS_HIGH[] = {
        PIN_NMI,
        PIN_IRQ,
};

const uint8_t PINS_PULLUP[] = {
        // RDY(input) for 6502, pullup
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
        // NC on 6502
        // E(output) from 65816
        PIN_E,
        // VSS for 6502
        // #VP(output) from W65Cxx
        PIN_VP,
        // NC on 6502
        // BE(input) for W65Cxx
        PIN_BE,
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
};

}  // namespace

bool PinsMos6502::native65816() const {
    return _hardType == HW_W65C816 && signal_e() == LOW;
}

void PinsMos6502::checkHardwareType() {
    phi0_lo();
    delayNanoseconds(500);
    phi0_hi();
    delayNanoseconds(500);
    if (signal_phi1o() == LOW) {
        // PIN_PHI1O is iverted PIN_PHI0, means not W65C816_ABORT.
        if (signal_vp() == LOW) {
            // PIN_VP keeps LOW, means Vss of MOS6502, G65SC02, R65C02.
            _hardType = HW_MOS6502;
        } else {
            // PIN_VP keeps HIGH, means #VP of W65C02S.
            _hardType = HW_W65C02S;
            // Enable BE
            digitalWriteFast(PIN_BE, HIGH);
            pinMode(PIN_BE, OUTPUT);
        }
        // Keep PIN_SO HIGH using pullup
        Target6502.setMems(mos6502::Memory);
    } else {
        // PIN_PHI1O/W65C816_ABORT keeps HIGH, means W65C816S.
        _hardType = HW_W65C816;
        // Negate #ABORT
        digitalWriteFast(W65C816_ABORT, HIGH);
        pinMode(W65C816_ABORT, OUTPUT);
        // Enable BE
        digitalWriteFast(PIN_BE, HIGH);
        pinMode(PIN_BE, OUTPUT);
    }
}

void PinsMos6502::checkSoftwareType() {
    if (_hardType == HW_W65C816) {
        _softType = SW_W65C816;
        return;
    }

    cycle(InstMos6502::JMP);
    cycle(0x00);
    cycle(0x10);
    auto base = cycle(InstMos6502::BRA);  // 4 clock branch
    cycle(InstMos6502::NOP);
    cycle(InstMos6502::NOP);
    cycle(InstMos6502::NOP);
    auto det65 = cycle(InstMos6502::JMP);
    cycle(0x00);
    cycle(0x10);
    if (det65->addr == base->addr + 3U) {
        _softType = SW_MOS6502;
        return;
    }
    base = cycle(InstMos6502::BBR0);  // 6 clock branch
    cycle(InstMos6502::NOP);
    cycle(InstMos6502::NOP);
    cycle(InstMos6502::NOP);
    cycle(InstMos6502::NOP);
    cycle(InstMos6502::NOP);
    cycle(InstMos6502::NOP);
    det65 = cycle(InstMos6502::NOP);
    cycle(InstMos6502::NOP);
    if (det65->addr == base->addr + 4) {
        _softType = SW_G65SC02;
    } else {
        _softType = _hardType == HW_MOS6502 ? SW_R65C02 : SW_W65C02S;
    }
}

void PinsMos6502::reset() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_PULLUP, sizeof(PINS_PULLUP), INPUT_PULLUP);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    assert_reset();
    checkHardwareType();
    // #RES must be held low for at lease two clock cycles.
    for (auto i = 0; i < 10; i++)
        cycle();
    auto s = prepareCycle();
    negate_reset();
    completeCycle(s);
    Signals::resetCycles();
    // When a positive edge is detected, there is an initalization
    // sequence lasting seven clock cycles.
    for (auto i = 0; i < 10; i++) {
        // there may be suprious write
        const auto s = completeCycle(prepareCycle()->capture());
        // Read Reset vector
        if (s->vector() && s->addr == InstMos6502::VECTOR_RESET) {
            cycle();
            break;
        }
    }

    Registers.save();
    assert_rdy();
    checkSoftwareType();
    negate_rdy();
}

Signals *PinsMos6502::rawPrepareCycle() {
    auto s = Signals::put();
    if (_hardType == HW_W65C816 && s->addr24()) {
        s->getAddr();
        s->addr |= busRead(BA);
    } else {
        delayNanoseconds(phi0_lo_sync);
        s->getAddr();
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
            Target6502.memory().write(s->addr, s->data);
        } else {
            delayNanoseconds(phi0_hi_capture);
        }
        phi0_lo();
    } else {
        if (s->readMemory()) {
            s->data = Target6502.memory().read(s->addr);
        } else {
            delayNanoseconds(phi0_hi_inject);
        }
        // [W65C816] Delay to avoid bus conflict with bank address of
        // this bus cycle.
        delayNanoseconds(phi0_hi_read_pre);
        // [W65C816] This order, mode change then write data, somehow
        // mitigate a jitter from |phi0_lo| to |busMode(D, INPUT)|.
        busMode(D, OUTPUT);
        busWrite(D, s->data);
        delayNanoseconds(phi0_hi_read_post);
        phi0_lo();
        // [W65C816] Immediately switch bus direction to avoid bus
        // conflict with bank address of next bus cycle.
        busMode(D, INPUT);
    }
    Signals::nextCycle();

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
        delayNanoseconds(phi0_lo_execute);
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
    delayNanoseconds(370);
    phi0_hi();
    delayNanoseconds(474);
    phi0_lo();
}

void PinsMos6502::loop() {
    while (true) {
        Devices.loop();
        delayNanoseconds(phi0_lo_fetch);
        auto s = rawPrepareCycle();
        if (s->fetch()) {
            const auto inst = Target6502.memory().raw_read(s->addr);
            if (inst == InstMos6502::BRK) {
                const auto opr = Target6502.memory().raw_read(s->addr + 1);
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
    const auto inst = Target6502.memory().raw_read(s->addr);
    if (InstMos6502::isStop(inst))
        return false;
    if (inst == InstMos6502::BRK) {
        const auto opr = Target6502.memory().raw_read(s->addr + 1);
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
    Signals::resetCycles();
    Registers.restore();
    if (show)
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
    Target6502.memory().put_inst(addr, InstMos6502::BRK);
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
                    Target6502.memory().disassemble(s->addr, 1) - s->addr;
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
