#include "pins_ins8070.h"
#include "debugger.h"
#include "devs_ins8070.h"
#include "mems_ins8070.h"
#include "regs_ins8070.h"
#include "signals_ins8070.h"

namespace debugger {
namespace ins8070 {

// clang-format off
/**
 * INS8070 External Bus cycle
 *       __    __    __    __    __    __    __    __    __    __
 *  XOUT   |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__
 *         __    __    __    __    __    __    __    __    __    __
 *   XIN _|  |__| 1|__| 2|__| 3|__| 4|__| 5|__| 6|__| 7|__| 8|__|  |
 *       ________\        \     \     \                 \     \ ____
 * #BREQ         |_____________________________________________|
 *                          _________________________________
 *  ADDR-------------------<_________________________________>------
 *       ________________________                         __________
 *  #RDS                         |______________________r|r
 *       _____________________________                    __________
 *  #WDS                              |wwwwwwwwwwwwwwwwww|
 *
 */
// clang-format on

namespace {

constexpr auto xin_hi_ns = 105;       // 120
constexpr auto xin_lo_ns = 98;        // 120
constexpr auto xin_hi_breq = 90;      // 120
constexpr auto xin_lo_breq = 82;      // 120
constexpr auto xin_hi_dir = 100;      // 120
constexpr auto xin_lo_dir = 52;       // 120
constexpr auto xin_lo_addr = 56;      // 120
constexpr auto xin_write_begin = 68;  // 120
constexpr auto xin_hi_write = 66;     // 120
constexpr auto xin_hi_wds = 100;      // 120
constexpr auto xin_lo_wds = 64;       // 120
constexpr auto xin_lo_capture = 54;   // 120
constexpr auto xin_write_end = 52;    // 120
constexpr auto xin_read_begin = 62;   // 120
constexpr auto xin_hi_read = 20;      // 120
constexpr auto xin_hi_inject = 92;    // 120
constexpr auto xin_read_out = 2;      // 120
constexpr auto xin_hi_rds = 100;      // 120
constexpr auto xin_lo_rds = 64;       // 120
constexpr auto xin_read_end = 30;     // 120
constexpr auto xin_lo_input = 14;     // 120
constexpr auto xin_breq_end = 50;     // 120

inline auto signal_breq() {
    return digitalReadFast(PIN_BREQ);
}

inline auto signal_enout() {
    return digitalReadFast(PIN_ENOUT);
}

inline auto signal_wds() {
    return digitalReadFast(PIN_WDS);
}

inline auto signal_rds() {
    return digitalReadFast(PIN_RDS);
}

void assert_sa() {
    digitalWriteFast(PIN_SA, LOW);
}

void negate_sa() {
    digitalWriteFast(PIN_SA, HIGH);
}

void assert_sb() __attribute__((unused));
void assert_sb() {
    digitalWriteFast(PIN_SB, LOW);
}

void assert_enin() {
    digitalWriteFast(PIN_ENIN, LOW);
}

void negate_enin() {
    digitalWriteFast(PIN_ENIN, HIGH);
}

void negate_reset() {
    digitalWriteFast(PIN_RST, HIGH);
}

constexpr uint8_t PINS_LOW[] = {
        PIN_XIN,
        PIN_RST,
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_SA,
        PIN_SB,
        PIN_ENIN,
        PIN_HOLD,
};

constexpr uint8_t PINS_PULLUP[] = {
        PIN_BREQ,
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
        PIN_RDS,
        PIN_WDS,
        PIN_ENOUT,
        PIN_F1,
        PIN_F2,
        PIN_F3,
};

inline void xin_hi() {
    digitalWriteFast(PIN_XIN, HIGH);
}

inline void xin_cycle() {
    digitalWriteFast(PIN_XIN, HIGH);
    delayNanoseconds(xin_hi_ns);
    digitalWriteFast(PIN_XIN, LOW);
    delayNanoseconds(xin_lo_ns);
}

}  // namespace

PinsIns8070::PinsIns8070() {
    auto regs = new RegsIns8070(this);
    _regs = regs;
    _devs = new DevsIns8070();
    _mems = new MemsIns8070(regs, _devs);
}

void PinsIns8070::xin_lo() const {
    digitalWriteFast(PIN_XIN, LOW);
    devs<DevsIns8070>()->sci()->loop();
}

void PinsIns8070::resetPins() {
    // Assert reset condition
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_PULLUP, sizeof(PINS_PULLUP), INPUT_PULLUP);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    // #RST must remain low is 8 Tc.
    for (auto i = 0; i < 4 * 8 * 2; i++)
        xin_cycle();
    negate_reset();
    negate_enin();
    Signals::resetCycles();
    // The first instruction will be fetched within 13 Tc after #RST
    // has gone high.
    _regs->save();
}

Signals *PinsIns8070::prepareCycle() {
    auto s = Signals::put();
    // XIN=H
    goto check_breq;
    while (true) {
    check_breq:
        xin_hi();
        delayNanoseconds(xin_hi_breq);
        if (signal_breq() == LOW)
            goto check_dir;
        xin_lo();
        delayNanoseconds(xin_lo_breq);
    }
    // XIN=H, #BREQ=LOW
    while (true) {
        xin_hi();
        delayNanoseconds(xin_hi_dir);
    check_dir:
        xin_lo();
        if (s->getDirection())
            break;
        delayNanoseconds(xin_lo_dir);
    }
    delayNanoseconds(xin_lo_addr);
    xin_hi();
    s->getAddr();
    return s;
}

Signals *PinsIns8070::completeCycle(Signals *s) {
    // XIN=H
    xin_lo();
    if (s->write()) {
        delayNanoseconds(xin_write_begin);
        xin_hi();
        s->getData();
        delayNanoseconds(xin_hi_write);
        xin_lo();
        if (s->writeMemory()) {
            _mems->write(s->addr, s->data);
        } else {
            delayNanoseconds(xin_lo_capture);
        }
        while (signal_wds() == LOW) {
            xin_hi();
            delayNanoseconds(xin_hi_wds);
            xin_lo();
            delayNanoseconds(xin_lo_wds);
        }
        xin_hi();
        Signals::nextCycle();
        delayNanoseconds(xin_write_end);
    } else {
        delayNanoseconds(xin_read_begin);
        xin_hi();
        if (s->readMemory()) {
            s->data = _mems->read(s->addr);
            delayNanoseconds(xin_hi_read);
        } else {
            delayNanoseconds(xin_hi_inject);
        }
        xin_lo();
        s->outData();
        if (xin_read_out)
            delayNanoseconds(xin_read_out);
        while (signal_rds() == LOW) {
            xin_hi();
            delayNanoseconds(xin_hi_rds);
            xin_lo();
            delayNanoseconds(xin_lo_rds);
        }
        xin_hi();
        Signals::nextCycle();
        delayNanoseconds(xin_read_end);
    }
    xin_lo();
    Signals::inputMode();
    delayNanoseconds(xin_lo_input);
    // Read-Modify-Write bus cycle keeps #BREQ low.
    for (auto i = 0; i < 3; i++) {
        if (signal_breq() != LOW)
            break;
        xin_hi();
        delayNanoseconds(xin_hi_breq);
        xin_lo();
        delayNanoseconds(xin_breq_end);
    }
    xin_hi();
    // XIN=H
    return s;
}

Signals *PinsIns8070::cycle() {
    return completeCycle(prepareCycle());
}

Signals *PinsIns8070::cycle(uint8_t data) {
    return completeCycle(prepareCycle()->inject(data));
}

void PinsIns8070::execInst(const uint8_t *inst, uint8_t len) {
    execute(inst, len, nullptr, nullptr, 0);
}

void PinsIns8070::captureWrites(const uint8_t *inst, uint8_t len,
        uint16_t *addr, uint8_t *buf, uint8_t max) {
    execute(inst, len, addr, buf, max);
}

void PinsIns8070::execute(const uint8_t *inst, uint8_t len, uint16_t *addr,
        uint8_t *buf, uint8_t max) {
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
        if (s->read() && inj < len) {
            ++inj;
        } else if (s->write()) {
            if (cap == 0 && addr)
                *addr = s->addr;
            if (buf && cap < max)
                buf[cap++] = s->data;
        }
    }
    negate_enin();
}

