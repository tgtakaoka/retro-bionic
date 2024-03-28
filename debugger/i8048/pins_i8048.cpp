#include "pins_i8048.h"
#include "debugger.h"
#include "devs_i8048.h"
#include "digital_bus.h"
#include "inst_i8048.h"
#include "mems_i8048.h"
#include "regs_i8048.h"
#include "signals_i8048.h"

namespace debugger {
namespace i8048 {

// clang-format off
/**
 * P8048 bus cycle.
 *        |                                                           |
 *        |_   _   _   _   _   _   _   _   _   _   _   _   _   _   _  |_   _
 * XTAL1 _|1|_|2|_|3|_|4|_|5|_|6|_|7|_|8|_|9|_|A|_|B|_|C|_|D|_|E|_|F|_|1|_|2
 *        \ ____________\     \     \                 \             \   \ __
 *   ALE __|             |_____|_____|_________________|_____________|___|
 *       ______________________|     |                 |_____________|______
 * #PSEN                       |_____|_________________|             |
 *       ____________________________|                               |______
 *   #RD                             |_______________________________|
 *       ____________________________|                               |______
 *   #WR                             |_______________________________|
 *                  ________                  ____________
 *   BUS ----------<__addr__>----------------<____inst____>-----------------
 *                  ________                  ______________________
 *   BUS ----------<__addr__>----------------<_________data_________>-------
 */
// clang-format on

namespace {
//      t: min  91 ns; clock period (11 MHz)
//    tLL: min 150 ns; ALE width
//    tAL: min  70 ns; Addr setup to ALE-
//    tLA: min  50 ns; Addr hold from ALE-
//   tCC1: min 480 ns; #RD/#WR width
//   tCC2: min 350 ns; #PSEN width
//    tDW: min 390 ns; Data setup to #WR+
//    tWD: min  40 ns; Data hold from #WR+
//    tDR: min   0 ns, max 110 ns; Data hold from #RD/#PSEN+
//   tRD1: max 375 ns; #RD- to data in
//   tRD2: max 460 ns; #PSEN- to data in
// tLAFC1: min 200 ns; ALE- to #RD/#WR-
// tLAFC2: min  60 ns; ALE- to #PSEN-
//   tCA1: min  25 ns; #RD/#WR+ to ALE+
//   tAC2: min 290 ns; #PSEN+ to ALE+
constexpr auto xtal1_hi_ns = 32;       // 50 ns
constexpr auto xtal1_lo_ns = 32;       // 50 ns
constexpr auto xtal1_hi_ale = 16;      // 50 ns
constexpr auto xtal1_lo_ale = 16;      // 50 ns
constexpr auto xtal1_hi_control = 16;  // 50 ns
constexpr auto xtal1_lo_control = 8;   // 50 ns
constexpr auto xtal1_lo_fetch = 30;    // 50 ns
constexpr auto xtal1_hi_memory = 10;   // 50 ns
constexpr auto xtal1_hi_inject = 20;   // 50 ns
constexpr auto xtal1_lo_input = 14;    // 50 ns
constexpr auto xtal1_lo_read = 10;     // 50 ns
constexpr auto xtal1_lo_write = 1;     // 50 ns
constexpr auto xtal1_hi_write = 20;    // 50 ns
constexpr auto xtal1_hi_capture = 28;  // 50 ns

inline void xtal1_hi() {
    digitalWriteFast(PIN_XTAL1, HIGH);
}

inline void xtal1_lo() {
    digitalWriteFast(PIN_XTAL1, LOW);
}

inline void assert_int() {
    digitalWriteFast(PIN_INT, LOW);
}

inline void negate_int() {
    digitalWriteFast(PIN_INT, HIGH);
}

inline auto signal_ale() {
    return digitalReadFast(PIN_ALE);
}

inline auto signal_psen() {
    return digitalReadFast(PIN_PSEN);
}

inline auto signal_rd() {
    return digitalReadFast(PIN_RD);
}

inline auto signal_wr() {
    return digitalReadFast(PIN_WR);
}

inline auto signal_prog() {
    return digitalReadFast(PIN_PROG);
}

inline auto signal_ss() {
    return digitalReadFast(PIN_SS);
}

inline void assert_ss() {
    digitalWriteFast(PIN_SS, LOW);
}

inline void negate_ss() {
    digitalWriteFast(PIN_SS, HIGH);
}

void assert_reset() {
    // Drive RESET condition
    xtal1_hi();
    digitalWriteFast(PIN_RESET, LOW);
    negate_int();
    negate_ss();
}

void negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
}

const uint8_t PINS_LOW[] = {
        PIN_RESET,
        PIN_T1,
};

const uint8_t PINS_HIGH[] = {
        PIN_XTAL1,
        PIN_INT,
        PIN_SS,
};

const uint8_t PINS_PULLUP[] = {
        PIN_T0,  // T0 can be output by ENT0 CLK
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
        PIN_ADDR8,
        PIN_ADDR9,
        PIN_ADDR10,
        PIN_ADDR11,
        PIN_ALE,
        PIN_PSEN,
        PIN_RD,
        PIN_WR,
        PIN_PROG,
        PIN_P24,
        PIN_P25,
        PIN_P26,
        PIN_P27,
        PIN_P10,
        PIN_P11,
        PIN_P12,
        PIN_P13,
        PIN_P14,
        PIN_P15,
        PIN_P16,
        PIN_P17,
};

inline void xtal1_cycle_lo() {
    xtal1_hi();
    delayNanoseconds(xtal1_hi_ns);
    xtal1_lo();
}

inline void xtal1_cycle() {
    xtal1_cycle_lo();
    delayNanoseconds(xtal1_lo_ns);
}

}  // namespace

void PinsI8048::reset() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_PULLUP, sizeof(PINS_PULLUP), INPUT_PULLUP);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    assert_reset();
    // Only 5 machine cycles are required if power is already on.
    for (auto i = 0; i < 15 * (5 + 2); ++i)
        xtal1_cycle();
    while (signal_psen() != LOW)
        xtal1_cycle();
    negate_reset();
    // #SS=L
    Signals::resetCycles();
    _regs.save();
}

