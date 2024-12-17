#include "signals_z8.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_z86.h"

namespace debugger {
namespace z8 {

void Signals::getAddr() {
    addr = busRead(ADDR);
    rw() = digitalReadFast(PIN_RW);
    fetch() = false;
}

void Signals::getData() {
    data = busRead(DATA);
}

void Signals::outData() {
    busMode(DATA, OUTPUT);
    busWrite(DATA, data);
}

void Signals::print() const {
    //                              0123456789012
    static constexpr char line[] = "R A=xxxx D=xx";
    static auto &buffer = *new CharBuffer(line);
    buffer[0] = read() ? 'R' : 'W';
    buffer.hex16(4, addr);
    buffer.hex8(11, data);
    cli.println(buffer);
}

}  // namespace z8
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
