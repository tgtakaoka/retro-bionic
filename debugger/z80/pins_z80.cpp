#include "pins_z80.h"
#include "debugger.h"
#include "devs_z80.h"
#include "inst_z80.h"
#include "mems_z80.h"
#include "regs_z80.h"
#include "signals_z80.h"

namespace debugger {
namespace z80 {

// clang-format off
/**
 * Z80 bus cycle.
 *          |   T1  |   T2  |   T3  |   T4  |   T1  |   T2  |   T3  |   T1  |   T2  |   T3  |
 *          |___    |___    |___    |___    |___    |___    |___    |___    |___    |___    |___
 *  CLK  ___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|
 *       ___\ __\___________\ __\___________\ __\_______________\____\__\_______\___________\ __
 * ADDR  ____X___|___________X___|___________X___|_______________|___X___|_______|___________X__
 *       ____|   |           |___|___________|___|_______________|_______|_______|___________|__
 *  #M1      |___|___________|   |           |   |               |       |       |           |
 *       ________|           |___|           |___|               |_______|       |           |__
 * #MREQ         |___________|   |___________|   |_______________|       |_______|___________|
 *       ________|___________|               |___|_______________|_______________|___________|__
 * #REFS         |           |_______________|   |               |               |           |
 *       ________|           |___________________|               |_______________|___________|__
 *  #RD          |___________|                   |_______________|               |           |
 *       ________________________________________________________________________|           |__
 *  #WR                                                                          |___________|
 *                          __                                  __          __________________
 * DATA  ------------------<__>--------------------------------<__>--------<__________________>--
 *
 *          |   T1  |   T2  |   TW  |   TW  |   T3  |   T4  |   T1  |   T2  |   TW  |   T3  |
 *          |___    |___    |___    |___    |___    |___    |___    |___    |___    |___    |___
 *  CLK  ___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|
 *       ___\___________________\___________\ __\___________\_______\___________________\____\__
 * ADDR  ____X___________________|___________X___|___________X_______|___________________|___X__
 *       ____|                   |           |___|___________|_______|___________________|______
 *  #M1      |___________________|___________|   |           |       |                   |
 *       ________________________|___________|___|           |_______|___________________|______
 * #REFS                         |           |   |___________|       |                   |
 *       ________________________|___________|___|           |_______|___________________|______
 * #MREQ                         |           |   |___________|       |                   |
 *       ________________________|           |_______________________|                   |______
 * #IORQ                         |___________|                       |___________________|
 *       ____________________________________________________________|                   |______
 *  #RD                                                              |___________________|
 *       ____________________________________________________________|                   |______
 *  #WR                                                              |___________________|
 *                                          __                     ____________________ __
 * DATA  ----------------------------------<__>-------------------<____________________<__>-----
 */
// clang-format on

namespace {

//         TcC: min 250 ns; CLK cycle time
//     TdCr(A): max 110 ns; Address valid from CLK+
//   TdCr(M1f): max 100 ns; CLK+ to #M1-
//   TdCr(M1r): max 100 ns; CLK+ to #M1+
// TdCf(MREQf): max  85 ns; CLK- to #MREQ-
// TdCr(MREQr): max  85 ns; CLK+ to #MREQ+
//   TdCf(RDf): max  95 ns; CLK- to #RD-
//   TdCr(RDr): max  95 ns; CLK+ to #RD+
//     TsD(Cr): min  35 ns; Data setup to CLK+
// TdCr(RFSHf): max 130 ns; CLK+ to #RFSH-
//     TwMREQh: min 110 ns; #MREQ pulse width HIGH
// TdCr(RFSHr): max 120 ns; CLK+ to #RFSH+
//   TdCf(WRf): max  80 ns; CLK- to #WR-
//   TdCf(WRr): max  80 ns; CLK- to #WR+
//     TdCf(D): max 150 ns; CLK- to data valid
// TdCf(IORQf): max  85 ns; CLK- to #IORQ-
// TdCf(IORQr): max  85 ns; CLK- to #IORQ+

constexpr auto clk_hi_ns = 100;      // 125 ns
constexpr auto clk_lo_ns = 100;      // 125 ns
constexpr auto clk_lo_addr = 40;     // +clk_lo_cntl; 125 ns
constexpr auto clk_hi_addr = 55;     // 125 ns
constexpr auto clk_lo_cntl = 28;     // +clk_lo_addr; 125 ns
constexpr auto clk_hi_read = 80;     // TdCr(M1f)
constexpr auto clk_hi_inject = 80;   // TdCf(MREQf)/TdCf(IORQf)
constexpr auto clk_lo_read = 43;     // 125 ns
constexpr auto clk_hi_next = 46;     // 125 ns
constexpr auto clk_hi_write = 34;    // 125 ns
constexpr auto clk_lo_get = 78;      // 125 ns
constexpr auto clk_hi_memory = 79;   // 125 ns
constexpr auto clk_hi_capture = 94;  // 125 ns
constexpr auto clk_hi_io = 85;       // 125 ns
constexpr auto clk_lo_input = 1;     // 125 ns
constexpr auto clk_lo_refresh = 52;  // 125 ns
constexpr auto clk_lo_exec = 6;      // 125 ns

inline void clk_hi() {
    digitalWriteFast(PIN_CLK, HIGH);
}

inline void clk_lo() {
    digitalWriteFast(PIN_CLK, LOW);
}

inline void assert_nmi() {
    digitalWriteFast(PIN_NMI, LOW);
}

inline void negate_nmi() {
    digitalWriteFast(PIN_NMI, HIGH);
}

inline void assert_int() {
    digitalWriteFast(PIN_INT, LOW);
}

inline void negate_int() {
    digitalWriteFast(PIN_INT, HIGH);
}

inline auto signal_rd() {
    return digitalReadFast(PIN_RD);
}

inline auto signal_wr() {
    return digitalReadFast(PIN_WR);
}

inline auto signal_m1() {
    return digitalReadFast(PIN_M1);
}

inline auto signal_mreq() {
    return digitalReadFast(PIN_MREQ);
}

inline auto signal_iorq() {
    return digitalReadFast(PIN_IORQ);
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
        PIN_CLK,
        PIN_RESET,
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_HALT,
        PIN_BUSREQ,
        PIN_WAIT,
        PIN_INT,
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
        PIN_AM8,
        PIN_AM9,
        PIN_AM10,
        PIN_AM11,
        PIN_AH12,
        PIN_AH13,
        PIN_AH14,
        PIN_AH15,
        PIN_M1,
        PIN_MREQ,
        PIN_IORQ,
        PIN_RD,
        PIN_WR,
        PIN_RFSH,
        PIN_BUSACK,
};

inline void clk_cycle_lo() {
    clk_hi();
    delayNanoseconds(clk_hi_ns);
    clk_lo();
}

inline void clk_cycle() {
    clk_cycle_lo();
    delayNanoseconds(clk_lo_ns);
}

}  // namespace

void PinsZ80::resetPins() {
    // Assert reset condition
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    // #RESET must be active for at least three clock cycles for CPU
    // to properly accept it.
    for (auto i = 0; i < 5; i++)
        clk_cycle();
    negate_reset();
    // Once #RESET goes inactive, two internal T cycles are consumed
    // before the CPU resumes normal processing operation.
    clk_cycle();
    clk_cycle();
    Signals::resetCycles();
    prepareWait();
    _regs->setIp(InstZ80::ORG_RESET);
    _regs->save();
}

Signals *PinsZ80::prepareCycle() const {
    const auto s = Signals::put();
    do {
        delayNanoseconds(clk_lo_addr);
        // TnH
        clk_hi();
        delayNanoseconds(clk_hi_addr);
        s->getAddress();
        // TnL
        clk_lo();
        delayNanoseconds(clk_lo_cntl);
        s->getControl();
    } while (s->nobus());
    // #MREQ:T1L, #IORQ:T2L/Twa1L
    return s;
}

Signals *PinsZ80::completeCycle(Signals *s) const {
    // #MREQ:T2H, #IORQ:TwaH/Twa2H
    clk_hi();
    if (s->mreq()) {  // Memory read or write cycles
        if (s->read()) {
            if (s->readMemory()) {
                s->data = _mems->raw_read(s->addr);
                delayNanoseconds(clk_hi_read);
            } else {
                delayNanoseconds(clk_hi_inject);
            }
            // #MREQ:T2L
            clk_lo();
            s->outData();
            delayNanoseconds(clk_lo_read);
            // #MREQ:T3H
            clk_hi();
            Signals::nextCycle();
            delayNanoseconds(clk_hi_next);
        } else {
            delayNanoseconds(clk_hi_write);
            Signals::nextCycle();
            // #MREQ:T2L
            clk_lo();
            delayNanoseconds(clk_lo_get);
            s->getData();
            // #MREQ:T3H
            clk_hi();
            if (s->writeMemory()) {
                _mems->raw_write(s->addr, s->data);
                delayNanoseconds(clk_hi_memory);
            } else {
                delayNanoseconds(clk_hi_capture);
            }
        }
    } else {  // Input or output cycles
        delayNanoseconds(clk_hi_io);
        const uint8_t ioaddr = s->addr;
        // #IORQ:TwaL/Twa2L
        clk_lo();
        if (s->read()) {
            if (_devs->isSelected(ioaddr))
                s->data = _devs->read(ioaddr);
            s->outData();
        } else if (s->intack()) {  // Interrupt acknowledge cycle
            s->markRead();
            s->data = _devs->vector();
            s->outData();
        } else {
            s->getData();
            if (_devs->isSelected(ioaddr))
                _devs->write(ioaddr, s->data);
        }
        // T3H
        clk_hi();
        Signals::nextCycle();
        delayNanoseconds(clk_hi_next);
    }
    // T3L
    clk_lo();
    delayNanoseconds(clk_lo_input);
    s->inputMode();
    if (s->m1()) {
        delayNanoseconds(clk_lo_refresh);
        // T4H
        clk_hi();
        delayNanoseconds(clk_hi_ns);
        // T4L
        clk_lo();
    }
    return s;
}

Signals *PinsZ80::prepareWait() const {
    assert_wait();
    const auto s = prepareCycle();
    // #MREQ:T2, #IORQ:Twa/Twa2
    clk_cycle();
    return s;
}

Signals *PinsZ80::resumeCycle(uint16_t pc) const {
    const auto s = Signals::put();
    negate_wait();
    s->setAddress(pc);
    s->getControl();
    return completeCycle(s);
}

Signals *PinsZ80::inject(uint8_t data) const {
    return completeCycle(prepareCycle()->inject(data));
}

void PinsZ80::execute(const uint8_t *inst, uint8_t len, uint16_t *addr,
        uint8_t *buf, uint8_t max) {
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
        if (s->mreq()) {
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
        if (inj >= len && cap >= max) {
            prepareWait();
            return;
        }
        delayNanoseconds(clk_lo_exec);
        s = prepareCycle();
    }
}

void PinsZ80::idle() {
    // #WAIT=L
    clk_cycle_lo();
}

void PinsZ80::loop() {
    resumeCycle(_regs->nextIp());
    while (true) {
        const auto s = prepareCycle();
        if (s->fetch() && _mems->raw_read(s->addr) == InstZ80::HALT) {
            completeCycle(s->inject(InstZ80::JR));
            inject(InstZ80::JR_HERE);
            prepareWait();
            return;
        }
        completeCycle(s);
        _devs->loop();
        if (haltSwitch()) {
            suspend();
            return;
        }
    }
}

void PinsZ80::run() {
    _regs->restore();
    Signals::resetCycles();
    saveBreakInsts();
    loop();
    restoreBreakInsts();
    disassembleCycles();
    _regs->save();
}

void PinsZ80::suspend() {
    assert_nmi();
    while (true) {
        const auto s = prepareCycle();
        if (s->fetch() && s->addr == InstZ80::ORG_NMI) {
            negate_nmi();
            completeCycle(s->inject(InstZ80::RETN_PREFIX));
            inject(InstZ80::RETN);
            inject(s->prev()->data);
            inject(s->prev(2)->data);
            Signals::discard(s->prev(3));
            prepareWait();
            return;
        }
        completeCycle(s);
    }
}

bool PinsZ80::rawStep() {
    const auto pc = _regs->nextIp();
    if (_mems->raw_read(pc) == InstZ80::HALT)
        return false;
    assert_nmi();
    resumeCycle(pc);
    suspend();
    return true;
}

bool PinsZ80::step(bool show) {
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

void PinsZ80::assertInt(uint8_t name) {
    assert_int();
}

void PinsZ80::negateInt(uint8_t name) {
    negate_int();
}

void PinsZ80::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

void PinsZ80::disassembleCycles() {
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

}  // namespace z80
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