Signals *PinsI8048::prepareCycle() {
    auto s = Signals::put();
    // tC~t1
    while (signal_ale() == LOW)
        xtal1_cycle();
    while (true) {
        // ALE=H
        // t2~t5
        for (auto t = 2;; ++t) {
            xtal1_hi();
            s->getAddress();
            delayNanoseconds(xtal1_hi_ale);
            xtal1_lo();
            delayNanoseconds(xtal1_lo_ale);
            if (signal_ale() == LOW)
                break;
            if (t >= 5 && signal_ss() == LOW) {
                s->markInvalid();
                return s;
            }
        }
        // ALE=L
        // t6~t8
        while (true) {
            xtal1_hi();
            delayNanoseconds(xtal1_hi_control);
            if (s->getControl()) {  // #RD/#WR
                return s;
            }
            negate_debug();
            xtal1_lo();
            delayNanoseconds(xtal1_lo_control);
            if (signal_ale() != LOW)
                break;  // found next cycle
            const auto valid = s->getControl();  // #PSEN
            if (valid) {
                // t7
                xtal1_hi();
                delayNanoseconds(xtal1_hi_control);
                return s;
            }
        }
    }
}

Signals *PinsI8048::completeCycle(Signals *s) {
    xtal1_lo();
    if (s->fetch()) {  // program read
        delayNanoseconds(xtal1_lo_fetch);
        // t8
        xtal1_hi();
        if (s->readMemory()) {
            s->data = _mems.raw_read(s->addr);
            delayNanoseconds(xtal1_hi_memory);
        } else {
            delayNanoseconds(xtal1_hi_inject);
        }
        xtal1_lo();
        delayNanoseconds(xtal1_lo_ns);
        // t9
        xtal1_cycle_lo();
        busWrite(DB, s->data);
        busMode(DB, OUTPUT);
        // tA~tC
        xtal1_cycle();
        xtal1_cycle();
        xtal1_cycle();
        // tD
        xtal1_cycle_lo();
        busMode(DB, INPUT);
        delayNanoseconds(xtal1_lo_input);
        // tE~tF
        xtal1_cycle();
        xtal1_cycle_lo();
    } else if (s->read()) {  // external data read
        delayNanoseconds(xtal1_lo_read);
        // t9
        xtal1_hi();
        if (s->readMemory()) {
            s->data = DataMemory.read(s->addr);
        } else {
            delayNanoseconds(xtal1_hi_inject);
        }
        xtal1_lo();
        delayNanoseconds(xtal1_lo_ns);
        // tA~tB
        xtal1_cycle();
        xtal1_cycle_lo();
        busWrite(DB, s->data);
        busMode(DB, OUTPUT);
        // tC~tF
        xtal1_cycle();
        xtal1_cycle();
        xtal1_cycle();
        xtal1_cycle_lo();
        while (signal_rd() == LOW) // ensure tDR
            ;
        busMode(DB, INPUT);
    } else if (s->write()) {  // external data write
        delayNanoseconds(xtal1_lo_write);
        // t9
        xtal1_cycle();
        // tA
        xtal1_hi();
        delayNanoseconds(xtal1_hi_write);
        s->getData();
        xtal1_lo();
        if (s->writeMemory()) {
            DataMemory.write(s->addr, s->data);
        } else {
            delayNanoseconds(xtal1_hi_capture);
        }
        // tB~tF
        xtal1_cycle();
        xtal1_cycle();
        xtal1_cycle();
        xtal1_cycle();
        xtal1_cycle_lo();
    } else if (s->port()) {
        xtal1_lo();
        delayNanoseconds(xtal1_lo_ns);
        // t7~tF
        while (signal_prog() == LOW) {
            xtal1_cycle();
        }
    }
    Signals::nextCycle();
    return s;
}

