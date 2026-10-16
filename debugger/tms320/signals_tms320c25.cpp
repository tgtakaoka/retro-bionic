#include "signals_tms320c25.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_tms320c25.h"

namespace debugger {
namespace tms320c25 {

namespace {

// 74AHCT157 select to output: 7.5 ns typical, 13 ns max at 5V
constexpr auto asel_delay_ns = 15;

}  // namespace

// ASEL rests low, which hands out A0-A3 and A8-A11; high gives A4-A7 and
// A12-A15. Both nibble pairs sit on GPIO7, so one read takes a pair.
void Signals::getAddr() {
    uint32_t p = PIN(AL);
    uint16_t a = busValue(AL, p) | busValue(AH, p);
    digitalWriteFast(PIN_ASEL, HIGH);
    delayNanoseconds(asel_delay_ns);
    p = PIN(AM);
    a |= busValue(AM, p) | busValue(AX, p);
    digitalWriteFast(PIN_ASEL, LOW);
    addr = a;
}

bool Signals::getControl() {
    cntl() = busRead(CNTL);
    return (cntl() & CNTL_STRB) == 0;
}

void Signals::getData() {
    data = busRead(DATA);
}

void Signals::outData() const {
    busWrite(DATA, data);
    busMode(DATA, OUTPUT);
}

void Signals::inputMode() const {
    busMode(DATA, INPUT);
}

bool Signals::strobe() const {
    return (cntl() & CNTL_STRB) == 0;
}

bool Signals::read() const {
    return (cntl() & CNTL_RW) != 0;
}

bool Signals::ps() const {
    return (cntl() & CNTL_PS) == 0;
}

bool Signals::ds() const {
    return (cntl() & CNTL_DS) == 0;
}

bool Signals::is() const {
    return (cntl() & CNTL_IS) == 0;
}

void Signals::print() const {
    //                              0123456789012345
    static constexpr char line[] = "PR A=xxxx D=xxxx";
    auto &buffer = Cycles::buffer();
    buffer.set(line);
    buffer[0] = ps() ? 'P' : (ds() ? 'D' : (is() ? 'I' : '-'));
    buffer[1] = read() ? 'R' : 'W';
    buffer.hex16(5, addr);
    buffer.hex16(12, data);
    cli.println(buffer);
}

}  // namespace tms320c25
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
