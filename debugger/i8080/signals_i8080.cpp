#include "signals_i8080.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_i8080.h"

namespace debugger {
namespace i8080 {

void Signals::getAddrLow() {
    addr = busRead(ADRL);
}

void Signals::getAddrMid() {
    addr |= busRead(ADRM);
}

void Signals::getAddrHigh() {
    addr |= busRead(ADRH);
}

void Signals::getDirection() {
    cntl() = busRead(CNTL);
}

bool Signals::read() const {
    return (cntl() & CNTL_DBIN) == 0;
}

bool Signals::write() const {
    return (cntl() & CNTL_WR) == 0;
}

bool Signals::memory() const {
    return true;
}

bool Signals::fetch() const {
    return false;
}

bool Signals::vector() const {
    return false;
}

void Signals::getData() {
    data = busRead(DATA);
}

void Signals::outData() const {
    busWrite(DATA, data);
    busMode(DATA, OUTPUT);
}

void Signals::inputMode() {
    busMode(DATA, INPUT);
}

void Signals::print() const {
    //                              01234567890123
    static constexpr char line[] = "R A=xxxx D=xx";
    static auto &buffer = *new CharBuffer(line);
    if (memory()) {
        buffer[0] = fetch() ? 'F' : (read() ? 'R' : (write() ? 'W' : ' '));
        buffer[2] = 'A';
    } else {
        buffer[0] = vector() ? 'A' : (read() ? 'R' : (write() ? 'W' : ' '));
        buffer[2] = 'I';
    }
    buffer.hex16(4, addr);
    buffer.hex8(11, data);
    cli.println(buffer);
}

}  // namespace i8080
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