Signals *PinsI8048::inject(uint8_t data) {
    auto s = prepareCycle();
    s->inject(data);
    return completeCycle(s);
}

Signals *PinsI8048::cycle(uint8_t data) {
    negate_ss();
    auto s = inject(data);
    assert_ss();
    return s;
}

void PinsI8048::execInst(const uint8_t *inst, uint8_t len) {
    execute(inst, len, nullptr, nullptr, 0);
}

uint8_t PinsI8048::captureWrites(const uint8_t *inst, uint8_t len,
        uint16_t *addr, uint8_t *buf, uint8_t max) {
    return execute(inst, len, addr, buf, max);
}

uint8_t PinsI8048::execute(const uint8_t *inst, uint8_t len, uint16_t *addr,
        uint8_t *buf, uint8_t max) {
    negate_ss();
    uint8_t inj = 0;
    uint8_t cap = 0;
    while (inj < len || cap < max) {
        auto s = prepareCycle();
        if (inj == 0 && addr)
            *addr = s->addr;
        if (inj < len)
            s->inject(inst[inj]);
        if (cap < max)
            s->capture();
        completeCycle(s);
        if (s->fetch() || s->read())
            ++inj;
        if (s->write()) {
            if (cap < max && buf)
                buf[cap++] = s->data;
        }
    }
    assert_ss();
    return cap;
}

void PinsI8048::idle() {
    // #SS=L
    xtal1_cycle();
}

void PinsI8048::loop() {
    while (true) {
        Devs.loop();
        if (!rawStep() || haltSwitch()) {
            return;
        }
    }
}

void PinsI8048::run() {
    _regs.restore();
    Signals::resetCycles();
    saveBreakInsts();
    loop();
    assert_ss();
    restoreBreakInsts();
    disassembleCycles();
    _regs.save();
}

void PinsI8048::injectJumpHere(Signals *s) {
    const auto offset = InstI8048::offset(s->addr - 1);
    s->inject(InstI8048::JMP(offset));
    completeCycle(s);
    inject(lo(offset));
    auto j = prepareCycle();
    const auto mb = InstI8048::mb(j->addr);
    if (mb == InstI8048::mb(s->addr)) {
        j->inject(InstI8048::NOP);
        completeCycle(j);
    } else {
        j->inject(InstI8048::SEL_MB(InstI8048::mb(s->addr)));
        completeCycle(j);
        inject(InstI8048::JMP(offset));
        inject(lo(offset));
        inject(InstI8048::SEL_MB(mb));
    }
}

bool PinsI8048::rawStep(bool step) {
    negate_ss();
    auto s = prepareCycle();
    const auto inst = _mems.raw_read(s->addr);
    const auto len = _inst.instLength(inst);
    if (inst == InstI8048::HALT || len == 0) {
        injectJumpHere(s);
        Signals::discard(s);
        return false;
    }
    if (step) {
        assert_ss();
        while (s->valid()) {
            completeCycle(s);
            s = prepareCycle();
        }
        return true;
    }
    const auto cycles = _inst.busCycles(inst);
    completeCycle(s);
    for (auto i = 1; i < cycles; ++i) {
        completeCycle(prepareCycle())->clearFetch();
    }
    return true;
}

bool PinsI8048::step(bool show) {
    _regs.restore();
    Signals::resetCycles();
    if (rawStep(true)) {
        if (show)
            printCycles();
        _regs.save();
        return true;
    }
    return false;
}

void PinsI8048::assertInt(uint8_t name) {
    assert_int();
}

void PinsI8048::negateInt(uint8_t name) {
    negate_int();
}

void PinsI8048::setBreakInst(uint32_t addr) const {
    _mems.put_inst(addr, InstI8048::HALT);
}

void PinsI8048::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

void PinsI8048::disassembleCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles;) {
        const auto s = g->next(i);
        if (s->fetch()) {
            const auto len = _mems.disassemble(s->addr, 1) - s->addr;
            const auto cycles = _inst.busCycles(s->data);
            for (auto i = len; i < cycles; ++i)
                s->next(i)->print();
            i += cycles;
        } else {
            s->print();
            ++i;
        }
        idle();
    }
}

}  // namespace i8048
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
