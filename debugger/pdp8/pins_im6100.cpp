#include "pins_im6100.h"
#include "debugger.h"
#include "devs_pdp8.h"
#include "inst_pdp8.h"
#include "mems_pdp8.h"
#include "regs_im6100.h"
#include "signals_im6100.h"

namespace debugger {
namespace im6100 {

// clang-format off
/**
 * IM6100 bus cycle.                                                     |  optional |
 *            _____       _____       _____       _____       _____      |_____      |__
 *  STATES __| T1H |_T1L_| T2H |_T2L_| T3H |_T3L_| T4H |_T4L_| T5H |_T5L_| T6H |_T6L_|
 *           __    __    __    __    __    __    __    __    __    __    __    __    __
 *    OSC  _|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |
 *             \     \  \           |     \                                \  |  \
 *              |_____|  |          v      |                                | v   |
 *   LXMAR _____/     \__|_________________|________________________________|_____|_____
 *         ______________|                 |________________________________|     |_____
 * #xxxSEL               \_________________/                                \_____/
 *                  ____           _________                                 ______
 *      DX --------<_AD_>---------<__RD_____>-------------------------------<__WR__>-----
 *         _________________________v_________________________________________v_________
 *   #WAIT ________________________/ \_______________________________________/ \________
 *           ________________________________________________________________________
 *  IFETCH _/                                                              \_________\__
 *         _____________ v _____________________________________________________________
 * #xxxREQ _____________\_/_____________________________________________________________
 *                              ___________
 *     XTA ____________________/           \____________________________________________
 *         _____________                                                   _____________
 *     XTB              \_________________________________________________/
 *           ___________________________________                                     ___
 *     XTC _/                          RD       \_____________________________WR____/
 * 120 ns ; OSC- to LXMAR+
 * 135 ns ; OSC- to LXMAR-
 * 152 ns ; OSC+ to #MEM/CPSEL-
 * 160 ns ; OSC+ to #MEM/CPSEL+
 * 152 ns ; OSC- to #MEM/CPSEL-
 * 136 ns ; OSC- to #MEM/CPSEL+
 */
// clang-format on

namespace {
// IM6100A
//   fOSC: min 0 MHz, max 5.71 MHz
//     tS: min 350 ns ; State cycle
// tLXMAR: min 150 ns ; LXMAR pulse width
//    tAS: min  55 ns ; DX setup to LXMAR-
//    tAH: min  60 ns ; DX hold from LXMAR-
//    tWP: min 140 ns ; #MEMSEL/#CPSEL pulse width
//    tDS: min 115 ns ; DX setup to #MEM/CPSEL+
//    tDH: min  60 ns ; DX hold from #MEM/CPSEL+
//    tAL: max 295 ns ; DX input from LXMAR-
//    tEN: max 185 ns ; DX input from #MEM/CPSEL-
//   tWPD: min 140 ns ; #DEVSEL pulse width
//   tDSD: min 110 ns ; DX setup to #DEVSEL+
//   tDHD: min  60 ns ; DX hold from #DEVSEL+
//   tEND: max 250 ns ; DX input from #DEVSEL-
//    tRS: min   0 ns ; #CP/INTREQ setup to T2+
//    tRH: min 125 ns ; #CP/INTREQ hold from T2+
//   tRHP: min  45 ns ; #RUN/HLT pulse width
//    tWS: min  45 ns ; #WAIT setup to T3+
//    tWH: min  15 ns ; #WAIT hold from T3+
//    tSL: min  35 ns, max 180 ns ; #xxSEL- delay from T2+
//    tXT: min  35 ns, max 155 ns ; LXMAR/XTx delay from T2-
//    tST:             max 190 ns ; DATAF/RUN/xxxGNT/IFETCH delay from T1+

constexpr auto osc_hi_ns = 70;       // 90
constexpr auto osc_lo_ns = 70;       // 90
constexpr auto osc_lo_lxmar = 50;    // 90
constexpr auto osc_lo_addr = 35;     // 90
constexpr auto osc_hi_inject = 50;   // 90
constexpr auto osc_lo_output = 25;   // 90
constexpr auto osc_hi_input = 10;    // 90
constexpr auto osc_lo_next = 45;     // 90
constexpr auto osc_lo_get = 0;       // 90
constexpr auto osc_hi_capture = 50;  // 90
constexpr auto osc_lo_xtc = 35;      // 90

inline void osc_hi() {
    digitalWriteFast(PIN_OSCOUT, HIGH);
}

inline void osc_lo() {
    digitalWriteFast(PIN_OSCOUT, LOW);
}

auto signal_xtb() {
    return digitalReadFast(PIN_XTB);
}

auto signal_xtc() {
    return digitalReadFast(PIN_XTC);
}

auto signal_lxmar() {
    return digitalReadFast(PIN_LXMAR);
}

auto signal_run() {
    return digitalReadFast(PIN_RUN);
}

void negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
}

void assert_runhlt() {
    digitalWriteFast(PIN_RUNHLT, LOW);
}

void negate_runhlt() {
    digitalWriteFast(PIN_RUNHLT, HIGH);
}

constexpr uint8_t PINS_LOW[] = {
        PIN_RESET,
        PIN_OSCOUT,
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_WAIT,
        PIN_CPREQ,
        PIN_INTREQ,
        PIN_DMAREQ,
        PIN_RUNHLT,
        PIN_C2,
        PIN_C1,
        PIN_C0,
        PIN_SKP,
};

constexpr uint8_t PINS_INPUT[] = {
        PIN_DX11,
        PIN_DX10,
        PIN_DX9,
        PIN_DX8,
        PIN_DX7,
        PIN_DX6,
        PIN_DX5,
        PIN_DX4,
        PIN_DX3,
        PIN_DX2,
        PIN_DX1,
        PIN_DX0,
        PIN_XTA,
        PIN_XTB,
        PIN_XTC,
        PIN_LXMAR,
        PIN_IFETCH,
        PIN_DATAF,
        PIN_RUN,
        PIN_INTGNT,
        PIN_DMAGNT,
        PIN_LINK,
        PIN_MEMSEL,
        PIN_CPSEL,
        PIN_DEVSEL,
        PIN_SWSEL,
};

inline void osc_cycle_lo() {
    osc_hi();
    delayNanoseconds(osc_hi_ns);
    osc_lo();
}

inline void osc_cycle() {
    osc_cycle_lo();
    delayNanoseconds(osc_lo_ns);
}

void osc_cycle(uint_fast8_t n) {
    while (n--)
        osc_cycle();
}

}  // namespace

PinsIm6100::PinsIm6100() : PinsPdp8() {
    _regs = new RegsIm6100(this);
    _devs = new pdp8::DevsPdp8();
    auto mems = new pdp8::MemsPdp8(12);
    _mems = mems;
    _cpmems = new pdp8::ControlPanel(mems);
}

void PinsIm6100::resetPins() {
    // Assert reset condition
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    osc_cycle(50);
    while (signal_xtb() == LOW)
        osc_cycle();
    negate_reset();
    osc_cycle();
    assert_runhlt();
    osc_cycle();
    negate_runhlt();
    while (signal_run() == LOW)
        osc_cycle();
    Signals::resetCycles();
    _regs->reset();
    prepareCycle();
    _regs->save();
}

pdp8::Signals *PinsIm6100::resumeCycle(uint16_t pc) const {
    auto s = Signals::put();
    s->clear();
    s->addr = pc;
    s->getControl();
    s->getDirection();
    return s;
}

pdp8::Signals *PinsIm6100::prepareCycle() const {
    auto s = Signals::put();
    if (signal_lxmar() != LOW) {
        osc_cycle_lo();
    } else {
        do {
            delayNanoseconds(osc_lo_lxmar);
            osc_cycle_lo();
        } while (signal_lxmar() == LOW);
    }
    delayNanoseconds(osc_lo_addr);
    // assert_debug();
    s->getAddress();
    // negate_debug();
    do {
        osc_cycle();
    } while (!s->getControl());
    // assert_debug();
    s->getDirection();
    // negate_debug();
    return s;
}

pdp8::Signals *PinsIm6100::completeCycle(pdp8::Signals *_s) const {
    auto s = static_cast<Signals *>(_s);
    osc_hi();
    if (s->mem()) {
        if (s->readMemory()) {
            s->data = _mems->read(s->addr);
            if (s->fetch() && pdp8::InstPdp8::isHalt(s->data))
                return nullptr;
        } else {
            delayNanoseconds(osc_hi_inject);
        }
    } else if (s->dev()) {
        if (_devs->isSelected(s->addr)) {
            s->data = _devs->read(s->addr);
            s->outIoc(devs<pdp8::DevsPdp8>()->control(s->addr));
        } else {
            constexpr auto OR = IOC_SKIP | IOC_C0 | IOC_C2;
            s->data = 0;
            s->outIoc(OR);  // to preserve AC contents.
        }
    } else if (s->cp()) {
        if (s->readMemory())
            s->data = _cpmems->read(s->addr);
    } else {
        s->data = regs<RegsIm6100>()->readSwitch();
    }
    osc_lo();
    // assert_debug();
    s->outData();
    // negate_debug();
    delayNanoseconds(osc_lo_output);
    osc_hi();
    Signals::nextCycle();
    delayNanoseconds(osc_hi_input);
    // assert_debug();
    s->inputMode();
    // negate_debug();
    osc_lo();
    auto n = Signals::put();
    *n = *s;  // copy address and control
    delayNanoseconds(osc_lo_next);
    osc_cycle();
    do {
        osc_cycle_lo();
        // assert_debug();
        if (n->getControl()) {
            n->getData();
            n->getDirection();
            // negate_debug();
            delayNanoseconds(osc_lo_get);
            osc_hi();
            if (n->mem()) {
                if (n->writeMemory()) {
                    _mems->write(n->addr, n->data);
                } else {
                    delayNanoseconds(osc_hi_capture);
                }
            } else if (n->dev()) {
                if (_devs->isSelected(n->addr))
                    _devs->write(n->addr, n->data);
            } else if (n->cp()) {
                if (n->writeMemory())
                    _cpmems->write(n->addr, n->data);
            }
            osc_lo();
            Signals::nextCycle();
            return n;
        }
        // negate_debug();
        delayNanoseconds(osc_lo_xtc);
    } while (signal_xtc() == LOW);
    return s;
}

}  // namespace im6100
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
