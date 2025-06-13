#include "pins_mn1613.h"
#include "debugger.h"
#include "devs_mn1613.h"
#include "inst_mn1613.h"
#include "mems_mn1613.h"
#include "regs_mn1613.h"
#include "signals_mn1613.h"

// #define DEBUG(e) e
#define DEBUG(e)

namespace debugger {
namespace mn1613 {

// clang-format off
/**
 * MN1613A bus cycle; BSAV=H
 *        __    __    __    __    __    __    __    __    __
 *    X2 |  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__
 *       ___\        \     \ ____\_________________\__\
 * #BSRQ     |________v_____|     v                 v  |_______
 *           v         _____
 *  ADSD _____________|     |__________________________________
 *       _________________________                   __________
 * #DTSD                          |_________________|
 *       __________________________                   _________
 * #DTAK                           \_________________/
 * EA0/1      ________________        _______________   _______
 *  BSxx ----<______addr______>------<______data_____>-<__addr_
 *  FSYC      ________________    ___________________   _______
 *   EA2 ----<_______EA2______>--<_________FSYC______>-<__EA2__
 * #CSTP      ________________        _______________   _______
 *   EA3 ----<_______EA3______>------<_____#CSTP_____>-<__EA3__
 *       ____ _______________________________________ _________
 *   WRT ____X_______________________________________X_________
 *       ____ _______________________________________ _________
 *  #IOP ____X_______________________________________X_________
 */
// clang-format on

namespace {
// f: max 13.34MHz, min 1 MHz;  X2 frequency

constexpr auto x2_hi_ns = 25;       // 37.5 ns
constexpr auto x2_lo_ns = 25;       // 37.5 ns
constexpr auto x2_hi_pre = 20;      // 37.5 ns
constexpr auto x2_lo_pre = 10;      // 37.5 ns
constexpr auto x2_hi_adsd = 20;     // 37.5 ns
constexpr auto x2_lo_adsd = 10;     // 37.5 ns
constexpr auto x2_lo_control = 0;   // 37.5 ns
constexpr auto x2_hi_read = 5;      // 37.5 ns
constexpr auto x2_lo_inject = 25;   // 37.5 ns
constexpr auto x2_hi_write = 5;     // 37.5 ns
constexpr auto x2_hi_get = 0;       // 37.5 ns
constexpr auto x2_lo_write = 10;    // 37.5 ns
constexpr auto x2_hi_capture = 25;  // 37.5 ns

const uint8_t PINS_LOW[] = {
        PIN_X2,
        PIN_RST,
        PIN_DTAK,
};

const uint8_t PINS_HIGH[] = {
        PIN_IRQ0,
        PIN_IRQ1,
        PIN_IRQ2,
        PIN_HLT,
        PIN_STRT,
        PIN_BSAV,
};

const uint8_t PINS_PULLUP[] = {
        PIN_RUN,  // CSRQ (bidirectional)
        PIN_ST,   // bidirectional
        PIN_SD,   // bidirectional
        PIN_EA3,  // CSTP (bidirectional)
};

const uint8_t PINS_INPUT[] = {
        PIN_SYNC,
        PIN_BSRQ,
        PIN_ADSD,
        PIN_DTSD,
        PIN_WRT,
        PIN_IOP,
        PIN_BS15,
        PIN_BS14,
        PIN_BS13,
        PIN_BS12,
        PIN_BS11,
        PIN_BS10,
        PIN_BS09,
        PIN_BS08,
        PIN_BS07,
        PIN_BS06,
        PIN_BS05,
        PIN_BS04,
        PIN_BS03,
        PIN_BS02,
        PIN_BS01,
        PIN_BS00,
        PIN_EA2,
        PIN_EA1,
        PIN_EA0,
};

inline void x2_lo() {
    digitalWriteFast(PIN_X2, LOW);
}

inline void x2_hi() {
    digitalWriteFast(PIN_X2, HIGH);
}

void x2_cycle_lo() {
    x2_hi();
    delayNanoseconds(x2_hi_ns);
    x2_lo();
}

void x2_cycle() {
    x2_cycle_lo();
    delayNanoseconds(x2_lo_ns);
}

void negate_rst() {
    digitalWriteFast(PIN_RST, HIGH);
}

void assert_hlt() {
    digitalWriteFast(PIN_HLT, LOW);
}

void negate_hlt() {
    digitalWriteFast(PIN_HLT, HIGH);
}

void assert_strt() {
    digitalWriteFast(PIN_STRT, LOW);
}

void negate_strt() {
    digitalWriteFast(PIN_STRT, HIGH);
}

auto halt_asserted() {
    return digitalReadFast(PIN_RUN) != LOW;
}

auto bsrq_asserted() {
    return digitalReadFast(PIN_BSRQ) == LOW;
}

auto adsd_asserted() {
    return digitalReadFast(PIN_ADSD) != LOW;
}

}  // namespace
   // namespace

PinsMn1613::PinsMn1613() {
    _devs = new DevsMn1613();
    auto mems = new MemsMn1613();
    _mems = mems;
    _regs = new RegsMn1613(this, mems);
}

void PinsMn1613::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_PULLUP, sizeof(PINS_PULLUP), INPUT_PULLUP);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    for (auto i = 0; i < 100; ++i)
        x2_cycle();
    negate_rst();
    while (adsd_asserted())
        x2_cycle();

