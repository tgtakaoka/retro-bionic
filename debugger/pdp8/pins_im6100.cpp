#include "pins_im6100.h"
#include "debugger.h"
#include "devs_pdp8.h"
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
 *                 _____             ______                                 ______
 *      DX -------<__AD_>-----------<__RD__>-------------------------------<__WR__>-----
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
 *           ___________________________________
 *     XTC _/                          RD       \_____________________________WR________
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

constexpr auto osc_hi_ns = 500;  // 87.5
constexpr auto osc_lo_ns = 500;  // 87.5

inline void osc_hi() {
    digitalWriteFast(PIN_OSCOUT, HIGH);
}

inline void osc_lo() {
    digitalWriteFast(PIN_OSCOUT, LOW);
}

auto signal_xta() {
    return digitalReadFast(PIN_XTA);
}

auto signal_lxmar() {
    return digitalReadFast(PIN_LXMAR);
}

void toggle_runhlt() {
    digitalToggleFast(PIN_RUNHLT);
    delayNanoseconds(40);
    digitalToggleFast(PIN_RUNHLT);
}

void negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
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

inline void osc_cycle() {
    osc_hi();
    delayNanoseconds(osc_hi_ns);
    osc_lo();
    delayNanoseconds(osc_lo_ns);
}

void system_cycle(uint_fast16_t n) {
    for (auto i = n * 2; i; --i)
        osc_cycle();
}

}  // namespace

PinsIm6100::PinsIm6100() : PinsPdp8() {
    _regs = new RegsIm6100(this);
    _devs = new pdp8::DevsPdp8();
    auto mems = new pdp8::MemsPdp8(12, _devs);
    _mems = mems;
    _cpmems = new pdp8::ControlPanel(mems);
}

void PinsIm6100::resetPins() {
    // Assert reset condition
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    // Synchronize OSC and State
    while (signal_xta() == LOW)
        osc_cycle();
    osc_cycle();
    // T4, T5, T6
    system_cycle(3);
    // T1
    system_cycle(5 * 100);
    negate_reset();
    system_cycle(5);
    toggle_runhlt();
    Signals::resetCycles();
    for (auto i = 0; i < 40; i++) {
        completeCycle(prepareCycle()->inject(03003));
    }
    printCycles();
    _regs->save();
}

bool PinsIm6100::isHalted() const {
    return digitalReadFast(PIN_RUN) == LOW;
}

pdp8::Signals *PinsIm6100::resumeCycle(uint16_t pc) const {
    auto s = Signals::put();
    s->addr = pc;
    s->getControl();
    s->getDirection();
    return s;
}

pdp8::Signals *PinsIm6100::prepareCycle() const {
    auto s = Signals::put();
    while (true) {
        if (signal_lxmar() != LOW) {
            assert_debug();
            s->getAddress();
            negate_debug();
        } else {
            assert_debug();
            if (s->getControl()) {
                s->getDirection();
                negate_debug();
                return s;
            }
            negate_debug();
        }
        osc_cycle();
    }
}

pdp8::Signals *PinsIm6100::completeCycle(pdp8::Signals *_s) const {
    auto s = static_cast<Signals *>(_s);
    if (s->read()) {
        if (s->mem()) {
            if (s->readMemory()) {
                s->data = _mems->read(s->addr);
            }
        } else if (s->dev()) {
            s->data = _devs->read(s->addr);
            s->outIoc(devs<pdp8::DevsPdp8>()->control(s->addr));
        } else if (s->cp()) {
            if (s->readMemory())
                s->data = _cpmems->read(s->addr);
        } else {
            s->data = regs<RegsIm6100>()->readSwitch();
        }
        assert_debug();
        s->outData();
        osc_cycle();
        s->inputMode();
        negate_debug();
    } else {
        assert_debug();
        s->getData();
        negate_debug();
        if (s->mem()) {
            if (s->writeMemory()) {
                _mems->write(s->addr, s->data);
            }
        } else if (s->dev()) {
            _devs->write(s->addr, s->data);
        } else if (s->cp()) {
            if (s->writeMemory()) {
                _cpmems->write(s->addr, s->data);
            }
        } else {
            regs<RegsIm6100>()->writeSwitch(s->data);
        }
    }
    Signals::nextCycle();
    auto n = Signals::put();
    *n = *s;  // copy address and control
    assert_debug();
    do {
        osc_cycle();
    } while (n->getControl());
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
