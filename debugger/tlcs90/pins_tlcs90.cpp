#include "pins_tlcs90.h"
#include "debugger.h"
#include "devs_tlcs90.h"
#include "inst_tlcs90.h"
#include "mems_tlcs90.h"
#include "regs_tlcs90.h"
#include "signals_tlcs90.h"

namespace debugger {
namespace tlcs90 {

// clang-format off
/*
 * TMP90C802 Bus cycle
 *          |     T1    |     T2    |     T1    |     T2    |
 *          |__    __   |__    __   |__    __   |__    __   |__
 *     X1 __| c|1_| c|2_| c|3_| c|4_| c|1_| c|2_| c|3_| c|4_|
 *          \  \ _\________\     \  \  \ _\__\_____\     \  \
 *    CLK ___|__|  |        |_____|__|__|  |  |     |_____|__|_
 *        ___|_____|        |     |__|_____|__|_____|_____|__|_
 *   #RD     |     |________|_____|  |     |  |     |     |  |
 *       ____|______________|_____|__|_____|  |     |     |__|_
 *   #WR     |              |     |  |     |__|_____|_____|  |
 *        ___|______________|_____|__|________|_____|_____|__|_
 * A0~A15 ___X______________|_____|__X________|_____|_____X__|_
 *        ___               |    _|_          |_____|________|
 *   DATA ___>--------------|---<___>---------<_____|________>-
 *        __________________|_________________>_____|__________
 *  W#AIT _________________/ \_____________________/ \_________
 */
// clang-format on

namespace {

// tCYC: min 400 ns; CLK period
//  tWH: min 160 ns; CLK high width
//  tWL: min 160 ns; CLK low width
//  tAC: min  55 ns; address setup to #RD/#WR
//  tRR: min 210 ns; #RD low width
//  tRD: max 170 ns; #RD low to valid data
// tCHL: min  40 ns; #RD/#WR hold after CLK
//  tWW: min 210 ns; #WR low width
//  tDW: min 150 ns; data setup to #WR
constexpr auto x1_hi_ns = 28;       // 50 ns
constexpr auto x1_lo_ns = 10;       // 50 ns
constexpr auto x1_lo_clk = 40;      // 50 ns
constexpr auto c1_lo_ns = 0;        // 50 ns
constexpr auto c2_hi_ns = 2;        // 50 ns
constexpr auto c3_hi_read = 0;      // 50 ns
constexpr auto c3_lo_read = 10;     // 50 ns
constexpr auto c4_hi_read = 26;     // 50 ns
constexpr auto c4_lo_read = 0;      // 50 ns
constexpr auto c3_hi_write = 4;     // 50 ns
constexpr auto c3_lo_write = 0;     // 50 ns
constexpr auto c4_hi_write = 0;     // 50 ns
constexpr auto c4_hi_capture = 22;  // 50 ns
constexpr auto c4_lo_write = 0;     // 50 ns
constexpr auto c3_hi_nobus = 4;     // 50 ns
constexpr auto c4_lo_nobus = 32;    // 50 ns

inline void x1_hi() {
    digitalWriteFast(PIN_X1, HIGH);
}

inline auto signal_clk() {
    return digitalReadFast(PIN_CLK);
}

void assert_nmi() {
    digitalWriteFast(PIN_NMI, LOW);
}

void negate_nmi() {
    digitalWriteFast(PIN_NMI, HIGH);
}

void assert_int0() {
    digitalWriteFast(PIN_INT0, HIGH);
}

void negate_int0() {
    digitalWriteFast(PIN_INT0, LOW);
}

void assert_int1() {
    digitalWriteFast(PIN_INT1, HIGH);
}

void negate_int1() {
    digitalWriteFast(PIN_INT1, LOW);
}

inline void assert_wait() {
    digitalWriteFast(PIN_WAIT, LOW);
}

inline void negate_wait() {
    digitalWriteFast(PIN_WAIT, HIGH);
}

void negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
}

constexpr uint8_t PINS_LOW[] = {
        PIN_RESET,
        PIN_INT0,
        PIN_INT1,
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_X1,
        PIN_NMI,
        PIN_WAIT,
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
        PIN_AD0,
        PIN_AD1,
        PIN_AD2,
        PIN_AD3,
        PIN_AD4,
        PIN_AD5,
        PIN_AD6,
        PIN_AD7,
        PIN_AD8,
        PIN_AD9,
        PIN_AD10,
        PIN_AD11,
        PIN_AD12,
        PIN_AD13,
        PIN_AD14,
        PIN_AD15,
        PIN_RD,
        PIN_WR,
        PIN_CLK,
};

}  // namespace

PinsTlcs90::PinsTlcs90() {
    auto regs = new RegsTlcs90(this);
    _regs = regs;
    _devs = new DevsTlcs90();
    _mems = new MemsTlcs90(regs, _devs);
}

void PinsTlcs90::x1_lo() const {
    digitalWriteFast(PIN_X1, LOW);
    devs<DevsTlcs90>()->uartLoop();
}

void PinsTlcs90::x1_cycle() const {
    x1_lo();
    delayNanoseconds(x1_lo_ns);
    x1_hi();
    delayNanoseconds(x1_hi_ns);
}

void PinsTlcs90::resetPins() {
    // Assert reset condition
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    // #RESET input must be maintained at the "0" level for at least
    // #10 systemn clock cycles (10 stated; 2usec at 10MHz).
    for (auto i = 0; i < 20 * 2 || signal_clk() == LOW; ++i)
        x1_cycle();
    negate_reset();
    Signals::resetCycles();
    while (true) {
        x1_lo();
        delayNanoseconds(x1_lo_clk);
        if (signal_clk() == LOW) {
            // C3L
            break;
        }
        x1_hi();
        delayNanoseconds(x1_hi_ns);
    }
    // C4H
    x1_hi();
    delayNanoseconds(x1_hi_ns);
    // C4L, C1H
    x1_cycle();
    prepareCycle();

    _regs->reset();
    _regs->save();
}

Signals *PinsTlcs90::prepareCycle() {
    auto s = Signals::put();
    // C1L
    x1_lo();
    if (c1_lo_ns)
        delayNanoseconds(c1_lo_ns);
    s->getAddrHigh();
    // C2H
    x1_hi();
    s->getAddrLow();
    if (c2_hi_ns)
        delayNanoseconds(c2_hi_ns);
    // C2L
    x1_lo();
    s->getDirection();

    return s;
}

Signals *PinsTlcs90::completeCycle(Signals *s) {
    // C3H
    x1_hi();
    if (s->read()) {
        if (s->readMemory()) {
            s->data = _mems->read(s->addr);
        }
        // C3L
        s->setData();
        if (c3_lo_read)
            delayNanoseconds(c3_hi_read);
        x1_lo();
        Signals::outputMode();
        if (c3_lo_read)
            delayNanoseconds(c3_lo_read);
        // C4H
        x1_hi();
        delayNanoseconds(c4_hi_read);
        // C4L
        x1_lo();
        Signals::nextCycle();
        if (c4_lo_read)
            delayNanoseconds(c4_lo_read);
    } else if (s->write()) {
        delayNanoseconds(c3_hi_write);
        // C3L
        x1_lo();
        if (c3_lo_write)
            delayNanoseconds(c3_lo_write);
        s->getData();
        // C4H
        x1_hi();
        if (s->writeMemory()) {
            _mems->write(s->addr, s->data);
            if (c4_hi_write)
                delayNanoseconds(c4_hi_write);
        } else {
            delayNanoseconds(c4_hi_capture);
        }
        // C4L
        x1_lo();
        Signals::nextCycle();
        if (c4_lo_write)
            delayNanoseconds(c4_lo_write);
    } else {
        delayNanoseconds(c3_hi_nobus);
        // C3L, C4H
        x1_cycle();
        // C4L
        x1_lo();
        delayNanoseconds(c4_lo_nobus);
    }
    // C1H
    x1_hi();
    Signals::inputMode();

    return s;
}

void PinsTlcs90::execInst(const uint8_t *inst, uint8_t len) {
    execute(inst, len, nullptr, 0, nullptr);
}

void PinsTlcs90::captureWrites(const uint8_t *inst, uint8_t len, uint8_t *buf,
        uint8_t max, uint16_t *addr) {
    execute(inst, len, buf, max, addr);
}

void PinsTlcs90::execute(const uint8_t *inst, uint8_t len, uint8_t *buf,
        uint8_t max, uint16_t *addr) {
    negate_wait();
    uint8_t inj = 0;
    uint8_t cap = 0;
    auto s = Signals::current();
    while (inj < len || cap < max) {
        if (cap < max)
            s->capture();
        if (inj < len)
            s->inject(inst[inj]);
        completeCycle(s);
        if (s->read()) {
            if (inj == 0 && addr)
                *addr = s->addr;
            if (inj < len)
                ++inj;
        } else if (s->write()) {
            if (buf && cap < max)
                buf[cap++] = s->data;
        }
        s = prepareCycle();
    }
    while (!s->read() && !s->write()) {
        completeCycle(s);
        s = prepareCycle();
    }
    assert_wait();
}

void PinsTlcs90::idle() {
    // TLCS90 is fully static and stop at #WAIT=LOW
}

void PinsTlcs90::loop() {
    auto s = Signals::current();
    while (true) {
        _devs->loop();
        if (s->addr == InstTlcs90::ORG_SWI) {
            auto r = regs<RegsTlcs90>();
            if (r->saveContext(s->prev(4))) {
                // SWI; break point or halt to system (HALT at ORG_SWI))
                const auto opc = _mems->read_byte(s->addr);
                const auto pc = r->nextIp() - 1;  // offset SWI
                if (opc == InstTlcs90::HALT || isBreakPoint(pc)) {
                    r->saveRegisters();
                    r->setIp(pc);
                    assert_wait();
                    Signals::discard(s->prev(6));
                    break;
                }
            }
        }
        completeCycle(s);
        s = prepareCycle();
        if (haltSwitch()) {
            suspend(true);
            break;
        }
    }
    disassembleCycles();
}

void PinsTlcs90::run() {
    _regs->restore();
    Signals::resetCycles();
    saveBreakInsts();
    negate_wait();
    loop();
    restoreBreakInsts();
    disassembleCycles();
}

void PinsTlcs90::suspend(bool show) {
    negate_wait();
    // Execute at least one cycle before asserting #NMI
    auto s = Signals::current();
    completeCycle(s);
    s = prepareCycle();
    assert_nmi();
    while (true) {
        // Interrupt; 0:n:d:d:V:d:W:W:W:W:V
        if (s->addr == InstTlcs90::ORG_NMI || s->addr == InstTlcs90::ORG_SWI) {
            negate_nmi();
            auto r = regs<RegsTlcs90>();
            if (r->saveContext(s->prev(4))) {
                r->saveRegisters();
                if (s->addr == InstTlcs90::ORG_SWI) {
                    r->setIp(s->addr);
                    r->offsetStack(-4);
                }
                break;
            }
        }
        completeCycle(s);
        s = prepareCycle();
    }
    if (s->addr == InstTlcs90::ORG_NMI && show)
        Signals::discard(s->prev(6));
}

bool PinsTlcs90::step(bool show) {
    Signals::resetCycles();
    const auto opc = _mems->read(_regs->nextIp());
    if (opc == InstTlcs90::HALT || !InstTlcs90::valid(_regs->nextIp(), _mems)) {
        // HALT or unknown instruction. Just return.
        return false;
    }
    _regs->restore();
    if (show)
        Signals::resetCycles();
    suspend(show);
    if (show)
        printCycles();
    return true;
}

void PinsTlcs90::assertInt(uint8_t name) {
    if (name == INTR_INT0)
        assert_int0();
    if (name == INTR_INT1)
        assert_int1();
}

void PinsTlcs90::negateInt(uint8_t name) {
    if (name == INTR_INT0)
        negate_int0();
    if (name == INTR_INT1)
        negate_int1();
}

void PinsTlcs90::setBreakInst(uint32_t addr) const {
    _mems->put_inst(addr, InstTlcs90::SWI);
}

void PinsTlcs90::printCycles(const Signals *end) {
    const auto g = Signals::get();
    const auto cycles = g->diff(end ? end : Signals::put());
    for (auto i = 0; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

bool PinsTlcs90::matchAll(Signals *begin, const Signals *end) {
    const auto cycles = begin->diff(end->prev());
    LOG_MATCH(cli.print("@@  matchAll: begin="));
    LOG_MATCH(begin->print());
    LOG_MATCH(cli.print("@@           cycles="));
    LOG_MATCH(cli.printlnDec(cycles));
    Signals *prefetch = nullptr;
    for (auto i = 0; i < cycles;) {
        idle();
        const auto s = begin->next(i);
        if (prefetch == s)
            prefetch = nullptr;
        InstTlcs90 inst;
        if (inst.match(s, end, prefetch)) {
            const auto f = prefetch ? prefetch : s;
            f->markFetch(inst.matched());
            prefetch = s->next(inst.nextInstruction());
            i += inst.matched();
            continue;
        }
        return false;
    }
    return true;
}

const Signals *PinsTlcs90::findFetch(Signals *begin, const Signals *end) {
    const auto cycles = begin->diff(end);
    const auto limit = cycles >= 14 ? 14 : cycles;
    LOG_MATCH(cli.print("@@ findFetch: begin="));
    LOG_MATCH(begin->print());
    LOG_MATCH(cli.print("@@              end="));
    LOG_MATCH(end->print());
    for (auto i = 0; i < limit; ++i) {
        for (auto j = i; j < cycles; ++j)
            begin->next(j)->clearFetch();
        auto s = begin->next(i);
        if (matchAll(s, end))
            return s;
        idle();
    }
    for (auto j = 0; j < cycles; ++j)
        begin->next(j)->clearFetch();
    return end;
}

void PinsTlcs90::disassembleCycles() {
    const auto end = Signals::put();
    const auto begin = findFetch(Signals::get(), end);
    printCycles(begin);
    const auto cycles = begin->diff(end);
    const Signals *pref = nullptr;
    for (auto i = 0; i < cycles;) {
        const auto s = begin->next(i);
        if (pref || s->fetch()) {
            const auto f = pref ? pref : s;
            const auto nexti = _mems->disassemble(f->addr, 1);
            const auto len = nexti - f->addr - (pref ? 1 : 0);
            const auto matched = f->matched();
            pref = nullptr;
            for (auto j = len; j < matched; ++j) {
                const auto p = s->next(j);
                p->print();
                if (p->fetch())
                    pref = p;
            }
            i += matched;
        } else {
            s->print();
            ++i;
        }
        idle();
    }
}

}  // namespace tlcs90
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
