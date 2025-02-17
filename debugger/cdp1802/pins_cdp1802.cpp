#include "pins_cdp1802.h"
#include "debugger.h"
#include "devs_cdp1802.h"
#include "inst_cdp1802.h"
#include "mems_cdp1802.h"
#include "regs_cdp1802.h"
#include "signals_cdp1802.h"

namespace debugger {
namespace cdp1802 {

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
// OSC1 5.0 MHz
constexpr auto clock_hi_ns = 200;
constexpr auto clock_lo_ns = 200;
constexpr auto c00_ns = 26;       // 100 ns
constexpr auto c01_ns = 72;       // 100 ns
constexpr auto c11_ns = 80;       // 100 ns
constexpr auto c20_ns = 58;       // 100 ns
constexpr auto c21_ns = 38;       // 100 ns
constexpr auto c30_ns = 84;       // 100 ns
constexpr auto c31_ns = 80;       // 100 ns
constexpr auto c40_ns = 52;       // 100 ns
constexpr auto c41_io = 12;       // 100 ns
constexpr auto c41_mem = 36;      // 100 ns
constexpr auto c41_inject = 40;   // 100 ns
constexpr auto c41_ns = 80;       // 100 ns
constexpr auto c50_ns = 76;       // 100 ns
constexpr auto c51_ns = 66;       // 100 ns
constexpr auto c60_write = 40;    // 100 ns
constexpr auto c61_write = 68;    // 100 ns
constexpr auto c70_write = 44;    // 100 ns
constexpr auto c70_capture = 64;  // 100 ns
// constexpr auto c60_read = 44;     // 100 ns
// constexpr auto c60_inject = 68;   // 100 ns
constexpr auto c60_read = 30;   // 100 ns
constexpr auto c61_read = 14;   // 100 ns
constexpr auto c70_read = 68;   // 100 ns
constexpr auto c60_nobus = 40;  // 100 ns
constexpr auto c61_nobus = 76;  // 100 ns
constexpr auto c70_nobus = 68;  // 100 ns
constexpr auto c71_ns = 58;     // 100 ns

inline void clock_hi() {
    digitalWriteFast(PIN_CLOCK, HIGH);
}

inline void clock_lo() {
    digitalWriteFast(PIN_CLOCK, LOW);
}

void assert_intr() {
    digitalWriteFast(PIN_INTR, LOW);
}

void negate_intr() {
    digitalWriteFast(PIN_INTR, HIGH);
}

inline auto signal_tpa() {
    return digitalReadFast(PIN_TPA);
}

void negate_reset() {
    digitalWriteFast(PIN_CLEAR, HIGH);
}

constexpr uint8_t PINS_LOW[] = {
        PIN_CLEAR,
        PIN_CLOCK,
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_INTR,
        PIN_WAIT,
        PIN_DMAIN,
        PIN_DMAOUT,
        PIN_EF1,
        PIN_EF2,
        PIN_EF3,
        PIN_EF4,
};

constexpr uint8_t PINS_INPUT[] = {
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

inline void clock_cycle() {
    clock_hi();
    delayNanoseconds(clock_hi_ns);
    clock_lo();
    delayNanoseconds(clock_lo_ns);
}

}  // namespace

PinsCdp1802::PinsCdp1802() {
    _regs = new RegsCdp1802(this);
    _devs = new DevsCdp1802();
    _mems = new MemsCdp1802();
}

void PinsCdp1802::resetPins() {
    // Assert reset condition
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    for (auto i = 0; i < 100; i++)
        clock_cycle();
    negate_reset();
    Signals::resetCycles();
    // The first machine cycle after termination of reset is an
    // intialization cycle which requires 9 clock pulses.
    for (auto i = 0; i < 20; i++) {
        clock_cycle();
        if (signal_tpa() != LOW)
            break;
    }
    _regs->save();
    _regs->reset();
}

Signals *PinsCdp1802::startCycle() {
    // CDP1802 bus cycle is CLOCK/8, so we toggle CLOCK 8 times
    auto s = Signals::put();
    // assert_debug();
    s->getStatus();
    //negate_debug();
    return s;
}

Signals *PinsCdp1802::prepareCycle(Signals *s) {
    // c11
    clock_hi();
    delayNanoseconds(c11_ns);
    // c20
    clock_lo();
    delayNanoseconds(c20_ns);
    // assert_debug();
    s->getAddr1();
    // c21
    clock_hi();
    const auto ioaddr = s->getIoAddr();
    s->getDirection();  // check #MRD
    // negate_debug();
    delayNanoseconds(c21_ns);
    // c30
    clock_lo();
    if (ioaddr && !s->read()) {
        // Input from Device to Memory & CPU
        if (_devs->isSelected(ioaddr)) {
            // assert_debug();
            s->data = _devs->read(ioaddr);
            s->outData();
            // negate_debug();
        }
    } else {
        delayNanoseconds(c30_ns);
    }
    // c31
    clock_hi();
    delayNanoseconds(c31_ns);
    // c40
    clock_lo();
    delayNanoseconds(c40_ns);
    // assert_debug();
    s->getAddr2();
    // negate_debug();
    //  c41
    clock_hi();
    if (s->read()) {
        if (ioaddr) {
            // Output from Memory to Device
            s->inject(_mems->read(s->addr));  // avoid duplicate read
            delayNanoseconds(c41_io);
        } else if (s->readMemory()) {
            s->inject(_mems->read(s->addr));
            delayNanoseconds(c41_mem);
        } else {
            delayNanoseconds(c41_inject);
        }
    } else {
        delayNanoseconds(c41_ns);
    }
    // c50
    clock_lo();
    if (ioaddr && s->read()) {
        // Output from Memory to Device
        if (_devs->isSelected(ioaddr)) {
            _devs->write(ioaddr, s->data);
        }
    } else {
        delayNanoseconds(c50_ns);
    }
    // c51
    clock_hi();
    delayNanoseconds(c51_ns);
    // assert_debug();
    s->getDirection();  // check #MWR
    // negate_debug();
    //  c60
    clock_lo();
    return s;
}

Signals *PinsCdp1802::completeCycle(Signals *s) {
    if (s->write()) {
        // c60
        delayNanoseconds(c60_write);
        // c61
        clock_hi();
        delayNanoseconds(c61_write);
        // assert_debug();
        s->getData();
        // negate_debug();
        // c70
        clock_lo();
        if (s->writeMemory()) {
            _mems->write(s->addr, s->data);
            delayNanoseconds(c70_write);
        } else {
            delayNanoseconds(c70_capture);
        }
    } else if (s->read()) {
        // c60
        delayNanoseconds(c60_read);
        // c61
        clock_hi();
        // assert_debug();
        s->outData();
        // negate_debug();
        delayNanoseconds(c61_read);
        // c70
        clock_lo();
        delayNanoseconds(c70_read);
    } else {
        // c60
        delayNanoseconds(c60_nobus);
        // c61
        clock_hi();
        delayNanoseconds(c61_nobus);
        s->data = 0;
        // c70
        clock_lo();
        delayNanoseconds(c70_nobus);
    }
    // c71
    clock_hi();
    // assert_debug();
    Signals::inputMode();
    // negate_debug();
    delayNanoseconds(c71_ns);
    // c00
    clock_lo();
    Signals::nextCycle();
    delayNanoseconds(c00_ns);
    // c01
    clock_hi();
    // BitBang serial handler
    devs<DevsCdp1802>()->sciLoop();
    delayNanoseconds(c01_ns);
    // c10
    clock_lo();
    return s;
}

Signals *PinsCdp1802::cycle() {
    return completeCycle(prepareCycle(startCycle()));
}

Signals *PinsCdp1802::inject(uint8_t data) {
    Signals::put()->inject(data);
    return cycle();
}

void PinsCdp1802::execInst(const uint8_t *inst, uint8_t len) {
    execute(inst, len, nullptr, nullptr, 0);
}

void PinsCdp1802::captureWrites(const uint8_t *inst, uint8_t len,
        uint16_t *addr, uint8_t *buf, uint8_t max) {
    execute(inst, len, addr, buf, max);
}

void PinsCdp1802::execute(const uint8_t *inst, uint8_t len, uint16_t *addr,
        uint8_t *buf, uint8_t max) {
    uint8_t inj = 0;
    uint8_t cap = 0;
    auto s = startCycle();
    while (inj < len || cap < max) {
        if (inj < len)
            s->inject(inst[inj]);
        if (cap < max)
            s->capture();
        completeCycle(prepareCycle(s));
        if (s->read())
            ++inj;
        if (s->write()) {
            if (cap == 0 && addr)
                *addr = s->addr;
            if (cap < max && buf)
                buf[cap++] = s->data;
        }
        s = startCycle();
    }
    while (true) {
        if (s->fetch())
            break;
        completeCycle(prepareCycle(s));
        s = startCycle();
    }
}

void PinsCdp1802::idle() {
    // CDP1802 can be static.
}

void PinsCdp1802::loop() {
    auto s = startCycle();
    while (true) {
        _devs->loop();
        s = rawStep(s);
        if (s == nullptr || haltSwitch())
            return;
    }
}

void PinsCdp1802::run() {
    _regs->restore();
    Signals::resetCycles();
    saveBreakInsts();
    loop();
    restoreBreakInsts();
    disassembleCycles();
    _regs->save();
}

Signals *PinsCdp1802::rawStep(Signals *s) {
    s = prepareCycle(s);
    if (s->data == InstCdp1802::IDL) {
        // Detect IDL, inject LBR * instead and halt.
        completeCycle(s->inject(InstCdp1802::LBR));
        inject(hi(s->addr));
        inject(lo(s->addr));
        Signals::discard(s);
        return nullptr;
    }
    completeCycle(s);
    // See if it was CDP1804/CDP1804A's double fetch instruction.
    auto n = startCycle();
    if (n->fetch() && s->data == InstCdp1802::PREFIX) {
        completeCycle(prepareCycle(n));
        s = startCycle();
    } else {
        s = n;
    }
    do {
        completeCycle(prepareCycle(s));
        s = startCycle();
    } while (!s->fetch());
    return s;
}

bool PinsCdp1802::step(bool show) {
    Signals::resetCycles();
    _regs->restore();
    if (show)
        Signals::resetCycles();
    if (rawStep(startCycle())) {
        if (show)
            printCycles();
        _regs->save();
        return true;
    }
    return false;
}

bool PinsCdp1802::skip(uint8_t inst) {
    auto org = startCycle();
    completeCycle(prepareCycle(org->inject(inst)));
    const auto skipi = org->addr;
    while (true) {
        auto s = startCycle();
        if (s->fetch()) {
            completeCycle(prepareCycle(s->inject(InstCdp1802::NOP)));
            const auto nexti = s->addr;
            while (true) {
                auto s = startCycle();
                if (s->fetch())
                    return nexti == skipi + 3;
                completeCycle(prepareCycle(s));
            }
        }
        completeCycle(prepareCycle(s));
    }
}

void PinsCdp1802::assertInt(uint8_t name) {
    assert_intr();
}

void PinsCdp1802::negateInt(uint8_t name) {
    negate_intr();
}

void PinsCdp1802::setBreakInst(uint32_t addr) const {
    _mems->put_inst(addr, InstCdp1802::IDL);
}

void PinsCdp1802::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles; ++i) {
        g->next(i)->print();
    }
}

void PinsCdp1802::disassembleCycles() const {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles;) {
        const auto s = g->next(i);
        if (s->fetch()) {
            const auto len = _mems->disassemble(s->addr, 1) - s->addr;
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
