#include "pins_hd6120.h"
#include "debugger.h"
#include "devs_pdp8.h"
#include "inst_pdp8.h"
#include "mems_pdp8.h"
#include "regs_hd6120.h"
#include "signals_hd6120.h"

namespace debugger {
namespace hd6120 {

// clang-format off
/**
 * HD6120 bus cycle.                                                     |  optional |
 *            _____       _____       _____       _____       _____      |_____      |__
 *  STATES __| T1H |_T1L_| T2H |_T2L_| T3H |_T3L_| T4H |_T4L_| T5H |_T5L_| T6H |_T6L_|
 *           __    __    __    __    __    __    __    __    __    __    __    __    __
 *   OSCIN _|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |
 *             \     \  \           \     \                                \  \  \
 *           ________|   v           v     |      __________________________|__v__|_____
 *  #LXxAR _/        \___|_________________|_____/__________________________|_____|__/
 *         ______________|                 |________________________________|     |_____
 * #xxxSEL               \_________________/                                \_____/
 *         ____________________             ____________________________________________
 *   #READ                     \___________/
 *         ________________________________________________________________        _____
 *  #WRITE                                                                 \______/
 *         __            ________________________
 *    #OUT __\__________/              RD        \____________________________WR________
 *                 _____             ______                                 ______
 *      DX -------<__AD_>-----------<__RD__>-------------------------------<__WR__>-----
 *         __________________________v_________________________________________v________
 *     ACK _________________________/ \_______________________________________/ \_______
 *         _______________                                                  ____________
 * #IFETCH                \________________________________________________/_________/
 *         _____________ v _____________________________________________________________
 * #xxxREQ _____________\_/_____________________________________________________________
 *
 * 120 ns ; OSC- to #LXMAR-
 * 135 ns ; OSC- to #LXMAR+
 * 152 ns ; OSC+ to #MEM/CPSEL-
 * 160 ns ; OSC+ to #MEM/CPSEL+
 * 152 ns ; OSC- to #MEM/CPSEL-
 * 136 ns ; OSC- to #MEM/CPSEL+
 */
// clang-format on

namespace {
// HD6120A
//   fOSC: min 0 MHz, max 5.71 MHz

constexpr auto osc_hi_ns = 150;  // 87.5
constexpr auto osc_lo_ns = 150;  // 87.5

inline void osc_hi() {
    digitalWriteFast(PIN_OSCIN, HIGH);
}

inline void osc_lo() {
    digitalWriteFast(PIN_OSCIN, LOW);
}

auto run_negated() {
    return digitalReadFast(PIN_RUN) != LOW;
}

void negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
}

constexpr uint8_t PINS_LOW[] = {
        PIN_RESET,
        PIN_OSCIN,
        // The followings are driven by open-drain inverter buffer.
        PIN_SKIP,
        PIN_C0,
        PIN_C1,
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_ACK,
        PIN_CPREQ,
        PIN_INTREQ,
        PIN_DMAREQ,
        PIN_RUNHLT,
        PIN_STRTUP,
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
        PIN_EMA2,
        PIN_EMA1,
        PIN_EMA0,
        PIN_LXMAR,
        PIN_IFETCH,
        PIN_DATAF,
        PIN_RUN,
        PIN_INTGNT,
        PIN_DMAGNT,
        PIN_MEMSEL,
        PIN_LXPAR,
        PIN_LXDAR,
        PIN_READ,
        PIN_WRITE,
        PIN_IOCLR,
};

inline void osc_cycle() {
    osc_hi();
    delayNanoseconds(osc_hi_ns);
    osc_lo();
    delayNanoseconds(osc_lo_ns);
}

}  // namespace

PinsHd6120::PinsHd6120() : PinsPdp8() {
    _regs = new RegsHd6120(this);
    _devs = new pdp8::DevsPdp8();
    auto mems = new pdp8::MemsPdp8(15);
    _mems = mems;
    _cpmems = new pdp8::ControlPanel(mems);
}

void PinsHd6120::resetPins() {
    // Assert reset condition
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    for (auto i = 0; i < 50; i++)
        osc_cycle();
    negate_reset();
    while (run_negated())
        osc_cycle();
    Signals::resetCycles();
    _regs->reset();
    prepareCycle();
    _regs->save();
}

pdp8::Signals *PinsHd6120::resumeCycle(uint16_t pc) const {
    auto s = Signals::put();
    s->clear();
    s->addr = pc;
    s->getControl();
    s->getDirection();
    return s;
}

pdp8::Signals *PinsHd6120::prepareCycle() const {
    auto s = Signals::put();
    if (!s->getControl()) {
        do {
            // assert_debug();
            s->getAddress();
            // negate_debug();
            if (s->getDirection()) {
                return s;  // OSR/WSR
            }
            osc_cycle();
        } while (!s->getControl());
    }
    // assert_debug();
    while (true) {
        if (s->getDirection()) {
            // negate_debug();
            return s;  // READ/WRITE
        }
        osc_cycle();
    }
}

pdp8::Signals *PinsHd6120::completeCycle(pdp8::Signals *_s) const {
    auto s = static_cast<Signals *>(_s);
bus_cycle:
    if (s->read()) {
        if (s->mem()) {
            if (s->readMemory()) {
                s->data = _mems->read(s->addr);
                if (s->fetch() && s->data == pdp8::InstPdp8::HLT)
                    return nullptr;
            }
        } else if (s->dev()) {
            if (_devs->isSelected(s->addr))
                s->data = _devs->read(s->addr);
        } else if (s->cp()) {
            if (s->readMemory())
                s->data = _cpmems->read(s->addr);
        } else {
            s->data = regs<RegsHd6120>()->readSwitch();
        }
        // assert_debug();
        s->outData();
        osc_cycle();
        s->inputMode();
        // negate_debug();
    } else {
        // assert_debug();
        s->getData();
        // negate_debug();
        if (s->mem()) {
            if (s->writeMemory()) {
                _mems->write(s->addr, s->data);
            }
        } else if (s->dev()) {
            if (_devs->isSelected(s->addr)) {
                _devs->write(s->addr, s->data);
                // assert_debug();
                s->outIoc(devs<pdp8::DevsPdp8>()->control(s->addr));
                // negate_debug();
            } else {
                constexpr auto WRITE = IOC_SKIP | IOC_C0 | IOC_C1;
                s->outIoc(WRITE);
            }
        } else if (s->cp()) {
            if (s->writeMemory()) {
                _cpmems->write(s->addr, s->data);
            }
        } else {
            regs<RegsHd6120>()->writeSwitch(s->data);
        }
    }
    Signals::nextCycle();
    auto n = Signals::put();
    *n = *s;  // copy address and control
    // assert_debug();
    do {
        osc_cycle();
    } while (n->getDirection());
    // negate_debug();
    s->resetIoc();
    // assert_debug();
    while (n->getControl()) {
        if (n->getDirection()) {
            s = n;
            // negate_debug();
            goto bus_cycle;
        }
        osc_cycle();
    }
    // negate_debug();
    return s;
}

}  // namespace hd6120
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
