#include "signals_tlcs90.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_tlcs90.h"

namespace debugger {
namespace tlcs90 {

void Signals::getAddrHigh() {
    addr = busRead(ADM) | busRead(ADH);
}

void Signals::getAddrLow() {
    addr |= busRead(ADL);
}

void Signals::getDirection() {
    cntl() = busRead(CNTL);
}

bool Signals::read() const {
    return (cntl() & CNTL_RD) == 0;
}

bool Signals::write() const {
    return (cntl() & CNTL_WR) == 0;
}

Signals *Signals::current() {
    auto s = put();
    s->getAddrHigh();
    s->getAddrLow();
    s->getDirection();
    return s;
}

void Signals::getData() {
    data = busRead(DB);
}

void Signals::setData() const {
    busWrite(DB, data);
}

void Signals::outputMode() {
    busMode(DB, OUTPUT);
}

void Signals::inputMode() {
    busMode(DB, INPUT);
}

void Signals::print() const {
    LOG_MATCH(cli.printDec(pos(), -4));
    //                              0123456789012
    static constexpr char line[] = "R A=xxxx D=xx";
    static auto &buffer = *new CharBuffer(line);
    buffer[0] = read() ? 'R' : (write() ? 'W' : ' ');
    buffer.hex16(4, addr);
    buffer.hex8(11, data);
    cli.println(buffer);
}

}  // namespace tlcs90
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
