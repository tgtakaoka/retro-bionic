#include "pins_mc68hc05c0.h"
#include "debugger.h"
#include "devs_mc68hc05c0.h"
#include "inst_mc68hc05.h"
#include "mems_mc6805.h"
#include "regs_mc68hc05.h"
#include "signals_mc68hc05c0.h"

namespace debugger {
namespace mc68hc05c0 {

/**
 * MC68HC05C0 bus cycle.
 *       |--c1-|--c2-|--c3-|--c4-|--c1-|--c2-|--c3-|--c4-|--c1-|
 *      _|   __|   __|   __|   __|   __|   __|   __|   __|   __|
 * OSC1  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|
 *          \ ____\  \           \  \ ____\  \           \  \ __
 *   AS _____|     |_________________|     |_________________|
 *      ______________             _____________________________
 *  #RD               |___________|
 *      ______________________________________             _____
 *  #WR                                       |___________|
 *      ______                        __________________________
 * #LIR       |______________________|
 *      ______ _______________________ _______________________ _
 *   AH ______X_______________________X_______________________X_
 *              ______        ____      ______   ___________   _
 *   AD -------<__AL__>------<_in_>----<__AL__>-<___out_____>-<_
 */

namespace {

//   fOSC: min  DC, max 16MHz
// tOH/OL: min  60 ns; OSC1 pulse width
//   tOSC: min  62.5 ns; one-half oscillator period
//   tCYC: min 250 ns; fOSC/4
//   tASH: typ 2*tOSC; AS high width
// tADDSU: typ 2*tOSC; address setup to AS-
//  tADDH: typ   tOSC; address hold from AS-
// tVARWL: typ 3*tOSC; address valid to #RD-/#WR-
//    tRL: typ 4*tOSC; #RD low width
//  tRDSU: typ  10 ns; read data setup to #RD+
//   tRDH: typ   0 ns; read data hold from #RD+
//    tWL: typ 4*tOSC; #WR low width
//  tWDSU: typ 4*tOSC; write data setup to #WR+
//   tWDH: typ   tOSC; write data hold from #WR+

constexpr auto osc1_hi_ns = 28;  // 62.5

// Bus cycle delays.
enum Delay : uint8_t {
    osc1_lo_ns,
    c2_hi_addr,
    c3_lo_control,
    c3_hi_write,
    c4_lo_capture,
    c3_hi_internal,
    c3_hi_inject,
    c4_lo_read,
    c1_lo_input,
    c1_hi_ns,
    c3_hi_read,
    c4_lo_write,
    c4_hi_ns,
    DELAYS,
};

constexpr uint16_t DELAY_NS[] = {
        50,  // osc1_lo_ns 62.5
        20,  // c2_hi_addr 62.5
        28,  // c3_lo_control 62.5
        20,  // c3_hi_write 62.5
        30,  // c4_lo_capture 62.5
        20,  // c3_hi_internal 62.5
        4,   // c3_hi_inject 62.5
        35,  // c4_lo_read 62.5
        50,  // c1_lo_input 62.5
        62,  // c1_hi_ns 62.5
        0,   // c3_hi_read 62.5
        24,  // c4_lo_write 62.5
        15,  // c4_hi_ns 62.5
};
static_assert(sizeof(DELAY_NS) / sizeof(DELAY_NS[0]) == DELAYS);


inline void osc1_hi() {
    digitalWriteFast(PIN_OSC1, HIGH);
}

inline void osc1_lo() {
    digitalWriteFast(PIN_OSC1, LOW);
}

inline auto signal_as() {
    return digitalReadFast(PIN_AS);
}

void negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
}

constexpr uint8_t PINS_OPENDRAIN[] = {
        PIN_RESET,  // bi-directional
};

constexpr uint8_t PINS_LOW[] = {
        PIN_OSC1,
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_IRQ,
};

constexpr uint8_t PINS_PULLUP[] = {
        PIN_LIR,  // #LIR/MODE
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
        PIN_AS,
        PIN_RD,
        PIN_WR,
        PIN_PB0,
        PIN_PB1,
        PIN_PB4,
        PIN_PB5,
};

}  // namespace

template <uint8_t F>
inline void PinsMc68HC05C0::delay() {
    if constexpr (DELAY_NS[F] != 0)
        delayNanoseconds(DELAY_NS[F]);
}

void PinsMc68HC05C0::osc1CycleHi() {
    osc1_lo();
    delay<osc1_lo_ns>();
    osc1_hi();
}

void PinsMc68HC05C0::osc1Cycle() {
    osc1CycleHi();
    delayNanoseconds(osc1_hi_ns);
}

PinsMc68HC05C0::PinsMc68HC05C0() : _dummyIn(0) {
    auto regs = new mc68hc05::RegsMc68HC05(this);
    _regs = regs;
    _devs = new DevsMc68HC05C0(ACIA_BASE);
    _mems = new mc6805::MemsMc6805(this, regs, _devs, 16);
    _inst = new mc68hc05::InstMc68HC05();
}

void PinsMc68HC05C0::resetCpu() {
    // Assert reset condition
    pinsMode(PINS_OPENDRAIN, sizeof(PINS_OPENDRAIN), OUTPUT_OPENDRAIN, LOW);
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    // #LIR/MODE=H; select multiplexed bus mode.
    pinsMode(PINS_PULLUP, sizeof(PINS_PULLUP), INPUT_PULLUP);

    // #RESET low for at least one and one-half machine cycles; this holds
    // it for 10, of 4 OSC1 clocks each.
    for (auto i = 0; i < 4 * 10; ++i)
        osc1Cycle();
    negate_reset();
    // Wait for the first bus cycle
    while (signal_as() != LOW)
        osc1Cycle();
    osc1Cycle();

    Cycles::reset();
    _dummyIn = 0;
    // Inject dummy reset vector and wait for the first instruction fetch.
    _addr = mc68hc05::InstMc68HC05::RESET_VEC;
    _regs->reset();
}

void PinsMc68HC05C0::idle() {
    // MC68HC05C0 is fully static, so we can stop clock safely.
}

Signals *PinsMc68HC05C0::currCycle(uint16_t pc) const {
    auto s = SignalsMc68HC05C0::put();
    s->getControl();
    s->addr = pc ? pc : _addr;
    return s;
}

Signals *PinsMc68HC05C0::rawPrepareCycle() {
    auto s = SignalsMc68HC05C0::put();
    // c2
    osc1CycleHi();
    s->getAddr();
    _addr = s->addr;
    delay<c2_hi_addr>();
    // c3
    osc1_lo();
    delay<c3_lo_control>();
    s->getControl();
    return s;
}

Signals *PinsMc68HC05C0::completeCycle(Signals *signals) {
    // c3
    osc1_hi();
    auto s = static_cast<SignalsMc68HC05C0 *>(signals);
    if (s->write()) {
        // An IRQ pushes before reading its vector; a Dx fetch it discarded
        // must not leave the vector's low byte undriven
        _dummyIn = 0;
        s->getData();
        delay<c3_hi_write>();
        // c4
        osc1_lo();
        if (s->writeMemory()) {
            mems<mc6805::MemsMc6805>()->write<DevsMc68HC05C0>(s->addr, s->data);
            delay<c4_lo_write>();
        } else {
            delay<c4_lo_capture>();
        }
    } else if ((_dummyIn && --_dummyIn == 0) || is_internal(s->addr)) {
        // IRV is enabled and an internal read appears on the external bus;
        // so does an indexed, 16-bit offset (Dx) instruction's dummy read,
        // the third cycle after its fetch
        delay<c3_hi_internal>();
        // toggle_debug();
        s->getData();
        // toggle_debug();
        // c4
        osc1_lo();
        delay<osc1_lo_ns>();
    } else {
        if (s->readMemory()) {
            s->data = mems<mc6805::MemsMc6805>()->read<DevsMc68HC05C0>(s->addr);
            delay<c3_hi_read>();
        } else {
            delay<c3_hi_inject>();
        }
        // c4
        osc1_lo();
        s->outData();
        delay<c4_lo_read>();
    }
    if (s->fetch() && (s->data & 0xF0) == 0xD0)
        _dummyIn = 3;
    // c4
    osc1_hi();
    Cycles::next();
    delay<c4_hi_ns>();
    // c1
    osc1_lo();
    delay<c1_lo_input>();
    SignalsMc68HC05C0::inputMode();
    osc1_hi();
    delay<c1_hi_ns>();
    return s;
}

bool PinsMc68HC05C0::is_internal(uint16_t addr) const {
    if (addr < 0x40) {
        // One bit per address, MSB first; a clear bit is internal.
        static constexpr uint8_t IO_MAP[] = {
                0x88,  // $00-$07
                0x00,  // $08-$0F
                0x00,  // $10-$17
                0x0F,  // $18-$1F
                0xFF,  // $20-$27
                0xFF,  // $28-$2F
                0xFF,  // $30-$37
                0xFF,  // $38-$3F
        };
        return (IO_MAP[addr >> 3] & (0x80 >> (addr & 7))) == 0;
    }
    return addr < 0x240;
}

}  // namespace mc68hc05c0
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
