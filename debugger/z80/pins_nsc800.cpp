#include "pins_nsc800.h"
#include "debugger.h"
#include "devs_z80.h"
#include "inst_z80.h"
#include "mems_z80.h"
#include "regs_z80.h"
#include "signals_nsc800.h"

namespace debugger {
namespace nsc800 {

using z80::InstZ80;

// clang-format off
/**
 * NSC800 Opecode fetch
 *           |----T1-----|----T2-----|----T3-----|----T4-----|----T1-----|
 *           |-T1A-|-T1B-|-T2A-|-T2B-|-T3A-|-T3B-|-T4A-|-T4B-|-T21-|-T1B-|
 *         __    __    __    __    __    __    __    __    __    __    __
 *    X1 _|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |
 *           \ ____\     \ ____\     \ ____\     \ ____\     \ ____\     \
 *   CLK _____|    T|1____|    T|2____|    T|3____|    T|4____|    T|1____
 *            \ ____\     \     |     \ ____\     \           \ ____\
 *   ALE ______|     |_____v____v______|     |_____v___________|     |____
 * AH/Sx ______\ __________|___________\ __________|___________\ _________
 * #IO/M _______X_________AH____________X________regI___________X_________
 *       _______|________  |          __|__________|            |________
 *    AD _______X___AL___>-----------<inX___regR___>------------<___AL___>
 * #INTA __________________|            ________________________|_________
 *   #RD                   |___________|
 *       ______________________________\                        |_________
 * #RFSH                                |_______________________|
 *       _______________________v_________________________________________
 * #WAIT ______________________/ \________________________________________
 *
 * NSC800 Memory read write
 *           |----T1-----|----T2-----|----T3-----|----T1-----|
 *           |-T1A-|-T1B-|-T2A-|-T2B-|-T3A-|-T3B-|-T21-|-T1B-|
 *         __    __    __    __    __    __    __    __    __
 *    X1 _|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |
 *           \ ____\     \ ____\     \ ____\     \ ____\     \
 *   CLK _____|    T|1____|    T|2____|    T|3____|    T|1____
 *            \ ____\     \     \           \     \ ____\
 *   ALE ______|     |_____v____vv___________v____|      |____
 * AH/Sx _______ __________|_____|___________|____\ __________
 * #IO/M _______X__________________________________X__________
 *       _______|________  |     |          _|     |__________
 *    AD _______X___AL___>-------_---------<in>---<___________
 *       __________________|     |           |_____|__________
 *   #RD                   |_________________|
 *       _______ ________ _______|___________|_____|__________
 *    AD _______X___AL___X__________out____________X__________
 *       ________________________|           |________________
 *   #WR                         |___________|
 *       _______________________v_____________________________
 * #WAIT ______________________/ \____________________________
 */
// clang-format on

namespace {
//   tx: min 125 ns, max 3,333 ns; XIN period (~8MHz, NSC800-4)
// tCYC: min 320 ns, max 2,000 ns; CLK cycle period
// tXCF: min   5 ns, max    80 ns; XIN falling to CLK faling
// tXCR: min   5 ns, max    80 ns; XIN falling to CLK rising
//  tLL: min 140 ns              ; ALE width
//  tLA: min 100 ns              ; Address hold time after ALE
//  tAL: min 115 ns              ; Address valid before trailing ALE
//  tLC: min 130 ns              ; Trailing ALE to leading control
// tLCK: min 100 ns              ; ALE low during CLK high
//  tAC: min 270 ns              ; A8-15 balid to leading control
// tLDR: max 460 ns              ; ALE to valid data for read
// tLDW: max 200 ns              ; ALE to valid data for write
//  tRD: max 300 ns              ; #RD to valid data
//  tWD: min 100 ns              ; data valid after trailing #WR
//  tCC: min 400 ns              ; width of #RD/#WR/#INTA
//  tCA: min 120 ns              ; A8-15 valid after control
//  tCL: min  50 ns              ; trailing control to leading ALE

constexpr auto xin_lo_ns = 38;       // 62.5 ns
constexpr auto xin_hi_ns = 40;       // 62.5 ns
constexpr auto xin_lo_ale = 40;      // 62.5 ns
constexpr auto xin_lo_control = 35;  // 62.5 ns

inline void xin_hi() {
    digitalWriteFast(PIN_XIN, HIGH);
}

inline void xin_lo() {
    digitalWriteFast(PIN_XIN, LOW);
}

inline auto signal_clk() {
    return digitalReadFast(PIN_CLK);
}

inline void assert_nmi() {
    digitalWriteFast(PIN_NMI, LOW);
}

inline void negate_nmi() {
    digitalWriteFast(PIN_NMI, HIGH);
}

inline void assert_intr() {
    digitalWriteFast(PIN_INTR, LOW);
}

inline void negate_intr() {
    digitalWriteFast(PIN_INTR, HIGH);
}

inline auto signal_ale() {
    return digitalReadFast(PIN_ALE);
}

inline auto signal_rd() {
    return digitalReadFast(PIN_RD);
}

inline auto signal_wr() {
    return digitalReadFast(PIN_WR);
}

inline auto signal_inta() {
    return digitalReadFast(PIN_INTA);
}

inline void assert_wait() {
    digitalWriteFast(PIN_WAIT, LOW);
}

inline void negate_wait() {
    digitalWriteFast(PIN_WAIT, HIGH);
}

inline void negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
}

const uint8_t PINS_LOW[] = {
        PIN_XIN,
        PIN_RESET,
};

const uint8_t PINS_HIGH[] = {
        PIN_RSTC,
        PIN_RSTB,
        PIN_RSTA,
        PIN_INTR,
        PIN_NMI,
        PIN_PS,
        PIN_BREQ,
        PIN_WAIT,
};

const uint8_t PINS_INPUT[] = {
        PIN_AD0,
        PIN_AD1,
        PIN_AD2,
        PIN_AD3,
        PIN_AD4,
        PIN_AD5,
        PIN_AD6,
        PIN_AD7,
        PIN_ADDR8,
        PIN_ADDR9,
        PIN_ADDR10,
        PIN_ADDR11,
        PIN_ADDR12,
        PIN_ADDR13,
        PIN_ADDR14,
        PIN_ADDR15,
        PIN_S0,
        PIN_S1,
        PIN_ALE,
        PIN_IOM,
        PIN_BACK,
        PIN_RFSH,
        PIN_INTA,
        PIN_RD,
        PIN_WR,
        PIN_RESOUT,
};

void xin_cycle_lo() {
    xin_hi();
    delayNanoseconds(xin_hi_ns);
    xin_lo();
}

void xin_cycle() {
    xin_cycle_lo();
    delayNanoseconds(xin_lo_ns);
}

}  // namespace

PinsNsc800::PinsNsc800() : PinsZ80Base() {
    _regs = new z80::RegsZ80(this);
}

void PinsNsc800::resetPins() {
    // Assert reset condition
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    // #RESET_IN should be kept low for a minimum of three clock
    // #periods to ensure proper synchronization of the CPU.
    for (auto i = 0; i < 10; i++)
        xin_cycle();
    negate_reset();
    Signals::resetCycles();
    prepareWait();
    _regs->setIp(InstZ80::ORG_RESET);
    _regs->save();
}

Signals *PinsNsc800::prepareCycle() const {
    const auto s = Signals::put();
    do {
        while (signal_ale() == LOW) {
            xin_cycle_lo();
            delayNanoseconds(xin_lo_ale);
        }
        // T1AH
        xin_hi();
        s->getAddress();
        // T1BL
        xin_lo();
        delayNanoseconds(xin_lo_ns);
        // T2A
        xin_cycle();
        // T2B
        xin_cycle_lo();
        delayNanoseconds(xin_lo_control);
        s->getControl();
    } while (s->nobus());
    return s;
}

Signals *PinsNsc800::completeCycle(Signals *s) const {
    if (s->read()) {
        if (s->mreq()) {  // Memory access
            if (s->readMemory()) {
                s->data = _mems->read_byte(s->addr);
            } else {
                ;  // inject
            }
        } else {  // I/O access
            const uint8_t ioaddr = s->addr;
            if (_devs->isSelected(ioaddr))
                s->data = _devs->read(ioaddr);
        }
        s->outData();
    } else if (s->intack()) {
        s->data = _devs->vector();
        s->outData();
    } else {
        s->getData();
        if (s->mreq()) {
            if (s->writeMemory()) {
                _mems->write_byte(s->addr, s->data);
            } else {
                ;  // capture
            }
        } else if (s->write()) {
            const uint8_t ioaddr = s->addr;
            if (_devs->isSelected(ioaddr))
                _devs->write(ioaddr, s->data);
        }
    }
    // T3A
    xin_cycle_lo();
    Signals::nextCycle();
    return s;
}

Signals *PinsNsc800::prepareWait() const {
    assert_wait();
    const auto s = prepareCycle();
    return s;
}

Signals *PinsNsc800::resumeCycle(uint16_t pc) const {
    const auto s = Signals::put();
    negate_wait();
    xin_cycle();
    xin_cycle();
    s->getAddress();
    s->setAddress(pc);
    return completeCycle(s);
}

Signals *PinsNsc800::inject(uint8_t data) const {
    return completeCycle(prepareCycle()->inject(data));
}

uint16_t PinsNsc800::execute(
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
        if (s->mreq()) {
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
        if (inj >= len && cap >= max) {
            prepareWait();
            return addr;
        }
        s = prepareCycle();
    }
}

void PinsNsc800::idle() {
    // #WAIT=L
    xin_cycle();
    xin_cycle_lo();
}

void PinsNsc800::loop() {
    resumeCycle(_regs->nextIp());
    while (true) {
        const auto s = prepareCycle();
        if (s->fetch() && _mems->read_byte(s->addr) == InstZ80::HALT) {
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

void PinsNsc800::run() {
    _regs->restore();
    Signals::resetCycles();
    saveBreakInsts();
    loop();
    restoreBreakInsts();
    disassembleCycles();
    _regs->save();
}

void PinsNsc800::suspend() {
    assert_nmi();
    while (true) {
        const auto s = prepareCycle();
        if (s->fetch() && s->addr == InstZ80::ORG_NMI) {
            negate_nmi();
            completeCycle(s->inject(InstZ80::RETN_PREFIX));
            inject(InstZ80::RETN);
            inject(s->prev()->data);
            inject(s->prev(2)->data);
            Signals::discard(s->prev(2));
            prepareWait();
            return;
        }
        completeCycle(s);
    }
}

bool PinsNsc800::rawStep() {
    const auto pc = _regs->nextIp();
    if (_mems->read_byte(pc) == InstZ80::HALT)
        return false;
    assert_nmi();
    resumeCycle(pc);
    suspend();
    return true;
}

bool PinsNsc800::step(bool show) {
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

void PinsNsc800::assertInt(uint8_t name) {
    switch (name) {
    default:
        assert_intr();
        break;
    case INTR_RSTC:
        digitalWriteFast(PIN_RSTC, HIGH);
        break;
    case INTR_RSTB:
        digitalWriteFast(PIN_RSTB, HIGH);
        break;
    case INTR_RSTA:
        digitalWriteFast(PIN_RSTA, HIGH);
        break;
    case INTR_NONE:
        break;
    }
}

void PinsNsc800::negateInt(uint8_t name) {
    switch (name) {
    default:
        negate_intr();
        break;
    case INTR_RSTC:
        digitalWriteFast(PIN_RSTC, LOW);
        break;
    case INTR_RSTB:
        digitalWriteFast(PIN_RSTB, LOW);
        break;
    case INTR_RSTA:
        digitalWriteFast(PIN_RSTA, LOW);
        break;
    case INTR_NONE:
        break;
    }
}

void PinsNsc800::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

void PinsNsc800::disassembleCycles() {
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

}  // namespace nsc800
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
