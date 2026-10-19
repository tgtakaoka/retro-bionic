#include "pins_i80296sa.h"
#include "debugger.h"
#include "signals_s80296sa.h"

namespace debugger {
namespace s80296sa {

// clang-format off
/**
 * S80C196NP and S80296SA bus cycle, in 64-Kbyte mode: a bus cycle of four
 * XTAL1 periods, stretched by wait states.
 *          __    __    __    __    __    __    __    __    __
 * XTAL1 __|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |
 *        __                      __                      __
 *   ALE /  \____________________/  \____________________/  \___
 *       ___________       ______________________________________
 *   #RD            \_____/
 *       _____________________________________       ____________
 *   #WR                                      \_____/
 *       ___ _______ _______ _______ ______ _______ _______ ____
 *    AD ___X_addr__X_______X_data__X_addr_X_______X_data__X____
 *
 * The CCB0 fetch takes three wait states on the S80C196NP and 15 on the
 * S80296SA; CCB0 and BUSCON5 leave the rest with none.
 *
 * The S80296SA's outputs follow XTAL1 3-50 ns after a rising edge, and it
 * holds the address only 1 ns after ALE falls: ALE and the strobes are
 * sampled at the end of a low phase, the address read while ALE is high,
 * and the strobe's end awaited rather than counted. Its reset, an RST's
 * too, holds ALE high before the CCB0 fetch's own ALE.
 *
 * ATTENTION: seen on an S80296SA only; the S80C196NP is unverified.
 */
// clang-format on

namespace {

// ATTENTION: from the P8095BH's measurements: XTAL1 high and low 20 ns
// minimum on the S80C196NP, 35% of a period on the S80296SA.
constexpr auto xtal1_lo_ns = 20;
constexpr auto xtal1_hi_ns = 20;
// The address settles on AD while ALE is high (TAVLL).
constexpr auto addr_ns = 30;
// A write's data is stable before #WR rises (TQVWH).
constexpr auto data_ns = 25;
// The S80296SA's CCB0 fetch: 15 wait states of two XTAL1 periods each.
constexpr auto max_strobe = 64;

inline void xtal1_lo() {
    digitalWriteFast(PIN_XTAL1, LOW);
}

inline void xtal1_hi() {
    digitalWriteFast(PIN_XTAL1, HIGH);
}

inline auto ale_asserted() {
    return digitalReadFast(PIN_ALE) != LOW;
}

void xtal1_pulse() {
    xtal1_hi();
    delayNanoseconds(xtal1_hi_ns);
    xtal1_lo();
}

}  // namespace

void PinsI80296SA::resetPins() {
    pinMode(PIN_INST, INPUT);
    // A rising NMI vectors to 203EH.
    pinMode(PIN_NMI, OUTPUT);
    digitalWriteFast(PIN_NMI, LOW);
    // #HOLD shares P2.5, a port pin: a pull-up keeps it negated without
    // fighting a program that drives the port low.
    pinMode(PIN_HOLD, INPUT_PULLUP);
    pinMode(PIN_HLDA, INPUT);
    PinsI8096::resetPins();
}

void PinsI80296SA::assertInt(uint8_t name) {
    if (name == i8096::INTR_NMI) {
        digitalWriteFast(PIN_NMI, HIGH);
    } else {
        PinsI8096::assertInt(name);
    }
}

void PinsI80296SA::negateInt(uint8_t name) {
    if (name == i8096::INTR_NMI) {
        digitalWriteFast(PIN_NMI, LOW);
    } else {
        PinsI8096::negateInt(name);
    }
}

bool PinsI80296SA::getControl(i8096::SignalsI8096 *s) const {
    return static_cast<s80296sa::Signals *>(s)->getControl();
}

i8096::SignalsI8096 *PinsI80296SA::startCycle(i8096::SignalsI8096 *s) {
    auto sa = static_cast<s80296sa::Signals *>(s);
    noInterrupts();
    // completeCycle() leaves XTAL1 low. ALE high at the end of a low phase
    // stays high until the next rising edge, the address with it.
    for (auto n = 0;; ++n) {
        if (n >= no_bus_cycles)
            return noBusCycle(s);
        delayNanoseconds(xtal1_lo_ns);
        if (ale_asserted())
            break;
        xtal1_pulse();
    }
    delayNanoseconds(addr_ns);
    s->getAddr();
    // #RD or #WR falls one or two periods after ALE; seen at the end of a
    // low phase, it stays low until the next rising edge. The address is
    // the last ALE's: a reset's ALE comes before the CCB0 fetch's.
    for (auto n = 0;; ++n) {
        if (n >= no_bus_cycles)
            return noBusCycle(s);
        xtal1_pulse();
        delayNanoseconds(xtal1_lo_ns);
        if (sa->getControl())
            break;
        if (ale_asserted()) {
            delayNanoseconds(addr_ns);
            s->getAddr();
        }
    }
    interrupts();
    return s;
}

// |low|: drive a read's data on AD0-AD7 only.
i8096::SignalsI8096 *PinsI80296SA::completeCycle(
        i8096::SignalsI8096 *s, bool low) {
    if (s->read()) {
        if (s->readMemory())
            s->data = readBus(s);
        if (low) {
            s->outLow();
        } else {
            s->outData();
        }
    } else if (s->write()) {
        delayNanoseconds(data_ns);
        s->getData();
    }
    noInterrupts();
    // #RD or #WR stays low through the wait states: clock until it rises,
    // a read's data held until then and AD released before the next ALE.
    s80296sa::Signals probe;
    for (auto n = 0; n < max_strobe; ++n) {
        xtal1_pulse();
        delayNanoseconds(xtal1_lo_ns);
        if (!probe.getControl())
            break;
    }
    if (s->read())
        s->inputMode();
    interrupts();
    if (s->write() && s->writeMemory())
        writeBus(s);
    if (!idling())
        Cycles::next();
    return s;
}

}  // namespace s80296sa
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