    Signals::resetCycles();
    _regs->reset();
    _regs->save();
    regs<RegsMn1613>()->checkCpuType();
}

Signals *PinsMn1613::waitReset() const {
    // X2=LOW
    auto s = Signals::put();
    do {
        x2_hi();
        delayNanoseconds(x2_hi_pre);
        x2_lo();
        delayNanoseconds(x2_lo_pre);
    } while (!bsrq_asserted());
    return prepareCycle(s);
}

Signals *PinsMn1613::waitBus() const {
    // X2=LOW
    auto s = Signals::put();
    do {
        x2_hi();
        delayNanoseconds(x2_hi_pre);
        if (halt_asserted()) {
            x2_lo();
            return s;
        }
        x2_lo();
        delayNanoseconds(x2_lo_pre);
    } while (!bsrq_asserted());
    return prepareCycle(s);
}

Signals *PinsMn1613::prepareCycle(Signals *s) const {
    // X2=LOW
    do {
        x2_hi();
        delayNanoseconds(x2_hi_adsd);
        x2_lo();
        delayNanoseconds(x2_lo_adsd);
    } while (!adsd_asserted());
    s->getAddress();
    x2_hi();
    return s;
}

Signals *PinsMn1613::completeCycle(Signals *s) const {
    // X2=HIGH
    x2_lo();
    delayNanoseconds(x2_lo_ns);
    x2_cycle_lo();
    delayNanoseconds(x2_lo_control);
    s->getControl();
    x2_hi();
    if (s->read()) {
        delayNanoseconds(x2_hi_read);
        if (s->readMemory()) {
            if (s->memory()) {
                x2_lo();
                s->data = _mems->read(s->addr);
            } else {
                x2_lo();
                s->data = _devs->read(s->addr);
            }
        } else {
            x2_lo();
            delayNanoseconds(x2_lo_inject);
        }
        x2_hi();
        s->outData();
        x2_lo();
        Signals::nextCycle();
        x2_hi();
        s->getFetch();
        s->inputMode();
    } else {
        delayNanoseconds(x2_hi_write);
        x2_lo();
        Signals::nextCycle();
        x2_hi();
        delayNanoseconds(x2_hi_get);
        s->getData();
        x2_lo();
        delayNanoseconds(x2_lo_write);
        if (s->writeMemory()) {
            if (s->memory()) {
                x2_hi();
                _mems->write(s->addr, s->data);
            } else {
                x2_hi();
                _devs->write(s->addr, s->data);
            }
        } else {
            x2_hi();
            delayNanoseconds(x2_hi_capture);
        }
    }
    x2_lo();
    // X2=LOW
    return s;
}

void PinsMn1613::idle() {
    // CPU is halted
    x2_cycle_lo();
}

void PinsMn1613::loop() {
    negate_hlt();
    unhalt();
    while (true) {
        auto s = waitBus();
        if (halt_asserted()) {
            assert_hlt();
            return;
        }
        completeCycle(s);
        if (haltSwitch())
            assert_hlt();
        _devs->loop();
    }
}

void PinsMn1613::run() {
    _regs->restore();
    Signals::resetCycles();
    saveBreakInsts();
    loop();
    // discard prefetches
    auto s = Signals::put()->prev();
    do {
        s = s->prev();
        if (s->fetch() && _mems->read(s->addr) == InstMn1613::H) {
            Signals::discard(s);
            break;
        }
    } while (s != Signals::get());
    const auto halt = s != Signals::get();
    restoreBreakInsts();
    disassembleCycles();
    _regs->save();
    if (halt)
        _regs->setIp(regs<RegsMn1613>()->addIp(-1));  // offset H instruction
}

bool PinsMn1613::rawStep() {
    unhalt();
    while (true) {
        const auto s = waitBus();
        completeCycle(s);
        if (halt_asserted())
            return true;
        DEBUG(cli.print("@@ step: "));
        DEBUG(s->print());
    }
}

