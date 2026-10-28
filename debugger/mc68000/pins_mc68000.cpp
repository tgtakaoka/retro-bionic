#include "pins_mc68000.h"
#include "debugger.h"
#include "devs_mc68000.h"
#include "digital_bus.h"
#include "mems_mc68000.h"
#include "regs_mc68000.h"
#include "signals_mc68000.h"

namespace debugger {
namespace mc68000 {

namespace {

constexpr auto ADDR_MASK = 0xFFFFFF;  // A0-A23

// 74HCS153 select to output: 15ns typical at 5V; the MC68HC16 board's
// outputs were seen to follow within one 10ns capture sample.
constexpr auto asel_delay_ns = 15;

const uint8_t PINS_LOW[] = {
        PIN_RESET,
        PIN_HALT,
        PIN_ASEL0,
        PIN_ASEL1,
};

const uint8_t PINS_HIGH[] = {
        PIN_DTACK,
        PIN_BERR,
        PIN_IPL0,
        PIN_IPL1,
        PIN_IPL2,
};

const uint8_t PINS_INPUT[] = {
        PIN_AS,
        PIN_RW,
        PIN_UDS,
        PIN_LDS,
        PIN_D0,
        PIN_D1,
        PIN_D2,
        PIN_D3,
        PIN_D4,
        PIN_D5,
        PIN_D6,
        PIN_D7,
        PIN_D8,
        PIN_D9,
        PIN_D10,
        PIN_D11,
        PIN_D12,
        PIN_D13,
        PIN_D14,
        PIN_D15,
        PIN_AL0,
        PIN_AL1,
        PIN_AL2,
        PIN_AL3,
        PIN_AH0,
        PIN_AH1,
        PIN_AH2,
        PIN_AH3,
};

inline uint16_t bothLanes(uint8_t v) {
    return static_cast<uint16_t>(v) << 8 | v;
}

}  // namespace

PinsMc68000::PinsMc68000()
    : PinsMc68000Base(Signals::UDS | Signals::LDS) {
    _devs = new DevsMc68000(ADDR_MASK);
    _mems = new MemsMc68000(ADDR_MASK);
    _regs = new RegsMc68000(this, mems<MemsMc68000>());
}

void PinsMc68000::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    resetCpu();
}

// MODE, on P37 in phase 10, is a strap: high or open for a 16-bit bus, low
// (JP1 closed on the FN board) for an MC68HC001's 8-bit bus. The P and PS
// boards tie it high. It must be known before the first cycle, whose A0
// comes from the strobes on a 16-bit bus and from the mux on an 8-bit one.
void PinsMc68000::readStraps() {
    digitalWriteFast(PIN_ASEL1, HIGH);  // 10
    delayNanoseconds(asel_delay_ns);
    const uint32_t sel10 = PIN(ADDR);
    digitalWriteFast(PIN_ASEL1, LOW);  // 00
    _bus8 = (sel10 & (UINT32_C(1) << 19)) == 0;
}

// ASEL1:ASEL0 rests at 00. Stepped in gray order 00, 01, 11, 10 the muxes
// hand out A0/A16, A4/A20, A8/-, A12/FC in turn, one select changing per
// step, and the last step brings it back to 00. The CPU holds the address
// until #DTACK. ATTENTION: unmeasured. The walk takes about four select
// delays, after #AS is first seen low.
void PinsMc68000::getAddr(Signals *s) const {
    const uint32_t sel00 = PIN(ADDR);
    digitalWriteFast(PIN_ASEL0, HIGH);  // 01
    delayNanoseconds(asel_delay_ns);
    const uint32_t sel01 = PIN(ADDR);
    digitalWriteFast(PIN_ASEL1, HIGH);  // 11
    delayNanoseconds(asel_delay_ns);
    const uint32_t sel11 = PIN(ADDR);
    digitalWriteFast(PIN_ASEL0, LOW);  // 10
    delayNanoseconds(asel_delay_ns);
    const uint32_t sel10 = PIN(ADDR);
    digitalWriteFast(PIN_ASEL1, LOW);  // 00
    const auto lo = [](uint32_t v) { return v & 0xF; };
    const auto hi = [](uint32_t v) { return (v >> 16) & 0xF; };
    s->addr = lo(sel00) | lo(sel01) << 4 | lo(sel11) << 8 | lo(sel10) << 12 |
              hi(sel00) << 16 | hi(sel01) << 20;
    s->setFc(hi(sel10) & 7, false);
    // A read asserts its strobes with #AS; the walk gave them time to
    // settle. A write's come later, for getLanes().
    if (s->read()) {
        s->cntl() = busRead(CNTL);
        getLanes(s);
    }
}

// Both strobes move a word; #UDS alone the even byte, #LDS alone the odd
// one. On an 8-bit bus #LDS strobes every byte and A0 is on the bus.
void PinsMc68000::getLanes(Signals *s) const {
    if (_bus8)
        return;
    const auto uds = (s->cntl() & Signals::UDS) == 0;
    const auto lds = (s->cntl() & Signals::LDS) == 0;
    s->addr = (s->addr & ~UINT32_C(1)) | ((lds && !uds) ? 1 : 0);
    s->setFc(s->fc(), uds && lds);
}

void PinsMc68000::getData(Signals *s) const {
    const uint16_t data = busRead(D);
    if (s->wordAccess()) {
        s->data = data;
    } else if (_bus8 || (s->cntl() & Signals::LDS) == 0) {
        s->data = data & 0xFF;
    } else {
        s->data = data >> 8;
    }
}

// A byte goes out on both lanes: the CPU takes the one it strobed.
void PinsMc68000::outData(const Signals *s) const {
    busWriteFast(D, s->wordAccess() ? s->data : bothLanes(s->data));
    busMode(D, OUTPUT);
}

void PinsMc68000::inputMode() const {
    busMode(D, INPUT);
}

// The level on #IPL2-#IPL0, active low.
void PinsMc68000::setIpl() const {
    busWriteFast(IPL, ~iplLevel() & 7);
}

}  // namespace mc68000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
