#include "signals_i8085.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_i8085.h"

namespace debugger {
namespace i8085 {

void Signals::getAddress() {
    s() = busRead(S);
    iom() = digitalReadFast(PIN_IOM);
    addr = busRead(ADDR);
}

void Signals::getDirection() {
    //    s() = busRead(S);
    rd() = digitalReadFast(PIN_RD);
    wr() = digitalReadFast(PIN_WR);
    inta() = digitalReadFast(PIN_INTA);
}

void Signals::getData() {
    data = busRead(AD);
}

void Signals::print() const {
    //                               01234567890123
    static constexpr char line[] = " R A=xxxx D=xx";
    static auto &buffer = *new CharBuffer(line);
    if (memory()) {
        buffer[0] = 'M';
        buffer[1] = fetch() ? 'F' : (read() ? 'R' : (write() ? 'W' : ' '));
    } else {
        buffer[0] = 'I';
        buffer[1] = vector() ? 'A' : (read() ? 'R' : (write() ? 'W' : ' '));
    }
    buffer.hex16(5, addr);
    buffer.hex8(12, data);
    cli.println(buffer);
}

}  // namespace i8085
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
