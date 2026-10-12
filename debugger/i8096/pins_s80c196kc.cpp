#include "pins_s80c196kc.h"
#include "debugger.h"
#include "signals_s80c196kc.h"

namespace debugger {
namespace s80c196kc {

// clang-format off
/**
 * S80C196KB/KC bus cycle: a state is two XTAL1 periods, a bus cycle four.
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
 * ATTENTION: unverified on hardware. The outputs follow XTAL1 through
 * CLKOUT, 20-110 ns after a rising edge, which can be longer than a
 * period here: each signal is sampled at the end of a low phase, and a
 * strobe's end is awaited, rather than counting edges.
 */
// clang-format on

namespace {

// ATTENTION: from the P8095BH's measurements, not the KC's: XTAL1 high and
// low 20 ns minimum.
constexpr auto xtal1_lo_ns = 20;
constexpr auto xtal1_hi_ns = 20;
// The address settles on AD 30 ns after ALE rises and holds until 50 ns
// after it falls (TLLAX, Tosc-35 ns).
constexpr auto addr_ns = 30;
// A write's data is stable Tosc-23 ns before #WR rises (TQVWH).
constexpr auto data_ns = 25;
// #RD or #WR rises at most ~130 ns after the rising XTAL1 edge ending it.
constexpr auto settle_ns = 150;
constexpr uint32_t settle_cycles = F_CPU / 1000000 * settle_ns / 1000;

inline void xtal1_lo() {
    digitalWriteFast(PIN_XTAL1, LOW);
}

inline void xtal1_hi() {
    digitalWriteFast(PIN_XTAL1, HIGH);
}

inline auto ale_asserted() {
    return digitalReadFast(PIN_ALE) != LOW;
}

inline auto reset_asserted() {
    return digitalReadFast(PIN_RESET) == LOW;
}

// A wait this long is no bus cycle's: RST, the watchdog or an illegal
// IDLPD holds #RESET low 16 states, ALE high.
constexpr auto reset_check = 8;

void xtal1_pulse() {
    xtal1_hi();
    delayNanoseconds(xtal1_hi_ns);
    xtal1_lo();
}

}  // namespace

void PinsS80C196KC::resetPins() {
    pinMode(PIN_INST, INPUT);
    // A rising NMI vectors to 203EH.
    pinMode(PIN_NMI, OUTPUT);
    digitalWriteFast(PIN_NMI, LOW);
    // #HOLD shares P1.7, a quasi-bidirectional port pin: a pull-up keeps
    // it negated without fighting a program that drives the port low.
    pinMode(PIN_HOLD, INPUT_PULLUP);
    pinMode(PIN_HLDA, INPUT);
    PinsI8096::resetPins();
}

void PinsS80C196KC::assertInt(uint8_t name) {
    if (name == i8096::INTR_NMI) {
        digitalWriteFast(PIN_NMI, HIGH);
    } else {
        PinsI8096::assertInt(name);
    }
}

void PinsS80C196KC::negateInt(uint8_t name) {
    if (name == i8096::INTR_NMI) {
        digitalWriteFast(PIN_NMI, LOW);
    } else {
        PinsI8096::negateInt(name);
    }
}

bool PinsS80C196KC::getControl(i8096::SignalsI8096 *s) const {
    return static_cast<s80c196kc::Signals *>(s)->getControl();
}

i8096::SignalsI8096 *PinsS80C196KC::startCycle(i8096::SignalsI8096 *s) {
    auto kc = static_cast<s80c196kc::Signals *>(s);
    noInterrupts();
    // completeCycle() leaves XTAL1 low. ALE high at the end of a low phase
    // stays high until the next rising edge, the address with it.
    for (auto n = 0;; ++n) {
        if (n >= no_bus_cycles)
            return noBusCycle(s);
        delayNanoseconds(xtal1_lo_ns);
        if (ale_asserted())
            break;
        if (n >= reset_check && reset_asserted())
            return cpuReset(s);
        xtal1_pulse();
    }
    delayNanoseconds(addr_ns);
    s->getAddr();
    // #RD or #WR falls a period after ALE; seen at the end of a low phase,
    // it stays low until the next rising edge.
    for (auto n = 0;; ++n) {
        if (n >= no_bus_cycles)
            return noBusCycle(s);
        xtal1_pulse();
        delayNanoseconds(xtal1_lo_ns);
        if (kc->getControl())
            break;
        if (n >= reset_check && reset_asserted())
            return cpuReset(s);
    }
    interrupts();
    return s;
}

// The CPU reset itself: clocked through the reset, it reads the CCB and
// fetches 2080H, as after the debugger's own.
i8096::SignalsI8096 *PinsS80C196KC::cpuReset(i8096::SignalsI8096 *s) {
    for (auto n = 0; reset_asserted(); ++n) {
        if (n >= no_bus_cycles)
            return noBusCycle(s);
        xtal1_pulse();
        delayNanoseconds(xtal1_lo_ns);
    }
    interrupts();
    return startAfterReset(startCycle(s));
}

// |low|: drive a read's data on AD0-AD7 only.
i8096::SignalsI8096 *PinsS80C196KC::completeCycle(
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
    // The rising edge ending #RD or #WR; a read's data is held until it
    // rises, and AD released before the next ALE.
    xtal1_hi();
    s80c196kc::Signals probe;
    const uint32_t start = ARM_DWT_CYCCNT;
    while (probe.getControl() && ARM_DWT_CYCCNT - start < settle_cycles)
        ;
    if (s->read())
        s->inputMode();
    xtal1_lo();
    interrupts();
    if (s->write() && s->writeMemory())
        writeBus(s);
    if (!idling())
        Cycles::next();
    return s;
}

}  // namespace s80c196kc
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