void PinsIns8070::idle() {
    // #ENIN is HIGH and bus cycle is suspened.
    xin_cycle();
}

const Signals *PinsIns8070::isCall15(const Signals *vector) const {
    if (vector->read()) {
        const auto call = vector->prev(3);
        if (call->read() && call->data == InstIns8070::CALL15) {
            const auto push = call->next();
            if (push->write() && push->next()->write()) {
                const auto vec15 = _mems->raw_read16(InstIns8070::VEC_CALL15);
                if (isBreakPoint(call->addr) || vec15 == 0)
                    return call;
            }
        }
    }
    return nullptr;
}

void PinsIns8070::loop() {
    while (true) {
        _devs->loop();
        auto s = prepareCycle();
        if (s->addr == InstIns8070::VEC_CALL15) {
            const auto call = isCall15(s);
            if (call) {
                const uint16_t pc = call->addr - 1;
                cycle(0);                 // inject low vector
                cycle(0);                 // inject high vector
                cycle(InstIns8070::RET);  // cancel CALL 15
                cycle(lo(pc));            // inject low address
                cycle(hi(pc));            // inject high address
                Signals::discard(call);
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

void PinsIns8070::suspend() {
    while (true) {
        auto s = prepareCycle();
        if (s->fetch()) {
            completeCycle(s->inject(InstIns8070::BRA));
            cycle(InstIns8070::BRA_HERE);
            Signals::discard(s);
            return;
        }
        completeCycle(s);
    }
}

void PinsIns8070::run() {
    _regs->restore();
    Signals::resetCycles();
    saveBreakInsts();
    assert_enin();
    loop();
    negate_enin();
    restoreBreakInsts();
    disassembleCycles();
    _regs->save();
}

uint8_t PinsIns8070::busCycles(InstIns8070 &inst) const {
    auto pc = _regs->nextIp();
    const auto opc = _mems->raw_read(pc);
    if (!inst.get(opc))
        return 0;
    const auto opr = _mems->raw_read(pc + 1);
    const auto ea = regs<RegsIns8070>()->effectiveAddr(inst, opr);
    return MemsIns8070::is_internal(ea) ? inst.externalCycles()
                                        : inst.busCycles();
}

bool PinsIns8070::step(bool show) {
    Signals::resetCycles();
    InstIns8070 inst;
    const auto cycles = busCycles(inst);
    if (cycles == 0)
        return false;
    if (inst.opc == InstIns8070::CALL15 &&
            _mems->raw_read16(InstIns8070::VEC_CALL15) == 0)
        return false;
    _regs->restore();
    if (show)
        Signals::resetCycles();
    assert_enin();
    if (inst.addrMode() == M_SSM) {
        // SSM instruction
        const auto addr = cycle()->addr;
        for (auto n = 0; n < 256; ++n) {
            auto s = prepareCycle();
            if (s->addr == addr + 3) {
                completeCycle(s->inject(InstIns8070::BRA));
                cycle(InstIns8070::BRA_HERE);
                if (show)
                    Signals::discard(s);
                break;
            }
            completeCycle(s);
        }
    } else {
        for (auto c = 0; c < cycles; ++c) {
            cycle();
        }
    }
    if (show)
        printCycles();
    _regs->save();
    return true;
}

void PinsIns8070::assertInt(uint8_t name) {
    // #INTA is negative-edge sensed.
    assert_sa();
}

void PinsIns8070::negateInt(uint8_t name) {
    negate_sa();
}

void PinsIns8070::setBreakInst(uint32_t addr) const {
    _mems->put_inst(addr, InstIns8070::CALL15);
}

void PinsIns8070::printCycles(const Signals *end) {
    const auto g = Signals::get();
    const auto cycles = g->diff(end ? end : Signals::put());
    for (auto i = 0; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

bool PinsIns8070::matchAll(Signals *begin, const Signals *end) {
    const auto cycles = begin->diff(end);
    LOG_MATCH(cli.print("@@  matchAll: begin="));
    LOG_MATCH(begin->print());
    LOG_MATCH(cli.print("@@           cycles="));
    LOG_MATCH(cli.printlnDec(cycles));
    for (auto i = 0; i < cycles;) {
        idle();
        auto s = begin->next(i);
        InstIns8070 inst;
        if (inst.match(s, end->next())) {
            s->markFetch(true);
            i += inst.matchedCycles();
            continue;
        }
        return false;
    }
    return true;
}
const Signals *PinsIns8070::findFetch(Signals *begin, const Signals *end) {
    const auto cycles = begin->diff(end);
    LOG_MATCH(cli.print("@@ findFetch: begin="));
    LOG_MATCH(begin->print());
    LOG_MATCH(cli.print("@@              end="));
    LOG_MATCH(end->print());
    for (auto i = 0; i < cycles; ++i) {
        idle();
        auto s = begin->next(i);
        if (matchAll(s, end))
            return s;
        for (auto j = i; j < cycles; ++j)
            begin->next(j)->markFetch(false);
    }
    return end;
}

void PinsIns8070::disassembleCycles() {
    const auto end = Signals::put();
    const auto begin = findFetch(Signals::get(), end);
    printCycles(begin);
    const auto cycles = begin->diff(end);
    for (auto i = 0; i < cycles;) {
        const auto s = begin->next(i);
        if (s->fetchMark()) {
            const auto len = _mems->disassemble(s->addr, 1) - s->addr;
            if (InstIns8070::isJsr(s->data)) {
                s->next(2)->print();
                s->next(3)->print();
                i += 5;
            } else {
                i += len;
            }
        } else {
            s->print();
            ++i;
        }
        idle();
    }
}

}  // namespace ins8070
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
