#include "signals_tms9900.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_tms9900.h"

namespace debugger {
namespace tms9900 {

void SignalsTms9900::getAddress() {
    constexpr auto delay = 2;
    // SEL0 low and high each select half of the address lines. It rests
    // high, so that half is read first and put away while the other one
    // settles.
    const uint32_t sel1 = PIN(ADR2);
    digitalWriteFast(PIN_SEL0, LOW);
    uint16_t a = busValue(ADR2, sel1) | busValue(ADR3, sel1);
    delayNanoseconds(delay);
    const uint32_t sel0 = PIN(ADR0);
    digitalWriteFast(PIN_SEL0, HIGH);
    addr = a | busValue(ADR0, sel0) | busValue(ADR1, sel0);
}

void SignalsTms9900::getControl() {
    // CNTL_MEMEN is active low
    // CNTL_DBIN is active high
    // CNTL_IAQ is active high
    cntl() = busRead(CNTL) | CNTL_16BIT;
}

void SignalsTms9900::getData() {
    data = busRead(DATA);
}

void SignalsTms9900::outData() const {
    busWrite(DATA, data);
    busMode(DATA, OUTPUT);
}

void SignalsTms9900::inputMode() const {
    busMode(DATA, INPUT);
}

}  // namespace tms9900
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
