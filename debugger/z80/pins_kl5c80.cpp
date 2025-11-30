#include "pins_kl5c80.h"
#include "debugger.h"
#include "devs_z80.h"
#include "inst_z80.h"
#include "mems_z80.h"
#include "regs_z80.h"
#include "signals_kl5c80.h"

namespace debugger {
namespace kl5c80 {

using z80::InstZ80;
using z80::MemsZ80;

// clang-format off
/**
 * KL5C80 bus cycle.  0 wait / 1 wait
 *       __    __    __    __    __    __    __    __    __    __    __    __    __
 *  XIN    |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__
 *       __\     \ ____\     \ ____\     \ ____\     \ ____\      \____\     \ ____\
 *  CLK     |_____|     |_____|     |_____|     |_____|     |_____|     |_____|     |__
 *       ___\ __________\___________\ ________________v_____\ ________________v_____\ _
 * ADDR  ____X___________X___________X_______________________X_______________________X_
 *       __________      _________________                    _________________________
 * #EMRD ____/     \____/                 \__________________/
 *       ______________________      ______________________________                  __
 * #EMWR                       \____/                              \________________/
 *                     __        ____                       ___      _______________
 * DATA  -------------<__>------<____>---------------------<___>----<_______________>--
 *       _____________________________________________v_______________________v________
 * ERDY  ____________________________________________/ \_____________________/ \_______
 */
// clang-format on

namespace {

constexpr auto xin_hi_ns = 30;
constexpr auto xin_lo_ns = 40;
constexpr auto xin_lo_cntl = 4;
constexpr auto xin_lo_get = 10;
constexpr auto xin_hi_get = 10;
constexpr auto xin_hi_inject = 20;
constexpr auto xin_hi_capture = 20;

inline void xin_hi() {
    digitalWriteFast(PIN_XIN, HIGH);
}

inline void xin_lo() {
    digitalWriteFast(PIN_XIN, LOW);
}

inline void assert_nmi() {
    digitalWriteFast(PIN_NMI, LOW);
}

inline void negate_nmi() {
    digitalWriteFast(PIN_NMI, HIGH);
}

inline void assert_int() {
    digitalWriteFast(PIN_IR0, HIGH);
}

inline void negate_int() {
    digitalWriteFast(PIN_IR0, LOW);
}

inline void assert_ready() {
    digitalWriteFast(PIN_ERDY, HIGH);
}

inline void negate_ready() {
    digitalWriteFast(PIN_ERDY, LOW);
}

void negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
}

constexpr uint8_t PINS_LOW[] = {
        PIN_XIN,
        PIN_RESET,
        PIN_IR0,
        PIN_ASEL,
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_ERDY,
        PIN_BREQ,
        PIN_NMI,
};

constexpr uint8_t PINS_INPUT[] = {
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
        PIN_CLK,
        PIN_M1,
        PIN_EMRD,
        PIN_EMWR,
        PIN_EIORD,
        PIN_EIOWR,
        PIN_BACK,
};

inline void xin_cycle_lo() {
    xin_hi();
    delayNanoseconds(xin_hi_ns);
    xin_lo();
}

inline void xin_cycle() {
    xin_cycle_lo();
    delayNanoseconds(xin_lo_ns);
}

}  // namespace

PinsKl5c80::PinsKl5c80() : PinsZ80Base() {
    _regs = new z80::RegsZ80(this);
    mems<MemsZ80>()->setMaxAddr(0x7FFFF);  // 512kB
}

void PinsKl5c80::resetPins() {
    // Assert reset condition
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    // #RESET must be active for at least three CLK cycles for CPU to
    // properly accept it.
    for (auto i = 0; i < 100; i++)
        xin_cycle();
    negate_reset();
    Signals::resetCycles();
    enableExternalReady(true);
    prepareWait();
    _regs->setIp(InstZ80::ORG_RESET);
    _regs->save();
}

void PinsKl5c80::enableExternalReady(bool enable) {
    static constexpr uint8_t PUSH_AF[] = {
            0xF5,  // PUSH AF
    };
    struct {
        uint8_t a;
        uint8_t f;
    } reg;
    captureWrites(PUSH_AF, sizeof(PUSH_AF), (uint8_t *)&reg, sizeof(reg));
    uint8_t config[] = {
            0x3E, 0x0F,          // LD A, <SCR1>
            0xD3, 0x3B,          // OUT (SCR1), A
            0xF1, reg.f, reg.a,  // POP_AF
            0x18, 0xF8,          // JR $-6
    };
    if (enable) {
        // Enable #M1,#NMI,#BREQ,#BACK
        // memory 1-wait, I/O 2-wait, wide write option
        config[1] = 0x4F;
    } else {
        // Enable #M1,#NMI,#BREQ,#BACK
        // memory 0-wait, I/O 1-wait
        config[1] = 0xCF;
    }
    execInst(config, sizeof(config));
}

Signals *PinsKl5c80::prepareCycle() const {
    const auto s = Signals::put();
    noInterrupts();
    do {
        // negate_debug();
        xin_hi();
        s->getAddr();
        xin_lo();
        delayNanoseconds(xin_lo_cntl);
        s->getAddrHigh();
        // assert_debug();
    } while (!s->getControl());
    // negate_debug();
    interrupts();
    return s;
}

Signals *PinsKl5c80::completeCycle(Signals *s) const {
    xin_hi();
    if (s->mrd()) {  // Memory read
        if (s->readMemory()) {
            s->data = _mems->read(s->addr);
        } else {
            delayNanoseconds(xin_hi_inject);
        }
        // assert_debug();
        s->outData();
        // negate_debug();
    } else if (s->mwr()) {  // Memory write
        // assert_debug();
        s->getData();
        // negate_debug();
        if (s->writeMemory()) {
            _mems->write(s->addr, s->data);
        } else {
            delayNanoseconds(xin_hi_capture);
        }
    } else {  // I/O cycles
        const uint8_t ioaddr = s->addr;
        if (s->iord()) {  // I/O read
            if (_devs->isSelected(ioaddr))
                s->data = _devs->read(ioaddr);
            // assert_debug();
            s->outData();
            // negate_debug();
        } else if (s->iowr()) {  // I/O write
            // assert_debug();
            s->getData();
            // negate_debug();
            if (_devs->isSelected(ioaddr))
                _devs->write(ioaddr, s->data);
        }
    }
    xin_lo();
    s->inputMode();  // data bus hold is acting
    Signals::nextCycle();
    auto n = Signals::put();
    noInterrupts();
    while (n->getControl())  // wait for end of bus cycle
        xin_cycle();
    interrupts();
    return s;
}

Signals *PinsKl5c80::prepareWait() const {
    negate_ready();
    const auto s = prepareCycle();
    return s;
}

Signals *PinsKl5c80::resumeCycle(uint16_t pc) const {
    const auto s = Signals::put();
    assert_ready();
    s->setAddress(pc);
    s->getControl();
    return completeCycle(s);
}

Signals *PinsKl5c80::inject(uint8_t data) const {
    return completeCycle(prepareCycle()->inject(data));
}

uint16_t PinsKl5c80::execute(
        const uint8_t *inst, uint_fast8_t len, uint8_t *buf, uint_fast8_t max) {
    uint16_t addr = 0;
    uint8_t inj = 0;
    uint8_t cap = 0;
    auto s = Signals::put();
    while (true) {
        if (inj < len)
            s->inject(inst[inj]);
        if (cap < max)
            s->capture();
        if (inj == 0) {
            resumeCycle(_regs->nextIp());
        } else {
            completeCycle(s);
        }
        if (s->mrd()) {
            if (inj < len)
                ++inj;
        } else if (s->mwr()) {
            if (cap == 0)
                addr = s->addr;
            if (cap < max && buf)
                buf[cap++] = s->data;
        }
        if (inj >= len && cap >= max) {
            prepareWait();
            return addr;
        }
        delayNanoseconds(xin_lo_ns);
        s = prepareCycle();
    }
}

void PinsKl5c80::idle() {
    // ERDY=L
    xin_cycle();
    xin_cycle_lo();
}

Signals *PinsKl5c80::loop() {
    resumeCycle(_regs->nextIp());
    while (true) {
        const auto s = prepareCycle();
        if (s->fetch() && _mems->read_byte(s->addr) == InstZ80::HALT) {
            completeCycle(s->inject(InstZ80::JR));
            inject(InstZ80::JR_HERE);
            return s;
        }
        completeCycle(s);
        _devs->loop();
        if (haltSwitch())
            return suspend();
    }
}

void PinsKl5c80::run() {
    _regs->restore();
    Signals::resetCycles();
    saveBreakInsts();
    enableExternalReady(false);
    auto s = loop();
    enableExternalReady(true);
    prepareWait();
    Signals::discard(s);
    restoreBreakInsts();
    disassembleCycles();
    _regs->save();
}

Signals *PinsKl5c80::suspend() {
    assert_nmi();
    while (true) {
        const auto s = prepareCycle();
        if (s->fetch() && s->addr == InstZ80::ORG_NMI) {
            negate_nmi();
            completeCycle(s->inject(InstZ80::RETN_PREFIX));
            inject(InstZ80::RETN);
            inject(s->prev()->data);
            inject(s->prev(2)->data);
            return s->prev(3);
        }
        completeCycle(s);
    }
}

bool PinsKl5c80::rawStep() {
    const auto pc = _regs->nextIp();
    if (_mems->read_byte(pc) == InstZ80::HALT)
        return false;
    resumeCycle(pc);
    auto s = suspend();
    Signals::discard(s);
    prepareWait();
    return true;
}

bool PinsKl5c80::step(bool show) {
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

void PinsKl5c80::assertInt(uint8_t) {
    assert_int();
}

void PinsKl5c80::negateInt(uint8_t) {
    negate_int();
}

void PinsKl5c80::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

void PinsKl5c80::disassembleCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles;) {
        const auto s = g->next(i);
        if (s->fetch()) {
            const auto next = _mems->disassemble(s->addr, 1);
            i += next - s->addr;
        } else {
            s->print();
            ++i;
        }
        idle();
    }
}

}  // namespace kl5c80
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
