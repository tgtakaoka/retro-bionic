#include "signals_i8051.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_i8051.h"

namespace debugger {
namespace i8051 {

void Signals::getAddress() {
    addr = busRead(AD);
}

void Signals::getControl() {
    cntl() = busRead(CNTL);
}

void Signals::getData() {
    data = busRead(DB);
}

bool Signals::read() const {
    return (cntl() & CNTL_RD) == 0;
}

bool Signals::write() const {
    return (cntl() & CNTL_WR) == 0;
}

bool Signals::fetch() const {
    return (cntl() & CNTL_PSEN) == 0;
}

void Signals::clearFetch() {
    cntl() |= CNTL_PSEN;
}

void Signals::print() const {
    //                              0123456789010
    static constexpr char line[] = "R A=xxxx D=xx";
    static auto &buffer = *new CharBuffer(line);
    buffer[0] = fetch() ? 'P' : (read() ? 'R' : (write() ? 'W' : ' '));
    buffer.hex16(4, addr);
    buffer.hex8(11, data);
    cli.println(buffer);
}

}  // namespace i8051
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
