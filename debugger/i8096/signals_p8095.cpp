#include "signals_p8095.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_p8095.h"

namespace debugger {
namespace p8095 {

bool Signals::addrValid() const {
    return digitalReadFast(PIN_ADV) == LOW;
}

void Signals::getAddr() {
    addr = busRead(AD);
}

bool Signals::getControl() {
    constexpr auto BUS_ACTIVE = CNTL_RD | CNTL_WR;
    cntl() = busRead(CNTL);
    return (cntl() & BUS_ACTIVE) != BUS_ACTIVE;
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

bool Signals::fetch() const {
    return (cntl() & (CNTL_ADV | CNTL_RD)) == 0;
}

bool Signals::read() const {
    return (cntl() & (CNTL_ADV | CNTL_RD)) == 0;
}

bool Signals::write() const {
    return (cntl() & (CNTL_ADV | CNTL_WR)) == 0;
}

void Signals::print() const {
    cli.print(readMemory() ? ' ' : 'i');
    cli.print(writeMemory() ? ' ' : 'c');
    cli.print(' ');
    cli.printDec(pos(), -8);
    //                              0123456789012
    static constexpr char line[] = "W A=xxxx D=xx";
    static auto &buffer = *new CharBuffer(line);
    if (fetch()) {
        buffer[0] = 'I';
    } else if (read()) {
        buffer[0] = 'R';
    } else if (write()) {
        buffer[0] = 'W';
    }
    buffer.hex16(4, addr);
    buffer.hex8(11, data);
    cli.println(buffer);
}

}  // namespace p8095
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
