#include "pins_mc68008.h"
#include "debugger.h"
#include "devs_mc68000.h"
#include "digital_bus.h"
#include "mems_mc68000.h"
#include "regs_mc68000.h"
#include "signals_mc68000.h"

namespace debugger {
namespace mc68000 {

namespace {

constexpr auto ADDR_MASK_P = 0x0FFFFF;   // MC68008P: A0-A19
constexpr auto ADDR_MASK_FN = 0x3FFFFF;  // MC68008FN: A0-A21

// ATTENTION: unmeasured. 74AHCT157 select to output, 7.5ns typical at 5V.
constexpr auto asel_delay_ns = 15;

const uint8_t PINS_LOW[] = {
        PIN_RESET,
        PIN_HALT,
        PIN_ASEL,
};

const uint8_t PINS_HIGH[] = {
        PIN_DTACK,
        PIN_BERR,
        PIN_IPL20,
        PIN_IPL1,
};

const uint8_t PINS_INPUT[] = {
        PIN_AS,
        PIN_RW,
        PIN_DS,
        PIN_D0,
        PIN_D1,
        PIN_D2,
        PIN_D3,
        PIN_D4,
        PIN_D5,
        PIN_D6,
        PIN_D7,
        PIN_AL0,
        PIN_AL1,
        PIN_AL2,
        PIN_AL3,
        PIN_AL4,
        PIN_AL5,
        PIN_AL6,
        PIN_AL7,
        PIN_AM0,
        PIN_AM1,
        PIN_AM2,
        PIN_AM3,
        PIN_AH0,
        PIN_AH1,
        PIN_AH2,
        PIN_AH3,
        PIN_FC0,
        PIN_FC1,
        PIN_FC2,
};

}  // namespace

PinsMc68008::PinsMc68008() : PinsMc68000Base(Signals::LDS) {
    _devs = new DevsMc68000(ADDR_MASK_P);
    _mems = new MemsMc68000(ADDR_MASK_P);
    _regs = new RegsMc68000(this, mems<MemsMc68000>());
}

void PinsMc68008::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    resetCpu();
}

// Parked at the fetch of ORG_PARK, 0FF000H: A16-A17 are 11 and A20-A21
// 00. The P board's P34-P35 carry A16-A17 under either select; the FN
// board's switch to A20-A21. P36-P37 are left out: on the FN board the
// second 157 has no A22-A23 for them. Its 22 address lines then move the
// ACIA, which is the board's wiring: set only when the board changes.
void PinsMc68008::detectBoard() {
    const uint32_t sel0 = PIN(ADDR);
    digitalWriteFast(PIN_ASEL, HIGH);
    delayNanoseconds(asel_delay_ns);
    const uint32_t sel1 = PIN(ADDR);
    digitalWriteFast(PIN_ASEL, LOW);
    const auto fn = ((sel0 >> 16) & 3) != ((sel1 >> 16) & 3);
    if (fn == _fn)
        return;
    _fn = fn;
    const auto mask = fn ? ADDR_MASK_FN : ADDR_MASK_P;
    mems<MemsMc68000>()->setAddrMask(mask);
    devs<DevsMc68000>()->setAddrMask(mask);
}

// ASEL rests at 0. One read of GPIO6 gives A0-A7, and one of GPIO7 per
// select A8-A11 and A16-A19 with FC2:FC0, then A12-A15 and, on the FN
// board, A20-A21. The CPU holds the address until #DTACK.
void PinsMc68008::getAddr(Signals *s) const {
    const uint32_t al = busRead(AL);
    const uint32_t sel0 = PIN(ADDR);
    digitalWriteFast(PIN_ASEL, HIGH);
    delayNanoseconds(asel_delay_ns);
    const uint32_t sel1 = PIN(ADDR);
    digitalWriteFast(PIN_ASEL, LOW);
    uint32_t addr = al | (sel0 & 0xF) << 8 | (sel1 & 0xF) << 12 |
                    ((sel0 >> 16) & 0xF) << 16;
    if (_fn)
        addr |= ((sel1 >> 16) & 3) << 20;
    s->addr = addr;
    s->setFc((sel0 >> 10) & 7, false);
}

void PinsMc68008::getData(Signals *s) const {
    s->data = busRead(D);
}

void PinsMc68008::outData(const Signals *s) const {
    busWrite(D, s->data);
    busMode(D, OUTPUT);
}

void PinsMc68008::inputMode() const {
    busMode(D, INPUT);
}

// The level on #IPL2-#IPL0, active low. #IPL2 and #IPL0 are tied, so
// levels 2, 5 and 7 are all there is.
void PinsMc68008::setIpl() const {
    const auto level = iplLevel();
    digitalWriteFast(PIN_IPL20, (level & 5) ? LOW : HIGH);
    digitalWriteFast(PIN_IPL1, (level & 2) ? LOW : HIGH);
}

}  // namespace mc68000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
