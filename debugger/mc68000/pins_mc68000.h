#ifndef __PINS_MC68000_H__
#define __PINS_MC68000_H__

// The pins every 68000-family board shares.
// The strobes and #HALT, one read of GPIO9. P42 and P43 are #UDS and #LDS
// on a 16-bit bus; on the MC68008 P42 is an output, #IPL2/#IPL0, and P43
// #DS.
#define PORT_CNTL 9  /* GPIO9 */
#define CNTL_gp 4    /* P9.04-P9.08 */
#define CNTL_gm 0x1F /* P9.04-P9.08 */
#define CNTL_vp 0
#define PIN_AS 2     /* P9.04 */
#define PIN_RW 3     /* P9.05 */
#define PIN_P42 4    /* P9.06 */
#define PIN_P43 33   /* P9.07 */
#define PIN_HALT 5   /* P9.08 */
#define PIN_DTACK 0  /* P6.03 */
#define PIN_BERR 1   /* P6.02 */
#define PIN_CLK 29   /* P9.31 */
#define PIN_RESET 28 /* P8.18 */
#define PIN_ASEL0 31 /* P8.22 */

#include "pins.h"
#include "pwm_clock.h"
#include "regs_mc68000.h"
#include "signals_mc68000.h"

namespace debugger {
namespace mc68000 {

// The bus-cycle engine, the halt, step and run every board shares. CLK
// never stops: it runs from the PWM from reset on, and the CPU waits for
// the Teensy by #DTACK alone. A board class reads the address, moves the
// data and drives the interrupt level.
struct PinsMc68000 : Pins {
    // The clock runs free and the parked CPU waits on #DTACK.
    void idle() override {}
    bool step(bool show) override;
    void run() override;

    void printCycles() override;
    uint32_t clockInput() const override { return _clockInput; }
    bool setClockInput(uint32_t hz) override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void setBreakInst(uint32_t addr) const override;

    // Where an injected sequence leaves off: back at its own origin, or --
    // given as a plain address -- the target of the jump it ends with.
    static constexpr uint32_t EXIT_ORG = UINT32_MAX;

    // Run |inst| on the parked CPU: its reads inside the window are fed
    // from |inst| by address, other program reads get NOPs, data reads
    // outside it come from memory, and every write is captured, never
    // stored. The first |max| bytes written go to |buf| and their
    // addresses to |addrs|; the count captured is returned. |org| is where
    // the CPU is parked, and is advanced to where the sequence leaves off.
    // Instructions are words, so |org| and |len| are even, and every
    // sequence ends in a taken branch: the prefetch reaches the exit before
    // the last instructions execute.
    void execInst(const uint8_t *inst, uint_fast8_t len, uint32_t &org,
            uint32_t exit);
    uint_fast8_t captureWrites(const uint8_t *inst, uint_fast8_t len,
            uint8_t *buf, uint32_t *addrs, uint_fast8_t max, uint32_t &org,
            uint32_t exit);
    // Complete the vector read the CPU is parked in with |handler|, and
    // park it again at the handler's first fetch.
    bool answerVector(uint32_t vecAddr, uint32_t handler);
    // Whether the last sequence ended anywhere but its exit.
    bool cutShort() const { return _cutShort; }

protected:
    // |strobes| are the data strobes a write waits for: #DS on the MC68008,
    // #UDS and #LDS on a 16-bit bus.
    PinsMc68000(uint8_t strobes);

    bool _cutShort = false;
    // The clock's rate, and the Teensy cycles in one of its clocks.
    uint32_t _clockInput;
    uint32_t _clockCycles;

    // The reset sequence, once the board's pins are set up.
    void resetCpu();
    // The interrupt level to drive: 7 for a halt, 5 for a device, else 0.
    uint8_t iplLevel() const { return _halting ? 7 : (_intr ? 5 : 0); }
    uint32_t wrap(uint32_t addr) const;

    // What each board does its own way.
    // The board's straps, read while #RESET is held.
    virtual void readStraps() {}
    // Parked at the fetch of ORG_PARK after reset: what its address shows.
    virtual void detectBoard() {}
    // The address, FC, and for a read the byte lanes, as #AS opens a cycle.
    virtual void getAddr(Signals *s) const = 0;
    // A write's byte lanes, once its data strobes assert.
    virtual void getLanes(Signals *s) const {}
    // A byte moves in bits 0-7 of |data|, whatever lane carries it.
    virtual void getData(Signals *s) const = 0;
    virtual void outData(const Signals *s) const = 0;
    virtual void inputMode() const = 0;
    virtual void setIpl() const = 0;

private:
    PwmClock<PIN_CLK> _clock;
    const uint8_t _strobes;
    // A device's interrupt is pending; level 7 is being raised for a halt.
    bool _intr = false;
    bool _halting = false;

    void printBacktrace() override;
    Signals *prepareCycle();
    // Gives up after |clocks| without #AS: the CPU is stopped.
    Signals *prepareCycle(uint_fast16_t clocks);
    // The cycle the CPU is parked in, read again.
    Signals *resumeCycle();
    Signals *completeCycle(Signals *s);
    void serveRead(Signals *s);
    void storeWrite(Signals *s);
    uint_fast8_t execute(const uint8_t *inst, uint_fast8_t len, uint8_t *buf,
            uint32_t *addrs, uint_fast8_t max, uint32_t &org, uint32_t exit);
    uint_fast8_t frameBytes();
    bool findFrame(const Signals *s, RegsMc68000::Frame &frame,
            const Signals *&from) const;
    bool loop();
    bool suspend(Signals *s);
};

}  // namespace mc68000
}  // namespace debugger
#endif /* __PINS_MC68000_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
