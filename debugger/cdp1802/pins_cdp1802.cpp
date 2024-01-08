#include "pins_cdp1802.h"
#include "cdp1802_sci_handler.h"
#include "debugger.h"
#include "devs_cdp1802.h"
#include "digital_bus.h"
#include "inst_cdp1802.h"
#include "mems_cdp1802.h"
#include "regs_cdp1802.h"
#include "signals_cdp1802.h"

namespace debugger {
namespace cdp1802 {

struct PinsCdp1802 Pins;

/**
 * CDP1802 bus cycle.
 *      __    __    __    __    __    __    __    __    __    __
 * OSC1 7 |_c|0 |_c|1 |_c|2 |_c|3 |_c|4 |_c|5 |_c|6 |_c|7 |_c|0 |__
 *              |>> _____
 *  TPA ___________|  |>>|_________________________________________
 *                                               |>>______
 *  TPB ___________________________________________|   |>>|________
 *             ____________  _____________________________
 *   MA -------<HHHHHHHHHHHH><LLLLLLLLLLLLLLLLLLLLLLLLLLLLL>-------
 *      _____________                                      ________
 * #MRD            |>|____________________________________|
 *      __________________________________              ___________
 * #MWR                                 |>|____________|
 *       _ |_______________________________________________  _______
 *   SC  _><_______________________________________________><_______
 *
 *  BUS --------------WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW---------
 *                                                     v
 *      -------------------------------------------RRRRRRR---------
 *
 *  - c7 falling edge to SC valid is ~130ns (300~450ns).
 *  - c0 falling edge to TPA rising edge is ~100ns (200~300ns)
 *  - c1 rising edge to #MRD falling edge is ~
 *  - c1 falling edge to TPA falling edge is ~100ns (200~300ns)
 *  - BUS data is sampled at rising edge of c7 with hodling time 200ns.
 */

namespace {

constexpr auto clock_hi_ns = 120;
constexpr auto clock_lo_ns = 120;
constexpr auto c00_ns = 120;
constexpr auto c01_ns = 120;
constexpr auto c10_ns = 50;
constexpr auto c11_ns = 120;
constexpr auto c20_ns = 102;
constexpr auto c21_ns = 120;
constexpr auto c30_ns = 120;
constexpr auto c31_ns = 120;
constexpr auto c40_ns = 82;
constexpr auto c41_ns = 120;
constexpr auto c50_ns = 120;
constexpr auto c51_ns = 74;
constexpr auto c60_read = 66;
constexpr auto c61_read = 75;
constexpr auto c70_read = 112;
constexpr auto c60_write = 92;
constexpr auto c61_write = 100;
constexpr auto c70_write = 78;
constexpr auto c60_nobus = 88;
constexpr auto c61_nobus = 96;
constexpr auto c70_nobus = 120;
constexpr auto c71_ns = 94;

inline void clock_hi() __attribute__((always_inline));
inline void clock_hi() {
    digitalWriteFast(PIN_CLOCK, HIGH);
}

inline void clock_lo() __attribute__((always_inline));
inline void clock_lo() {
    digitalWriteFast(PIN_CLOCK, LOW);
}

void assert_intr() {
    digitalWriteFast(PIN_INTR, LOW);
}

void negate_intr() {
    digitalWriteFast(PIN_INTR, HIGH);
}

inline uint8_t signal_tpa() __attribute__((always_inline));
inline uint8_t signal_tpa() {
    return digitalReadFast(PIN_TPA);
}

void assert_wait() __attribute__((unused));
void assert_wait() {
    digitalWriteFast(PIN_WAIT, LOW);
}

void negate_wait() {
    digitalWriteFast(PIN_WAIT, HIGH);
}

void assert_reset() {
    // Drive RESET condition
    digitalWriteFast(PIN_CLEAR, LOW);
    negate_wait();
    negate_intr();
    digitalWriteFast(PIN_DMAIN, HIGH);
    digitalWriteFast(PIN_DMAOUT, HIGH);
    clock_lo();
}

void negate_reset() {
    digitalWriteFast(PIN_CLEAR, HIGH);
}

const uint8_t PINS_LOW[] = {
        PIN_CLEAR,
        PIN_CLOCK,
};

const uint8_t PINS_HIGH[] = {
        PIN_INTR,
        PIN_WAIT,
        PIN_DMAIN,
        PIN_DMAOUT,
        PIN_EF1,
        PIN_EF2,
        PIN_EF3,
        PIN_EF4,
};

const uint8_t PINS_INPUT[] = {
        PIN_DBUS0,
        PIN_DBUS1,
        PIN_DBUS2,
        PIN_DBUS3,
        PIN_DBUS4,
        PIN_DBUS5,
        PIN_DBUS6,
        PIN_DBUS7,
        PIN_MA0,
        PIN_MA1,
        PIN_MA2,
        PIN_MA3,
        PIN_MA4,
        PIN_MA5,
        PIN_MA6,
        PIN_MA7,
        PIN_Q,
        PIN_TPA,
        PIN_TPB,
        PIN_SC0,
        PIN_SC1,
        PIN_MRD,
        PIN_MWR,
        PIN_N0,
        PIN_N1,
        PIN_N2,
};

inline void clock_cycle() __attribute__((always_inline));
inline void clock_cycle() {
    clock_hi();
    delayNanoseconds(clock_hi_ns);
    clock_lo();
    delayNanoseconds(clock_lo_ns);
}

}  // namespace

void PinsCdp1802::reset() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    assert_reset();
    for (auto i = 0; i < 100; i++)
        clock_cycle();
    negate_reset();
    // The first machine cycle after termination of reset is an
    // intialization cycle which requires 9 clock pulses.
    for (auto i = 0; i < 20; i++) {
        clock_cycle();
        if (signal_tpa() != LOW) {
            // c10
            break;
        }
    }
    Regs.reset();
    Regs.save();
}

Signals *PinsCdp1802::rawPrepareCycle() {
    // CDP1802 bus cycle is CLOCK/8, so we toggle CLOCK 8 times
    auto signals = Signals::put();
    busMode(DBUS, INPUT);
    signals->getStatus();
    return signals;
}

Signals *PinsCdp1802::prepareCycle() {
    // c10
    delayNanoseconds(c10_ns);
    return rawPrepareCycle();
}

Signals *PinsCdp1802::directCycle(Signals *signals) {
    // c11
    clock_hi();
    delayNanoseconds(c11_ns);
    // c20
    clock_lo();
    delayNanoseconds(c20_ns);
    signals->getAddr1();
    // c21
    clock_hi();
    delayNanoseconds(c21_ns);
    // c30
    clock_lo();
    delayNanoseconds(c30_ns);
    // c31
    clock_hi();
    delayNanoseconds(c31_ns);
    // c40
    clock_lo();
    delayNanoseconds(c40_ns);
    signals->getAddr2();
    // c41
    clock_hi();
    delayNanoseconds(c41_ns);
    // c50
    clock_lo();
    delayNanoseconds(c50_ns);
    // c51
    clock_hi();
    delayNanoseconds(c51_ns);
    signals->getDirection();
    // c60
    clock_lo();

    return signals;
}

Signals *PinsCdp1802::completeCycle(Signals *signals) {
    if (signals->write()) {
        // c60
        delayNanoseconds(c60_write);
        // c61
        clock_hi();
        signals->getData();
        delayNanoseconds(c61_write);
        // c70
        clock_lo();
        if (signals->writeMemory()) {
            Memory.write(signals->addr, signals->data);
        } else {
            // capture data to signals->data
            ;
        }
        delayNanoseconds(c70_write);
    } else if (signals->read()) {
        // c60
        if (signals->readMemory()) {
            signals->data = Memory.read(signals->addr);
        } else {
            // inject data from signals->data
            ;
        }
        delayNanoseconds(c60_read);
        // c61
        clock_hi();
        busMode(DBUS, OUTPUT);
        busWrite(DBUS, signals->data);
        delayNanoseconds(c61_read);
        // c70
        clock_lo();
        delayNanoseconds(c70_read);
        // DBUS will goes INPUT at prepareCycle() to fullfill hold time.
    } else {
        // c60
        delayNanoseconds(c60_nobus);
        // c61
        clock_hi();
        signals->getData();
        delayNanoseconds(c61_nobus);
        // c70
        clock_lo();
        delayNanoseconds(c70_nobus);
    }
    // c71
    clock_hi();
    Signals::nextCycle();
    delayNanoseconds(c71_ns);
    // c00
    clock_lo();
    delayNanoseconds(c00_ns);
    // c01
    clock_hi();
    delayNanoseconds(c01_ns);
    // c10
    clock_lo();
    // BitBang serial handler
    SciH.loop();
    return signals;
}

Signals *PinsCdp1802::cycle() {
    return completeCycle(directCycle(prepareCycle()));
}

Signals *PinsCdp1802::cycle(uint8_t data) {
    Signals::put()->inject(data);
    return cycle();
}

void PinsCdp1802::execInst(const uint8_t *inst, uint8_t len) {
    execute(inst, len, nullptr, nullptr, 0);
}

uint8_t PinsCdp1802::captureWrites(const uint8_t *inst, uint8_t len,
        uint16_t *addr, uint8_t *buf, uint8_t max) {
    return execute(inst, len, addr, buf, max);
}

uint8_t PinsCdp1802::execute(const uint8_t *inst, uint8_t len, uint16_t *addr,
        uint8_t *buf, uint8_t max) {
    uint8_t inj = 0;
    uint8_t cap = 0;
    while (inj < len || cap < max) {
        auto signals = rawPrepareCycle();
        if (inj < len)
            signals->inject(inst[inj]);
        if (cap < max)
            signals->capture();
        completeCycle(directCycle(signals));
        if (signals->read())
            ++inj;
        if (signals->write()) {
            if (cap == 0 && addr)
                *addr = signals->addr;
            if (cap < max && buf)
                buf[cap++] = signals->data;
        }
    }
    while (true) {
        auto signals = prepareCycle();
        if (signals->fetch())
            break;
        completeCycle(directCycle(signals));
    }
    return cap;
}

void PinsCdp1802::idle() {
    // CDP1802 can be static.
}

void PinsCdp1802::loop() {
    while (true) {
        Devs.loop();
        if (!rawStep() || haltSwitch()) {
            restoreBreakInsts();
            disassembleCycles();
            Regs.save();
            return;
        }
    }
}

void PinsCdp1802::run() {
    Regs.restore();
    Signals::resetCycles();
    saveBreakInsts();
    loop();
}

bool PinsCdp1802::rawStep() {
    auto signals = directCycle(prepareCycle());
    if (Memory.raw_read(signals->addr) == InstCdp1802::IDL) {
        // Detect IDL, inject LBR * instead and halt.
        completeCycle(signals->inject(InstCdp1802::LBR));
        cycle(hi(signals->addr));
        cycle(lo(signals->addr));
        Signals::discard(signals);
        return false;
    }
    completeCycle(signals);
    // See if it was CDP1804/CDP1804A's double fetch instruction.
    auto s = prepareCycle();
    if (s->fetch() && signals->data == InstCdp1802::PREFIX) {
        completeCycle(directCycle(s));
    }
    while (true) {
        auto s = prepareCycle();
        if (s->fetch())
            break;
        completeCycle(directCycle(s));
    }
    return true;
}

bool PinsCdp1802::step(bool show) {
    Regs.restore();
    Signals::resetCycles();
    if (rawStep()) {
        if (show)
            printCycles();
        Regs.save();
        return true;
    }
    return false;
}

bool PinsCdp1802::skip(uint8_t inst) {
    auto org = prepareCycle();
    completeCycle(directCycle(org->inject(inst)));
    const auto skipi = org->addr;
    while (true) {
        auto s = prepareCycle();
        if (s->fetch()) {
            completeCycle(directCycle(s->inject(InstCdp1802::NOP)));
            const auto nexti = s->addr;
            while (true) {
                auto s = prepareCycle();
                if (s->fetch())
                    return nexti == skipi + 3;
                completeCycle(directCycle(s));
            }
        }
        completeCycle(directCycle(s));
    }
}

void PinsCdp1802::assertInt(uint8_t name) {
    assert_intr();
}

void PinsCdp1802::negateInt(uint8_t name) {
    negate_intr();
}

void PinsCdp1802::setBreakInst(uint32_t addr) const {
    Memory.put_inst(addr, InstCdp1802::IDL);
}

void PinsCdp1802::printCycles() const {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles; i++) {
        g->next(i)->print();
    }
}

void PinsCdp1802::disassembleCycles() const {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles;) {
        const auto s = g->next(i);
        if (s->fetch()) {
            const auto len = Memory.disassemble(s->addr, 1) - s->addr;
            i += len;
        } else {
            s->print();
            ++i;
        }
    }
}

}  // namespace cdp1802
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
