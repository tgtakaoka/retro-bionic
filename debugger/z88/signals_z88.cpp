#include "signals_z88.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_z88.h"

namespace debugger {
namespace z88 {

void Signals::getAddr() {
    addr = busRead(ADDR);
}

void Signals::getDirection() {
    rw() = digitalReadFast(PIN_RW);
#if Z88_DATA_MEMORY == 1
    dm() = digitalReadFast(PIN_DM);
#endif
}

void Signals::getData() {
    data = busRead(DATA);
}

void Signals::outData() {
    // This order is necessary to reduce noise....
    busMode(DATA, OUTPUT);
    busWrite(DATA, data);
}

void Signals::print() const {
    static constexpr char line[] =
#if Z88_DATA_MEMORY == 1
            " "
#endif
            "R A=xxxx D=xx";
    static auto &buffer = *new CharBuffer(line);
#if Z88_DATA_MEMORY == 1
    buffer[0] = dm() ? 'P' : 'D';
#endif
    buffer[0 + Z88_DATA_MEMORY] = read() ? 'R' : 'W';
    buffer.hex16(4 + Z88_DATA_MEMORY, addr);
    buffer.hex8(11 + Z88_DATA_MEMORY, data);
    cli.println(buffer);
}

}  // namespace z88
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=
