#include "signals_mn1613.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_mn1613.h"

namespace debugger {
namespace mn1613 {

void Signals::getAddress() {
    addr = busRead(ADDR) | busRead(EA);
}

void Signals::getControl() {
    cntl() = busRead(CNTL);
}

void Signals::getData() {
    data = busRead(DATA);
}

void Signals::getFetch() {
    if (digitalReadFast(PIN_FSYC) != LOW)
        cntl() |= CNTL_FETCH;
}

void Signals::outData() const {
    busWrite(DATA, data);
    busMode(DATA, OUTPUT);
}

void Signals::inputMode() {
    busMode(DATA, INPUT);
}

bool Signals::memory() const {
    return (cntl() & CNTL_IOP) == 0;
}

bool Signals::read() const {
    return (cntl() & CNTL_WRT) == 0;
}

bool Signals::write() const {
    return (cntl() & CNTL_WRT) != 0;
}

bool Signals::fetch() const {
    return (cntl() & CNTL_FETCH) != 0;
}

void Signals::print() const {
    // cli.printDec(pos(), -4);
    //                              0123456789012345
    static constexpr char line[] = "R A=xxxxx D=xxxx";
    static auto &buffer = *new CharBuffer(line);
    if (fetch()) {
        buffer[0] = 'F';
    } else if (read()) {
        buffer[0] = 'R';
    } else {
        buffer[0] = 'W';
    }
    if (memory()) {
        buffer[2] = 'A';
    } else {
        buffer[2] = 'I';
    }
    buffer.hex20(4, addr);
    buffer.hex16(12, data);
    cli.println(buffer);
}

}  // namespace mn1613
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
