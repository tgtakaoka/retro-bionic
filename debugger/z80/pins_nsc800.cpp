#include "pins_nsc800.h"
#include "debugger.h"
#include "devs_z80.h"
#include "inst_z80.h"
#include "mems_nsc800.h"
#include "pins_nsc800_config.h"
#include "regs_nsc800.h"
#include "signals_nsc800.h"

namespace debugger {
namespace nsc800 {

using z80::Devs;
using z80::InstZ80;

struct PinsNsc800 Pins {
    Regs,
};

// clang-format off
/**
 * NSC800 bus cycle.
 *        _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _
 *   XIN | |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_
 *         \   \   \   \   \   \   \   \   \   \   \   \   \   \   \   \   \   \   \   \   \   \
 *          |t1H t1L t2H t2L t3H t3L t4H t4L
 *          |___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|   |___
 *   CLK ___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|
 *          \ __\   \       \ __\           \ __
 *   ALE ____|   |___|_______|   |___________|
 *       ____|_______|       |_____________________
 *   #RD     |       |_______|   
 *       ____|_______________                _____
 * #RFSH     |               |_______________|
 *       ____|____         __ ____
 *   AD  ____Xaddr>-------<inXrfsh>---
 */
// clang-format on

namespace {
// XIN- to CLK+: typ  15 ns
// XIN- to CLK-: typ  15 ns
//          tx: min 125 ns; XIN period
//           T: min 250 ns; CLK period
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

constexpr auto xin_hi_ns = 400;      // 125 ns
constexpr auto xin_lo_ns = 400;      // 125 ns
constexpr auto clk_lo_xin_lo = 400;  // 125 ns
constexpr auto clk_lo_xin_hi = 400;  // 125 ns
constexpr auto clk_hi_xin_lo = 400;  // 125 ns
constexpr auto clk_hi_xin_hi = 400;  // 125 ns

inline void xin_hi() {
    digitalWriteFast(PIN_XIN, HIGH);
}

inline void xin_lo() {
    digitalWriteFast(PIN_XIN, LOW);
}

inline auto signal_clk() {
    return digitalReadFast(PIN_CLK);
}

inline auto signal_ale() {
    return digitalReadFast(PIN_ALE);
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

inline auto signal_iom() {
    return digitalReadFast(PIN_IOM);
}

inline void assert_wait() {
    digitalWriteFast(PIN_WAIT, LOW);
}

inline void negate_wait() {
    digitalWriteFast(PIN_WAIT, HIGH);
}

void negate_reset() {
    digitalWriteFast(PIN_RESETIN, HIGH);
}

constexpr uint8_t PINS_LOW[] = {
        PIN_RESETIN,
        PIN_XIN,
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_PS,
        PIN_BREQ,
        PIN_WAIT,
        PIN_INT,
        PIN_NMI,
        PIN_RSTA,
        PIN_RSTB,
        PIN_RSTC,
};

constexpr uint8_t PINS_INPUT[] = {
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
        PIN_CLK,
        PIN_IOM,
        PIN_ALE,
        PIN_RD,
        PIN_WR,
        PIN_S0,
        PIN_S1,
        PIN_RFSH,
        PIN_BACK,
};

inline void xin_cycle() {
    xin_hi();
    delayNanoseconds(xin_hi_ns);
    xin_lo();
    delayNanoseconds(xin_lo_ns);
}

inline void clk_hi_nowait() {
    xin_hi();
    delayNanoseconds(clk_lo_xin_hi);
    xin_lo();
}

inline void clk_hi() {
    clk_hi_nowait();
    delayNanoseconds(clk_lo_xin_lo);
}

inline void clk_lo_nowait() {
    xin_hi();
    delayNanoseconds(clk_hi_xin_hi);
    xin_lo();
}

inline void clk_lo() {
    clk_hi_nowait();
    delayNanoseconds(clk_hi_xin_lo);
}

inline void clk_cycle() {
    clk_hi();
    clk_lo();
}

}  // namespace

void PinsNsc800::reset() {
    // Assert reset condition
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT_PULLUP);

    // Synchronize XIN and CLK
    while (signal_clk() == LOW)
        xin_cycle();
    while (signal_clk() != LOW)
        xin_cycle();
    // CLK=L
    // #RESETIN must remain low for at least 3t state (CLK) times.
    for (auto i = 0; i < 5; i++)
        clk_cycle();
    negate_reset();
    // 2T cycles are necessary before first ALE.
    // (Waiting for ALE at prepareCycle().
    Signals::resetCycles();
    // prepareWait();
    //_regs.setIp(InstZ80::ORG_RESET);
    //_regs.save();
}

z80::Signals *PinsNsc800::prepareCycle() const {
    const auto s = SignalsNsc800::put();
    while (signal_ale() == LOW) {
        clk_hi();
        clk_lo();
    }
    xin_lo();
    s->getAddress();
    xin_hi();
    clk_lo();
    clk_hi_nowait();
    s->getControl();
    return s;
}

z80::Signals *PinsNsc800::completeCycle(z80::Signals *signals) const {
    auto s = static_cast<SignalsNsc800 *>(signals);
    // T3A
    clk_lo_nowait();
    if (s->read()) {
        if (s->memory()) {  // Memory access
            if (s->readMemory()) {
                s->data = Memory.raw_read(s->addr);
            }
        } else {  // I/O access
            const uint8_t ioaddr = s->addr;
            if (s->intack()) {
                s->data = Devs.vector();
            } else if (Devs.isSelected(ioaddr)) {
                s->data = Devs.read(ioaddr);
            } else {
                s->data = 0;
            }
        }
        s->outData();
    } else {
        s->getData();
        if (s->memory()) {
            if (s->writeMemory()) {
                Memory.raw_write(s->addr, s->data);
            } else {
                ; //
            }
        } else {
            const uint8_t ioaddr = s->addr;
            if (Devs.isSelected(ioaddr)) {
                Devs.write(ioaddr, s->data);
            }
        }
    }
    // T3B
    clk_hi_nowait();
    Signals::nextCycle();
    s->inputMode();
    // T1AL or T4/T5
    clk_lo();
    return s;
}

z80::Signals *PinsNsc800::prepareWait() const {
    assert_wait();
    const auto s = prepareCycle();
    // #MREQ:T2, #IORQ:Twa/Twa2
    clk_cycle();
    return s;
}

void PinsNsc800::idle() {
    // #WAIT=L
    clk_hi();
    clk_lo();
}

void PinsNsc800::assertInt(uint8_t name) {
    assert_int();
}

void PinsNsc800::negateInt(uint8_t name) {
    negate_int();
}

}  // namespace nsc800
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
