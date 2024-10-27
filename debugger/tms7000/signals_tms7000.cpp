#include "signals_tms7000.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_tms7000.h"

namespace debugger {
namespace tms7000 {

void Signals::getAddress() {
    addr = busRead(ADDR);
}

void Signals::getDirection() {
    cntl() = busRead(CNTL);
    markFetch(false);
}

bool Signals::external() const {
    return (cntl() & CNTL_ENABLE) == 0;
}

bool Signals::read() const {
    return (cntl() & (CNTL_ENABLE | CNTL_RW)) == CNTL_RW;
}

bool Signals::write() const {
    return (cntl() & (CNTL_ENABLE | CNTL_RW)) == 0;
}

bool Signals::intack() const {
    return addr == 0 && (cntl() & (CNTL_ENABLE | CNTL_RW)) == (CNTL_ENABLE | CNTL_RW);
}

void Signals::getData() {
    data = busRead(AD);
}

void Signals::outData() {
    busWrite(AD, data);
    busMode(AD, OUTPUT);
}

void Signals::inputMode() const {
    busMode(AD, INPUT);
}

void Signals::print() const {
    //                              0123456789012
    static constexpr char line[] = "R A=xxxx D=xx";
    static auto &buffer = *new CharBuffer(line);
    if (fetch()) {
        buffer[0] = 'F';
    } else if (external()) {
        buffer[0] = write() ? 'W' : 'R';
    } else {
        buffer[0] = intack() ? 'I' : ((cntl() & CNTL_RW) ? 'r' : 'w');
    }
    buffer.hex16(4, addr);
    buffer.hex8(11, data);
    cli.println(buffer);
}

}  // namespace tms7000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
