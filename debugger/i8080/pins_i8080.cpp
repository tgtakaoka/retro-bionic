#include "pins_i8080.h"
#include "debugger.h"
#include "devs_i8080.h"
#include "inst_i8080.h"
#include "mems_i8080.h"
#include "regs_i8080.h"
#include "signals_i8080.h"

// #define DEBUG(e) e
#define DEBUG(e)

namespace debugger {
namespace i8080 {

// clang-format off
/**
 * P8080 bus cycle.
 *       --|---T1---|---T2---|---T3---|---T4---|---T5---|---T1---|
 *          _        _        _        _        _        _
 * phi 1 __/ \______/ \______/ \______/ \______/ \______/ \______
 *             ___      ___      ___      ___      ___      ___
 * phi 2 _____/   \____/   \____/   \____/   \____/   \____/   \_
 *             ___v____
 *  SYNC _____/        \_________________________________________
 *                       _______
 *  DBIN _______________/       \________________________________
 *       ____________________          __________________________
 *   #WR                     \________/
 *       _________v______________________________________________
 * READY ________/ \_____________________________________________
 */
// clang-format on

namespace {
// [i8224]
//  tCYC: min 500 ns; PHI1/PHI2 cycle time
// tPHI1: min  89 ns; PHI1 high width 2/9 tCYC-23ns
//   tD1: min   0 ns; PHI1- to PHI2+ delay
// tPHI2: min 236 ns; PHI2 high width 5/9 tCYC-35ns
//   tD2: min  95 ns; PHI2- to PHI1+ delay
//   tD3: min 109 ns; PHI1+ to PHI2+ delay
// [i8080]
//   tDC: max 150 ns; PHI2+ to SYNC+ and SYNC-
//   tDF: max 140 ns; PHI2+ to DBIN+ and DBIN-
//   tDC: max 150 ns; PHI1+ to #WR- and #WR+
//   tDA: max 200 ns; PHI2+ to address valid
//   tDD: max 220 ns; PHI2+ to status valid
//  tDS1: min  30 ns; data in setup to PHI1+
//  tDS2: min 150 ns; data in setup to PHI2+
//   tDH: min  50 ns; data in hold from PHI2+
//   tDD: max 220 ns; PHI2+ to data out
//   tRS: min 120 ns; READY setup to PHI2-
//    tH: min   0 ns; READY hold from PHI2-
constexpr auto phi1_hi_ns = 70;         // 95
constexpr auto phi1_lo_ns = 4;          // 40
constexpr auto phi2_hi_ns = 240;        // 270
constexpr auto phi2_lo_ns = 60;         // 95
constexpr auto phi2_hi_sync = 60;       // 70 (tDD-tDC)
constexpr auto phi2_hi_prep = 130;      // 200 (270-t2_hi_sync)
constexpr auto phi2_hi_addr = 10;       // 200 (270-t2_hi_sync)
constexpr auto phi2_hi_memread = 115;   // 270
constexpr auto phi2_hi_inject = 180;    // 270
constexpr auto phi2_hi_ioread = 20;     // 270
constexpr auto phi2_hi_intack = 75;     // 270
constexpr auto phi2_hi_input = 180;     // 270
constexpr auto phi2_hi_next = 190;      // 270
constexpr auto phi2_hi_memwrite = 115;  // 270
constexpr auto phi2_hi_capture = 235;   // 270
constexpr auto ready_setup_ns = 120;    // 120

const uint8_t PINS_LOW[] = {
        PIN_PHI1,
        PIN_PHI2,
        PIN_INT,
        PIN_HOLD,
        PIN_READY,
};

const uint8_t PINS_HIGH[] = {
        PIN_RESET,
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
        PIN_DBIN,
        PIN_WR,
        PIN_SYNC,
        PIN_HLDA,
        PIN_INTE,
        PIN_WAIT,
};

void phi1_hi() {
    digitalWriteFast(PIN_PHI1, HIGH);
}

void phi1_lo() {
    digitalWriteFast(PIN_PHI1, LOW);
}

void phi2_hi() {
    digitalWriteFast(PIN_PHI2, HIGH);
}

void phi2_lo() {
    digitalWriteFast(PIN_PHI2, LOW);
}

void phi1_pulse() {
    phi1_hi();
    delayNanoseconds(phi1_hi_ns);
    phi1_lo();
}

void phi2_pulse() {
    phi2_hi();
    delayNanoseconds(phi2_hi_ns);
    phi2_lo();
}

void phi_pulses() {
    phi1_pulse();
    delayNanoseconds(phi1_lo_ns);
    phi2_pulse();
}

auto signal_sync() {
    return digitalReadFast(PIN_SYNC);
}

void assert_ready() {
    digitalWriteFast(PIN_READY, HIGH);
}

void assert_reset() {
    // An external RESET signal of three clock period duration
    // (minimum) restores the processor's internal program counter to
    // zero.
    digitalWriteFast(PIN_RESET, HIGH);
    for (auto i = 0; i < 10; i++) {
        phi_pulses();
        delayNanoseconds(phi2_lo_ns);
    }
}

void negate_reset() {
    digitalWriteFast(PIN_RESET, LOW);
}

}  // namespace

PinsI8080::PinsI8080() {
    _regs = new RegsI8080(this);
    _mems = new MemsI8080();
    _devs = new DevsI8080();
}

void PinsI8080::resetPins() {
    // Assert reset condition
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    Signals::resetCycles();

    assert_reset();
    negate_reset();
    assert_ready();
    prepareCycle();
    enterWait();
    _regs->save();
}

Signals *PinsI8080::prepareCycle() const {
    auto s = Signals::put();
    // T4/T1
    if (signal_sync() == LOW) {
        while (true) {
            phi1_pulse();
            delayNanoseconds(phi1_lo_ns);
            phi2_hi();
            delayNanoseconds(phi2_hi_sync);
            if (signal_sync() != LOW) {
                // SYNC=H
                break;
            }
            delayNanoseconds(phi2_hi_prep);
            phi2_lo();
            delayNanoseconds(phi2_lo_ns);
        }
    }
    // T1
    // assert_debug();
    delayNanoseconds(phi2_hi_addr);
    s->getAddress();
    s->getStatus();
    // negate_debug();
    return s;
}

void PinsI8080::enterWait() const {
    digitalWriteFast(PIN_READY, LOW);
    delayNanoseconds(ready_setup_ns);
    // READY is sampled here
    phi2_lo();
    delayNanoseconds(phi2_lo_ns);
    phi_pulses();
    delayNanoseconds(phi2_lo_ns);
}

Signals *PinsI8080::resumeCycle(uint16_t pc) const {
    auto s = Signals::put();
    s->addr = pc;
    s->getStatus();
    // T1
    // assert_debug();
    assert_ready();
    phi1_hi();
    // negate_debug();
    return s;
}

Signals *PinsI8080::completeCycle(Signals *s) const {
    // T1
    phi2_lo();
    delayNanoseconds(phi2_lo_ns);
    //  T2
    phi1_pulse();
    if (s->read()) {
        phi2_hi();
        if (s->memory()) {
            if (s->readMemory()) {
                s->data = _mems->read(s->addr);
                delayNanoseconds(phi2_hi_memread);
            } else {
                delayNanoseconds(phi2_hi_inject);
            }
        } else if (s->ioRead()) {
            const uint8_t ioaddr = s->addr;
            if (_devs->isSelected(ioaddr)) {
                s->data = _devs->read(ioaddr);
            }
            delayNanoseconds(phi2_hi_ioread);
        } else if (s->intAck()) {
            s->data = InstI8080::vec2Inst(_devs->vector());
            delayNanoseconds(phi2_hi_intack);
        }
        // assert_debug();
        s->outData();
        // negate_debug();
        phi2_lo();
        delayNanoseconds(phi2_lo_ns);
        // T3
        phi1_pulse();
        delayNanoseconds(phi1_lo_ns);
        phi2_hi();
        // assert_debug();
        delayNanoseconds(phi2_hi_input);
        Signals::nextCycle();
        Signals::inputMode();
        // negate_debug();
        phi2_lo();
    } else if (s->write()) {
        phi2_hi();
        // assert_debug();
        delayNanoseconds(phi2_hi_next);
        Signals::nextCycle();
        // negate_debug();
        phi2_lo();
        delayNanoseconds(phi2_lo_ns);
        // T3
        phi1_pulse();
        // assert_debug();
        s->getData();
        // negate_debug();
        phi2_hi();
        if (s->memory()) {
            if (s->writeMemory()) {
                _mems->write(s->addr, s->data);
                delayNanoseconds(phi2_hi_memwrite);
            } else {
                delayNanoseconds(phi2_hi_capture);
            }
        } else {
            const uint8_t ioaddr = s->addr;
            if (_devs->isSelected(ioaddr)) {
                _devs->write(ioaddr, s->data);
            }
        }
        phi2_lo();
    } else {
        phi2_pulse();
        delayNanoseconds(phi2_lo_ns);
        // T3
        phi1_pulse();
        delayNanoseconds(phi1_lo_ns);
        phi2_pulse();
    }
    return s;
}

Signals *PinsI8080::inject(uint8_t data) const {
    return completeCycle(prepareCycle()->inject(data));
}

uint16_t PinsI8080::captureWrites(
        const uint8_t *inst, uint_fast8_t len, uint8_t *buf, uint_fast8_t max) {
    DEBUG(cli.print("@@ captureWrites: len="));
    DEBUG(cli.printDec(len));
    DEBUG(cli.print(" max="));
    DEBUG(cli.printlnDec(max));
    uint16_t addr = 0;
    uint_fast8_t inj = 0;
    uint_fast8_t cap = 0;
    auto s = resumeCycle();
    while (inj < len || cap < max) {
        if (inj < len)
            s->inject(inst[inj]);
        if (cap < max)
            s->capture();
        completeCycle(s);
        if (s->read()) {
            if (inj < len)
                ++inj;
        } else if (s->write()) {
            if (cap == 0)
                addr = s->addr;
            if (cap < max && buf)
                buf[cap++] = s->data;
        }
        DEBUG(cli.print("@@ captureWrites: inj="));
        DEBUG(cli.printDec(inj));
        DEBUG(cli.print(" cap="));
        DEBUG(cli.printDec(cap));
        DEBUG(cli.print(' '));
        DEBUG(s->print());
        s = prepareCycle();
    }
    enterWait();
    return addr;
}

void PinsI8080::idle() {
    // READY=L
    phi_pulses();
}

Signals *PinsI8080::loop() const {
    auto s = resumeCycle(_regs->nextIp());
    while (true) {
        completeCycle(s);
        _devs->loop();
        s = prepareCycle();
        if (s->halt()) {
            const auto halt = s->prev();
            Signals::discard(halt);
            return halt;
        }
        if (haltSwitch() && s->fetch()) {
            enterWait();
            return nullptr;
        }
    }
}

void PinsI8080::run() {
    _regs->restore();
    Signals::resetCycles();
    saveBreakInsts();
    const auto halt = loop();
    restoreBreakInsts();
    disassembleCycles();
    if (halt) {
        const auto pc = halt->addr;
        const auto inte = digitalReadFast(PIN_INTE);
        assert_reset();  // reseume from HALT
        negate_reset();
        prepareCycle();
        enterWait();
        // Registers, except PC and IE, are preserved
        regs<RegsI8080>()->saveContext(pc, inte);
    } else {
        _regs->save();
    }
}

bool PinsI8080::rawStep() const {
    auto s = resumeCycle(_regs->nextIp());
    const auto insn = _mems->read(s->addr);
    if (insn == InstI8080::HLT) {
        enterWait();
        return false;
    }
    s->inject(insn);
    do {
        completeCycle(s);
        s = prepareCycle();
    } while (!s->fetch());
    enterWait();
    return true;
}

bool PinsI8080::step(bool show) {
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

void PinsI8080::assertInt(uint8_t name) {
    digitalWriteFast(PIN_INT, HIGH);
}

void PinsI8080::negateInt(uint8_t name) {
    digitalWriteFast(PIN_INT, LOW);
}

void PinsI8080::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

void PinsI8080::disassembleCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles;) {
        const auto s = g->next(i);
        if (s->fetch() && !s->intAck()) {
            const auto next = _mems->disassemble(s->addr, 1);
            i += next - s->addr;
        } else {
            s->print();
            ++i;
        }
        idle();
    }
}

}  // namespace i8080
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
