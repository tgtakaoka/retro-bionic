#ifndef __PINS_TMS7000_H__
#define __PINS_TMS7000_H__

#define PORT_AD 6       /* GPIO6 */
#define AD_gp 16        /* P6.16-P6.23 */
#define AD_gm 0xFF      /* P6.16-P6.23 */
#define AD_vp 0         /* AD0-AD7 */
#define PIN_AD0 19      /* P6.16 */
#define PIN_AD1 18      /* P6.17 */
#define PIN_AD2 14      /* P6.18 */
#define PIN_AD3 15      /* P6.19 */
#define PIN_AD4 40      /* P6.20 */
#define PIN_AD5 41      /* P6.21 */
#define PIN_AD6 17      /* P6.22 */
#define PIN_AD7 16      /* P6.23 */
#define PORT_ADDR 6     /* GPIO6 */
#define ADDR_gp 16      /* P6.16-P6.31 */
#define ADDR_gm 0xFFFF  /* P6.16-P6.31 */
#define ADDR_vp 0       /* A0-A15 */
#define PIN_ADDR8 22    /* P6.24 */
#define PIN_ADDR9 23    /* P6.25 */
#define PIN_ADDR10 20   /* P6.26 */
#define PIN_ADDR11 21   /* P6.27 */
#define PIN_ADDR12 38   /* P6.28 */
#define PIN_ADDR13 39   /* P6.29 */
#define PIN_ADDR14 26   /* P6.30 */
#define PIN_ADDR15 27   /* P6.31 */
#define PIN_PA0 10      /* P7.00 */
#define PIN_PA1 12      /* P7.01 */
#define PIN_PA2 11      /* P7.02 */
#define PIN_PA3 13      /* P7.03 */
#define PIN_PA4 8       /* P7.16 */
#define PIN_PA5 7       /* P7.17 */
#define PIN_PA6 36      /* P7.18 */
#define PIN_PA7 37      /* P7.19 */
#define PORT_CNTL 9     /* GPIO9 */
#define CNTL_gp 4       /* P9.04-P9.07 */
#define CNTL_gm 0xF     /* P9.04-P9.07 */
#define CNTL_vp 0       /* CNTL0-CNTL3 */
#define PIN_ENABLE 2    /* P9.04 */
#define PIN_RW 3        /* P9.05 */
#define PIN_ALATCH 33   /* P9.07 */
#define CNTL_ENABLE 0x1 /* CNTL0 */
#define CNTL_RW 0x2     /* CNTL1 */
#define CNTL_ALATCH 0x8 /* CNTL3 */
#define PIN_PB3 0       /* P6.03 */
#define PIN_CLKIN 5     /* P9.08 */
#define PIN_CLKOUT 29   /* P9.31 */
#define PIN_INT1 6      /* P7.10 */
#define PIN_INT3 9      /* P7.11 */
#define PIN_PB0 32      /* P7.12 */
#define PIN_RESET 28    /* P8.18 */
#define PIN_PB1 31      /* P8.22 */
#define PIN_PB2 30      /* P8.23 */

#include <Arduino.h>
#include "devs_tms7000.h"
#include "digital_bus.h"
#include "pins.h"
#include "signals_tms7000.h"

namespace debugger {
namespace tms7000 {

enum HardwareType : uint8_t {
    HW_TMS7000 = 0,  // TMS7000
    HW_TMS7001 = 3,  // TMS7001
    HW_TMS7002 = 6,  // TMS7002
};

enum ClockType : uint8_t {
    CLK_DIV2 = 0,  // NL2
    CLK_DIV4 = 1,  // NL4
    CLK_CMOS = 2,  // CMOS
};

// How the bus cycles are clocked; each has its own bus-cycle engine.
enum EngineType : uint8_t {
    ENG_DIV2 = 0,       // NMOS /2
    ENG_DIV4 = 1,       // NMOS /4
    ENG_CMOS = 2,       // CMOS /2, CLKIN inverted
    ENG_DIV2_7002 = 3,  // TMS7002, ALATCH rising in the CLKIN low half
    ENG_CMOS_7002 = 4,  // TMS70C02
};

enum IntrName : uint8_t {
    INTR_NONE = 0,
    INTR_INT1 = 1,
    INTR_INT3 = 3,
};

struct PinsTms7000 final : Pins {
    PinsTms7000();

    void resetPins() override;
    void idle() override;
    bool step(bool show) override;
    void run() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void setBreakInst(uint32_t addr) const override;
    void printCycles() override;

    void execInst(const uint8_t *inst, uint8_t len, uint8_t extra = 0);
    uint16_t captureWrites(
            const uint8_t *inst, uint8_t len, uint8_t *buf, uint8_t max);
    // Stops at an opcode fetch the register save can start from
    void park();
    HardwareType hardwareType() const { return _hardType; }
    ClockType clockType() const { return _clockType; }

private:
    HardwareType _hardType;
    ClockType _clockType;
    // The fetch park() left prepared, for execute() to start from
    Signals *_parked = nullptr;

    // The engine bus cycles run on.
    EngineType _engine = ENG_DIV2;
    template <typename F>
    auto withEngine(F f) const;


    template <EngineType E>
    void div2Complete(Signals *) const;
    void div4Complete(Signals *) const;
    template <EngineType E>
    Signals *prepareCycle() const;
    template <EngineType E>
    Signals *completeCycle(Signals *s) const;

    void synchronizeClock();
    template <EngineType E>
    void resetCpu();
    template <EngineType E>
    void checkHardwareType();
    template <EngineType E>
    Signals *cycle() const;
    template <EngineType E>
    Signals *inject(uint8_t data) const;
#ifdef PROFILE_CYCLES
    const Signals *_profileEnd;
    template <EngineType E>
    bool loop();
    static bool isTrap23Vector(const Signals *s);
    void markFetches(const Signals *end);
#else
    template <EngineType E>
    void loop();
#endif
    template <EngineType E>
    bool rawStep();
    template <EngineType E>
    void park();
    template <EngineType E>
    uint16_t execute(const uint8_t *inst, uint8_t len, uint8_t *buf = nullptr,
            uint8_t max = 0);

    const SignalsImpl *findBacktraceStart() override;
    void printBacktrace() override;
};

void Signals::getAddress() {
    addr = busRead(ADDR);
}

void Signals::getDirection() {
    cntl() = busRead(CNTL);
    markFetch(false);
}

bool Signals::external() const {
    return (cntl() & CNTL_ENABLE) == 0;
}

bool Signals::read() const {
    return (cntl() & (CNTL_ENABLE | CNTL_RW)) == CNTL_RW;
}

bool Signals::write() const {
    return (cntl() & (CNTL_ENABLE | CNTL_RW)) == 0;
}

// Interrupt acknowledge: an internal read of address 0
bool Signals::intack() const {
    return addr == 0 && (cntl() & (CNTL_ENABLE | CNTL_RW)) == (CNTL_ENABLE | CNTL_RW);
}

void Signals::getData() {
    data = busRead(AD);
}

void Signals::outData() const {
    busWrite(AD, data);
    busMode(AD, OUTPUT);
}

void Signals::inputMode() const {
    busMode(AD, INPUT);
}

}  // namespace tms7000
}  // namespace debugger
#endif /* __PINS_TMS7000_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
