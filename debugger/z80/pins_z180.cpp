#include "pins_z180.h"
#include "debugger.h"
#include "inst_z80.h"
#include "mems_z80.h"
#include "regs_z180.h"
#include "signals_z180.h"

namespace debugger {
namespace z180 {

using z80::InstZ80;
using z80::MemsZ80;

// clang-format off
/**
 * Z180 bus cycle.  0 wait / 1 wait
 *        __    __    __    __    __    __    __    __    __    __    __    __    __
 * EXTAL |  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__
 *          \ ____\     \ ____\     \____\     \ ____\      \____\     \ ____\       __
 *   PHI ___|    T|1____|    T|2____|    T|3____|    T|1____|    T|2____|    T|3____|
 *       ___\ ____\_____\_____v_____\ __________\ ____\_____\ ____v_________________\ _
 * ADDR  ____X___________________________________X____________X______________________X_
 *       __________                             _______________________________________
 * #MREQ           \___________________________/
 *       ______________________________________________                             ___
 * #IORQ                                               \___________________________/
 *       __________                             _______                             ___
 *   #RD           \___________________________/       \___________________________/
 *       _______________                        ____________                        ___
 *   #WR                \______________________/            \______________________/
 *                                  ____________           ________________________
 *  DATA  -------------------------<____________>---------<________________________>----
 *       _____________________v___________________________________v____________________
 * #WAIT  ___________________/ \_________________________________/ \___________________
 */
// clang-format on

namespace {

constexpr auto extal_hi_ns = 20;
constexpr auto extal_lo_ns = 20;
constexpr auto extal_lo_cntl = 20;

inline void extal_hi() {
    digitalWriteFast(PIN_EXTAL, HIGH);
}

inline void extal_lo() {
    digitalWriteFast(PIN_EXTAL, LOW);
}

inline void assert_nmi() {
    digitalWriteFast(PIN_NMI, LOW);
}

inline void negate_nmi() {
    digitalWriteFast(PIN_NMI, HIGH);
}

inline void assert_int0() {
    digitalWriteFast(PIN_INT0, LOW);
}

inline void negate_int0() {
    digitalWriteFast(PIN_INT0, HIGH);
}

inline void negate_wait() {
    digitalWriteFast(PIN_WAIT, HIGH);
}

inline void assert_wait() {
    digitalWriteFast(PIN_WAIT, LOW);
}

void negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
}

constexpr uint8_t PINS_LOW[] = {
        PIN_EXTAL,
        PIN_RESET,
        PIN_ASEL,
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_WAIT,
        PIN_BUSREQ,
        PIN_NMI,
        PIN_INT0,
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
        PIN_PHI,
        PIN_M1,
        PIN_MREQ,
        PIN_IORQ,
        PIN_RD,
        PIN_WR,
        PIN_BUSACK,
};

inline void extal_cycle_lo() {
    extal_hi();
    delayNanoseconds(extal_hi_ns);
    extal_lo();
}

inline void extal_cycle() {
    extal_cycle_lo();
    delayNanoseconds(extal_lo_ns);
}

}  // namespace

PinsZ180::PinsZ180() : PinsZ80Base() {
    _regs = new RegsZ180(this);
    mems<MemsZ80>()->setMaxAddr(0xFFFFF);  // 1MB
}

void PinsZ180::resetPins() {
    // Assert reset condition
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    // #RESET must be active for at least three CLK cycles for CPU to
    // properly accept it.
    for (auto i = 0; i < 100; i++)
        extal_cycle();
    negate_reset();
    Signals::resetCycles();
    assert_wait();
    _regs->setIp(InstZ80::ORG_RESET);
    configureCpu();
    _regs->save();
}

void PinsZ180::configureCpu() {
    // Set 0-wait for memory and 1-wait for I/O (DCNTL)
    // Disable DRAM refresh (RCR)
    static constexpr uint8_t CONFIG[] = {
            0xAF,              // XOR A
            0xED, 0x39, 0x32,  // OUT0 (DCNTL), A
            0xED, 0x39, 0x36,  // OUT0 (RCR), A
            0x18, 0xF7         // JR $-7
    };
    execInst(CONFIG, sizeof(CONFIG));
}

Signals *PinsZ180::prepareCycle() const {
    const auto s = Signals::put();
    while (true) {
        noInterrupts();
        do {
            // negate_debug();
            extal_hi();
            s->getAddr();
            extal_lo();
            delayNanoseconds(extal_lo_cntl);
            // assert_debug();
        } while (!s->getControl());
        // negate_debug();
        extal_hi();
        s->getAddrHigh();
        extal_lo();
        delayNanoseconds(extal_lo_cntl);
        // assert_debug();
        s->getControl();  // To capture #WR
        // negate_debug();
        interrupts();
        if (!s->nobus())
            return s;
        while (s->getControl())
            extal_cycle();
    }
}

Signals *PinsZ180::completeCycle(Signals *s) const {
    extal_hi();
    if (s->mreq()) {      // Memory request
        if (s->read()) {  // Memory read
            if (s->readMemory()) {
                s->data = _mems->read(s->addr);
            } else {
                delayNanoseconds(extal_hi_ns);
            }
            // assert_debug();
            s->outData();
            // negate_debug();
        } else if (s->write()) {  // Memory write
            // assert_debug();
            s->getData();
            // negate_debug();
            if (s->writeMemory()) {
                _mems->write(s->addr, s->data);
            } else {
                delayNanoseconds(extal_hi_ns);
            }
        }
    } else {  // I/O request
        // using lower 8-bit for I/O device addressing.
        const uint16_t ioaddr = s->addr & 0xFF;
        if (s->read() && _devs->isSelected(ioaddr)) {
            s->data = _devs->read(ioaddr);
            // assert_debug();
            s->outData();
            // negate_debug();
        } else if (s->write()) {  // I/O write
            // assert_debug();
            s->getData();
            // negate_debug();
            if (_devs->isSelected(ioaddr))
                _devs->write(ioaddr, s->data);
        } else if (s->intack()) {
            s->data = _devs->vector();
            // assert_debug();
            s->outData();
            // negate_debug();
        } else {
            delayNanoseconds(extal_hi_ns);
        }
    }
    extal_lo();
    s->inputMode();  // data bus hold is acting
    Signals::nextCycle();
    auto n = Signals::put();
    noInterrupts();
    while (n->getControl())
        extal_cycle();
    interrupts();
    return s;
}

Signals *PinsZ180::resumeCycle(uint16_t pc) const {
    const auto s = Signals::put();
    negate_wait();
    s->setAddress(pc);
    s->getControl();
    return completeCycle(s);
}

Signals *PinsZ180::inject(uint8_t data) const {
    return completeCycle(prepareCycle()->inject(data));
}

uint16_t PinsZ180::execute(
        const uint8_t *inst, uint_fast8_t len, uint8_t *buf, uint_fast8_t max) {
    uint16_t addr = 0;
    uint_fast8_t inj = 0;
    uint_fast8_t cap = 0;
    auto s = Signals::put();
    while (inj < len || cap < max) {
        if (inj < len)
            s->inject(inst[inj]);
        if (cap < max)
            s->capture();
        if (inj == 0) {
            resumeCycle(_regs->nextIp());
        } else {
            completeCycle(s);
        }
        if (!s->nobus()) {
            if (s->read()) {
                if (inj < len)
                    ++inj;
            } else {
                if (cap == 0)
                    addr = s->addr;
                if (cap < max && buf)
                    buf[cap++] = s->data;
            }
        }
        delayNanoseconds(extal_lo_ns);
        s = prepareCycle();
    }
    while (!s->fetch()) {
        completeCycle(s);
        s = prepareCycle();
    }
    assert_wait();
    return addr;
}

void PinsZ180::idle() {
    // #WAIT=L
    extal_cycle();
    extal_cycle_lo();
}

bool PinsZ180::isRst38Break(const Signals *org_rst) const {
    const auto rst38h = org_rst->prev(3);
    if (rst38h->data == InstZ80::RST38H) {
        if (org_rst->prev(1)->mwrite() && org_rst->prev(2)->mwrite()) {
            if (isBreakPoint(rst38h->addr))
                return true;  // break point
            if (_mems->read(InstZ80::ORG_RST38H) == InstZ80::RST38H)
                return true;  // halt to system
        }
    }
    return false;
}

Signals *PinsZ180::loop() {
    resumeCycle(_regs->nextIp());
    while (true) {
        const auto s = prepareCycle();
        if (s->addr == InstZ80::ORG_RST38H && s->fetch()) {
            if (isRst38Break(s)) {
                const auto rst38h = s->prev(3);
                // restore PC to the break point
                completeCycle(s->inject(InstZ80::RET));
                inject(lo(rst38h->addr));
                inject(hi(rst38h->addr));
                assert_wait();
                return rst38h;
            }
        }
        completeCycle(s);
        _devs->loop();
        if (haltSwitch())
            return suspend();
    }
}

void PinsZ180::run() {
    _regs->restore();
    Signals::resetCycles();
    saveBreakInsts();
    auto s = loop();
    assert_wait();
    Signals::discard(s);
    restoreBreakInsts();
    disassembleCycles();
    _regs->save();
}

Signals *PinsZ180::suspend() {
    assert_nmi();
    while (true) {
        const auto s = prepareCycle();
        if (s->fetch() && s->addr == InstZ80::ORG_NMI) {
            negate_nmi();
            completeCycle(s->inject(InstZ80::RETN_PREFIX));
            inject(InstZ80::RETN);
            inject(s->prev()->data);
            inject(s->prev(2)->data);
            assert_wait();
            return s->prev(3);
        }
        completeCycle(s);
    }
}

bool PinsZ180::rawStep() {
    const auto pc = _regs->nextIp();
    if (_mems->read_byte(pc) == InstZ80::HALT)
        return false;
    assert_nmi();  // Assert #NMI as soon as possible
    resumeCycle(pc);
    auto s = suspend();
    Signals::discard(s);
    return true;
}

bool PinsZ180::step(bool show) {
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

void PinsZ180::assertInt(uint8_t) {
    assert_int0();
}

void PinsZ180::negateInt(uint8_t) {
    negate_int0();
}

void PinsZ180::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

void PinsZ180::disassembleCycles() {
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

}  // namespace z180
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
