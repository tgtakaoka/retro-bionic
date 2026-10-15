#include "pins_p8095bh.h"
#include "debugger.h"
#include "signals_p8095bh.h"

namespace debugger {
namespace p8095bh {

void PinsP8095BH::resetPins() {
    pinMode(PIN_PWM, INPUT);
    pinMode(PIN_ACH4, INPUT);
    pinMode(PIN_ACH5, INPUT);
    pinMode(PIN_ACH6, INPUT);
    // EXTINT is low again: a source's next assert gives the CPU an edge.
    _extintAsserted = _nmiAsserted = false;
    PinsI8096::resetPins();
}

// EXTINT acts on a rising edge. A halt asserting or negating INTR_NMI
// drops it until the next bus cycle ends: the halt gets an edge though the
// USART holds EXTINT, and so does a USART request the halt's entry took.
void PinsP8095BH::assertInt(uint8_t name) {
    if (name == i8096::INTR_NMI) {
        _nmiAsserted = true;
        digitalWriteFast(PIN_EXTINT, LOW);
    } else {
        _extintAsserted = true;
    }
}

void PinsP8095BH::negateInt(uint8_t name) {
    if (name == i8096::INTR_NMI) {
        _nmiAsserted = false;
        digitalWriteFast(PIN_EXTINT, LOW);
    } else {
        _extintAsserted = false;
    }
}

void PinsP8095BH::restoreExtint() {
    const auto high = _extintAsserted || _nmiAsserted;
    digitalWriteFast(PIN_EXTINT, high ? HIGH : LOW);
}

bool PinsP8095BH::getControl(i8096::SignalsI8096 *s) const {
    return static_cast<p8095bh::Signals *>(s)->getControl();
}

}  // namespace p8095bh
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
