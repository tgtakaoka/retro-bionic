#ifndef __PINS_Z8_H__
#define __PINS_Z8_H__

#include "inst_z8.h"

#define PORT_DATA 6    /* GPIO6 */
#define DATA_gp 16     /* P6.16-P6.23 */
#define DATA_gm 0xFF   /* P6.16-P6.23 */
#define DATA_vp 0      /* D0-D7 */
#define PIN_DATA0 19   /* P6.16 */
#define PIN_DATA1 18   /* P6.17 */
#define PIN_DATA2 14   /* P6.18 */
#define PIN_DATA3 15   /* P6.19 */
#define PIN_DATA4 40   /* P6.20 */
#define PIN_DATA5 41   /* P6.21 */
#define PIN_DATA6 17   /* P6.22 */
#define PIN_DATA7 16   /* P6.23 */
#define PORT_ADDR 6    /* GPIO6 */
#define ADDR_gp 16     /* P6.16-P6.31 */
#define ADDR_gm 0xFFFF /* P6.16-P6.31 */
#define ADDR_vp 0      /* A0-A15 */
#define PIN_ADDR0 19   /* P6.16 */
#define PIN_ADDR1 18   /* P6.17 */
#define PIN_ADDR2 14   /* P6.18 */
#define PIN_ADDR3 15   /* P6.19 */
#define PIN_ADDR4 40   /* P6.20 */
#define PIN_ADDR5 41   /* P6.21 */
#define PIN_ADDR6 17   /* P6.22 */
#define PIN_ADDR7 16   /* P6.23 */
#define PIN_ADDR8 22   /* P6.24 */
#define PIN_ADDR9 23   /* P6.25 */
#define PIN_ADDR10 20  /* P6.26 */
#define PIN_ADDR11 21  /* P6.27 */
#define PIN_ADDR12 38  /* P6.28 */
#define PIN_ADDR13 39  /* P6.29 */
#define PIN_ADDR14 26  /* P6.30 */
#define PIN_ADDR15 27  /* P6.31 */
#define PIN_AS 2       /* P9.04 */
#define PIN_RW 3       /* P9.05 */
#define PIN_IRQ1 4     /* P9.06 */
#define PIN_IRQ0 33    /* P9.07 */
#define PIN_TXD 0      /* P6.03 */
#define PIN_RXD 1      /* P6.02 */
#define PIN_XTAL1 5    /* P9.08 */
#define PIN_DS 29      /* P9.31 */
#define PIN_IRQ2 6     /* P7.10 */
#define PIN_RESET 28   /* P8.18 */
#define PIN_DM 30      /* P8.23 */

// Follows the Super8's fast interrupt, which pushes nothing; left out of
// normal builds, since checking every bus cycle for one slows the Super8 bus.
// #define Z88_FAST_INTERRUPT

#include "digital_bus.h"
#include "inst_z8.h"
#include "pins.h"
#include "serial_handler.h"
#include "signals_z8.h"

namespace debugger {
namespace z8 {

enum IntrName : uint8_t {
    INTR_NONE = 0,
    INTR_IRQ0 = 1,
    INTR_IRQ1 = 2,
    INTR_IRQ2 = 3,
};

// The part, one bus-cycle engine each.
enum Variant : uint8_t {
    VAR_Z86 = 0,
    VAR_Z88 = 1,
};

// Bus-cycle delays, in ns, each engine's own.
enum Delay : uint8_t {
    xtal1_first_ns,
    xtal1_second_ns,
    DELAY_COUNT,
};

struct PinsZ8 : Pins {
    void resetPins() override;
    bool step(bool show) override;
    void run() override;
    void idle() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void setBreakInst(uint32_t addr) const override;
    void printCycles() override;

    // The next fetch's address, past an interrupt acknowledge under way.
    uint16_t nextFetch();
    void execInst(const uint8_t *inst, uint8_t len);
    void captureWrites(const uint8_t *inst, uint8_t len, uint16_t *addr,
            uint8_t *buf, uint8_t max);

protected:
    PinsZ8(Variant engine, SerialHandler *serial, const InstZ8 &inst)
        : _engine(engine), _serial(serial), _inst(inst) {}

    // The engine bus cycles run on.
    const Variant _engine;
    SerialHandler *const _serial;
    const InstZ8 &_inst;
    uint8_t _writes;

    template <Variant E, Delay F>
    void delay() const;

    // What sets each engine's cycles apart, as constants.
    template <Variant E>
    struct Traits;
    template <typename F>
    auto withEngine(F f);

    template <Variant E>
    void xtal1_cycle() const;
    template <Variant E>
    void resetCpu();
    template <Variant E>
    Signals *prepareCycle();
    template <Variant E>
    Signals *completeCycle(Signals *signals);
    template <Variant E>
    Signals *cycle();
    template <Variant E>
    Signals *cycle(uint8_t insn);
    template <Variant E>
    uint16_t nextFetch();
    template <Variant E>
    void loop();
#ifdef PROFILE_CYCLES
    template <Variant E>
    Signals *awaitCycle(uint16_t clocks);
    void markFetches() const;
#endif
    template <Variant E>
    bool rawStep();
    void intrAck(Signals *frame) const;
    template <Variant E>
    void idle();
    template <Variant E>
    void execute(const uint8_t *inst, uint8_t len, uint16_t *addr, uint8_t *buf,
            uint8_t max);
    void execute(const uint8_t *inst, uint8_t len, uint16_t *addr, uint8_t *buf,
            uint8_t max);

#if defined(Z88_FAST_INTERRUPT)
    // The Super8's fast interrupt pushes nothing; PinsZ88 follows it, and
    // keeps here the fetch at IP one would make while it can be taken.
    // Out of address range otherwise; the Z86 engine never checks it.
    static constexpr uint32_t NO_FAST = 0x10000;
    uint32_t _fastFetch = NO_FAST;
    bool fastEntry(const Signals *s) const;
#endif

    const SignalsImpl *findBacktraceStart() override;
    void printBacktrace() override;
};

void Signals::getAddr() {
    addr = busRead(ADDR);
    rw() = digitalReadFast(PIN_RW);
    fetch() = false;
}

void Signals::getData() {
    data = busRead(DATA);
}

void Signals::outData() {
    busWrite(DATA, data);
    busMode(DATA, OUTPUT);
}

}  // namespace z8
}  // namespace debugger
#endif /* __PINS_Z8_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