bool PinsMn1613::step(bool show) {
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

void PinsMn1613::injectReset(const uint16_t *data, uint_fast8_t len) {
    assert_hlt();
    for (uint_fast8_t i = 0; i < len; ++i) {
        const auto s = waitReset()->inject(data[i]);
        completeCycle(s);
        DEBUG(cli.print("@@ psw: "));
        DEBUG(s->print());
    }
}

void PinsMn1613::unhalt() const {
    assert_strt();
    delayNanoseconds(100);
    negate_strt();
    while (halt_asserted())
        x2_cycle();
}

void PinsMn1613::halt() const {
    assert_hlt();
    while (true) {
        const auto s = waitBus()->inject(InstMn1613::NOP)->capture();
        if (halt_asserted()) {
            Signals::discard(s);
            return;
        }
        completeCycle(s);
    }
}

void PinsMn1613::injectInst(uint16_t inst) {
    injectInst(&inst, 1);
}

void PinsMn1613::injectInst(const uint16_t *inst, uint_fast8_t len) {
    unhalt();
    for (uint_fast8_t i = 0; i < len; ++i) {
        const auto s = waitBus()->inject(inst[i]);
        completeCycle(s);
        DEBUG(cli.print("@@  inject: "));
        DEBUG(s->print());
    }
    halt();
}

void PinsMn1613::captureInst(uint16_t inst, uint16_t *buf) {
    unhalt();
    auto s = waitBus()->inject(inst);
    completeCycle(s);
    DEBUG(cli.print("@@  inject: "));
    DEBUG(s->print());
    while (true) {
        s = waitBus()->inject(InstMn1613::NOP)->capture();
        completeCycle(s);
        if (s->write()) {
            *buf = s->data;
            DEBUG(cli.print("@@ capture: "));
            DEBUG(s->print());
            break;
        } else {
            DEBUG(cli.print("@@        : "));
            DEBUG(s->print());
        }
    }
    halt();
}

void PinsMn1613::injectReads(
        uint16_t inst, uint32_t addr, const uint16_t *data, uint_fast8_t len) {
    unhalt();
    auto s = waitBus()->inject(inst);
    completeCycle(s);
    DEBUG(cli.print("@@  inject: addr="));
    DEBUG(cli.printlnHex(addr, 5));
    DEBUG(cli.print("@@  inject: "));
    DEBUG(s->print());
    uint_fast8_t cyc = 0;
    for (uint_fast8_t inj = 0; inj < len && cyc < 10; cyc++) {
        s = waitBus();
        if (s->addr == addr + inj) {
            s->inject(data[inj]);
        } else {
            s->capture()->inject(InstMn1613::NOP);
        }
        completeCycle(s);
        if (s->read() && s->addr == addr + inj) {
            inj++;
            DEBUG(cli.print("@@  inject: "));
            DEBUG(s->print());
        } else {
            DEBUG(cli.print("@@        : "));
            DEBUG(s->print());
        }
    }
    halt();
}

void PinsMn1613::assertInt(uint8_t name_) {
    const auto name = static_cast<IntrName>(name_);
    if (name == INTR_IRQ0)
        digitalWriteFast(PIN_IRQ0, LOW);
    if (name == INTR_IRQ1)
        digitalWriteFast(PIN_IRQ1, LOW);
    if (name == INTR_IRQ2)
        digitalWriteFast(PIN_IRQ2, LOW);
}

void PinsMn1613::negateInt(uint8_t name_) {
    const auto name = static_cast<IntrName>(name_);
    if (name == INTR_IRQ0)
        digitalWriteFast(PIN_IRQ0, HIGH);
    if (name == INTR_IRQ1)
        digitalWriteFast(PIN_IRQ1, HIGH);
    if (name == INTR_IRQ2)
        digitalWriteFast(PIN_IRQ2, HIGH);
}

void PinsMn1613::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, InstMn1613::H);
}

void PinsMn1613::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

void PinsMn1613::disassembleCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles;) {
        const auto s = g->next(i);
        if (s->fetch()) {
            const auto len = _mems->disassemble(s->addr, 1) - s->addr;
            // print bus cycles other than instruction bytes
            for (uint_fast8_t j = 1; j < len; j++) {
                const auto t = s->next(j);
                if (t->addr < s->addr || t->addr >= s->addr + len) {
                    t->print();
                    idle();
                }
            }
            i += len;
        } else {
            s->print();
            ++i;
        }
        idle();
    }
}

}  // namespace mn1613
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
